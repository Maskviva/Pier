/**
 * CustomDimensionManager.cpp: the whole of dimension registration.
 * Five orderings and tests that must not be violated, each implemented where it says:
 *  1. The authority is the engine NameIdStore, not VanillaDimensions::DimensionMap or
 *     the factory map. Updating only the latter two makes registration return 3 while
 *     teleporting fails.
 *  2. The factory closure must be in place before native registration, otherwise the
 *     registry gains an entry pointing at nothing.
 *  3. The id is allocated by the engine, never from customDimensionMap.size(), since
 *     one failed config load puts the runtime id out of step with the config.
 *  4. A broken SNBT in the config must not continue past the entry: dropping the id
 *     means the next boot allocates a new one and every player's saved DimensionId is
 *     invalidated on the spot.
 *  5. VanillaDimensions::Undefined() is never rewritten; the engine uses that sentinel
 *     itself. The native path only: a failed registration is a failure.
 */
#include "pier/dimensions/dim/custom_dimension_manager.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "magic_enum.hpp"

#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/memory/Memory.h"
#include "ll/api/service/Bedrock.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/server/DedicatedServer.h"
#include "mc/server/PropertiesSettings.h"
#include "mc/util/BidirectionalUnorderedMap.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/GeneratorType.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "mc/world/level/storage/LevelStorage.h"

#include "pier/dimensions/base/native_dimensions.h"
#include "pier/dimensions/base/simple_custom_dimension.h"
#include "pier/dimensions/dim/chunk_trace.h"
#include "pier/dimensions/dim/custom_dimension_config.h"
#include "pier/dimensions/dim/dimension_height.h"
#include "pier/dimensions/dim/dimension_rules.h"

#include "pier/support/log.h"

namespace pier::dimensions
{
    namespace
    {
        using ::pier::hostLogger;

        /**
         * `SimpleCustomDimension` writes the generator type into the payload as a
         * magic_enum name. `PlotDimension` has no such field, since it takes over
         * `createGenerator` itself.
         * The value does decide the terrain for SimpleCustomDimension, whose
         * `createGenerator` switches on it. A fallback when it cannot be read is
         * therefore not harmless: it decides whether a player sees flat, nether or void,
         * so every fallback says what it fell back to (contract §5.1).
         * The name is written by `generateNewData` when the dimension is first created
         * and is never overwritten by a later argument, so a wrong choice can only be
         * fixed by editing the config or deleting and recreating the dimension.
         */
        GeneratorType readGeneratorType(CompoundTag const& nbt)
        {
            if (!nbt.contains("generatorType"))
            {
                // PlotDimension does not write this field at all, which is normal, so
                // this is debug level.
                hostLogger().debug("[dim] no generatorType in the dimension data, treating it as Flat");
                return GeneratorType::Flat;
            }
            std::string stored;
            try
            {
                auto const name = static_cast<std::string_view>(nbt.at("generatorType"));
                stored.assign(name);
                if (auto parsed = magic_enum::enum_cast<GeneratorType>(name)) return *parsed;
            }
            catch (...)
            {
                // Unreadable and read-but-unparsable are the same thing to a caller:
                // the generator type is unavailable. Both paths join the error below,
                // which carries the raw text that was read.
                stored.clear();
            }
            hostLogger().error(
                "[dim] generatorType='{}' in the dimension data could not be parsed, falling "
                "back to Flat; the terrain will differ from what was chosen at creation",
                stored
            );
            return GeneratorType::Flat;
        }

        /** Announces ready once per dimension name. A reload calls addDimension
         *  again. */
        void announceReady(std::string const& name, int id)
        {
            static std::mutex mtx;
            static std::unordered_set<std::string> announced;
            {
                std::lock_guard lock{mtx};
                if (!announced.insert(name).second) return;
            }
            hostLogger().info("[dim] '{}' ready with id {}", name, id);
        }
    } // namespace

    namespace hook_list
    {
        using ll::memory::HookPriority;

        /**
         * Cross-dimension coordinate conversion. The engine computes the ratio between
         * the vanilla dimensions itself, overworld to nether being 1:8. As soon as one
         * end is a custom dimension the coordinate is carried over unchanged, because a
         * custom dimension has no ratio to the overworld and any conversion would be a
         * guess.
         */
        LL_TYPE_STATIC_HOOK(
            VanillaDimensionsConvertPointHook,
            HookPriority::Normal,
            VanillaDimensions,
            VanillaDimensions::convertPointBetweenDimensions,
            bool,
            Vec3 const& oldPos,
            Vec3& toPos,
            DimensionType oldDim,
            DimensionType toDim,
            DimensionConversionData const& data
        )
        {
            if (oldDim <= 2 && toDim <= 2) return origin(oldPos, toPos, oldDim, toDim, data);
            toPos = oldPos;
            return true;
        };

        /*
         * fromSerializedInt takes exactly one hook.
         * Upstream MoreDimensions hooks it twice, as ...Hook and ...HookI, because the
         * BDS generation it targets really does have two overloads with the same mangled
         * shape. The 26.20 SDK has one hookable overload,
         * `Bedrock::Result<DimensionType>(Bedrock::Result<int>&&)`. The other,
         * `DimensionType fromSerializedInt(int)`, is MCFOLD: the linker folded it into a
         * byte-identical function body elsewhere, so hooking it detours a pile of
         * unrelated functions.
         * Registering the hookable one twice makes the second detour trampoline into the
         * first, and `origin()` stops meaning what the code assumes.
         */
        LL_TYPE_STATIC_HOOK(
            VanillaDimensionsFromSerializedIntHook,
            HookPriority::Normal,
            VanillaDimensions,
            VanillaDimensions::fromSerializedInt,
            Bedrock::Result<DimensionType>,
            Bedrock::Result<int>&& dim
        )
        {
            // *dim is never taken unconditionally: a Bedrock::Result may hold an error
            // and dereferencing it is undefined behavior, so one bad field in a save
            // would crash the server here. 0, 1 and 2 must go to origin, otherwise this
            // takes over deserialization of the vanilla dimensions too. The test is the
            // engine NameIdStore, of which this package's ledger is a mirror, while
            // DimensionMap serves only as the fallback mirror for fromString and
            // dimensionSelector.
            if (!dim) return origin(std::move(dim));

            int const value = *dim;
            if (value >= 0 && value <= 2) return origin(std::move(dim));

            if (!dimensionNameOf(value).empty()) return DimensionType{value};
            if (VanillaDimensions::DimensionMap().mLeft.contains(DimensionType{value}))
            {
                return DimensionType{value};
            }
            return VanillaDimensions::Undefined();
        };

        /**
         * When the dimension a player last left from is not registered this time,
         * because the config was deleted or registration failed, the Y is pushed to the
         * sentinel so the engine finds a new landing spot instead of placing the player
         * in a dimension that does not exist.
         */
        LL_TYPE_INSTANCE_HOOK(
            LevelStorageLoadServerPlayerDataHook,
            HookPriority::Normal,
            LevelStorage,
            &LevelStorage::loadServerPlayerData,
            std::unique_ptr<class CompoundTag>,
            Player const& client,
            bool isXboxLive
        )
        {
            auto result = origin(client, isXboxLive);
            if (!result) return result;

            if (!result->contains("DimensionId")) return result;
            if (!result->contains("Pos")) return result;

            int savedDim = 0;
            try
            {
                savedDim = static_cast<int>(result->at("DimensionId"));
            }
            catch (...)
            {
                // An unreadable dimension number is treated as a save that said nothing
                // about a dimension: returned unchanged so the engine follows its own
                // default flow. Assuming the overworld here is exactly the kind of
                // filled-in default contract §5.1 rejects.
                return result;
            }

            // The same test as fromSerializedInt: the engine ledger first, DimensionMap
            // only as a fallback, and the three vanilla dimensions pass straight
            // through.
            bool const known = (savedDim >= 0 && savedDim <= 2) || !dimensionNameOf(savedDim).empty()
                || VanillaDimensions::DimensionMap().mLeft.contains(DimensionType{savedDim});
            if (!known)
            {
                hostLogger().warn("[dim] dimension {} in a player save is unavailable, resetting the spawn point", savedDim);
                result->at("Pos")[1] = FloatTag{0x7fff};
            }
            return result;
        }

        /*
         * LL_AUTO_* installs itself during static initialization, which is what is
         * wanted here because `initializeHttp` runs long before any dimension
         * registration. It must therefore not also appear in the HookRegistrar below:
         * listing it in both places installs the detour twice and the reference count
         * never returns to zero on unhook.
         */
        LL_AUTO_TYPE_INSTANCE_HOOK(
            PropertiesSettingsClientSideGenHook,
            HookPriority::Normal,
            DedicatedServer,
            &DedicatedServer::initializeHttp,
            void,
            PropertiesSettings const& properties
        )
        {
            auto& mutableProperties = const_cast<PropertiesSettings&>(properties);
            mutableProperties.mClientSideGenerationEnabled = false;
            return origin(mutableProperties);
        }

        using HookReg = ll::memory::HookRegistrar<
            VanillaDimensionsConvertPointHook,
            VanillaDimensionsFromSerializedIntHook,
            LevelStorageLoadServerPlayerDataHook>;
    } // namespace hook_list

    struct CustomDimensionManager::Impl
    {
        std::mutex mMapMutex;

        struct DimensionInfo
        {
            DimensionType id;
            CompoundTag nbt;
        };

        /** name to {id, payload}, for every entry that could be fully restored. */
        std::unordered_map<std::string, DimensionInfo> customDimensionMap;

        /**
         * Names present in the config whose SNBT payload could not be parsed. The id is
         * kept and never handed to another dimension, and the payload is regenerated on
         * the next addDimension.
         * Skipping such an entry instead would drop it from customDimensionMap while it
         * stays in dimensionList, and with ids allocated as
         * `3 + customDimensionMap.size()` the runtime id falls out of step with the
         * config, dimensionList no longer finds it, and downstream this reads as a
         * teleport failing because dimension N is not registered.
         */
        std::unordered_map<std::string, int> salvagedIds;

        /** Every id the config declares, so one reload cannot hand out the same number
         *  twice. */
        std::unordered_set<int> usedIds;

        std::unordered_set<std::string> registeredDimension;
    };

    CustomDimensionManager::CustomDimensionManager() : impl(std::make_unique<Impl>())
    {
        std::lock_guard lock{impl->mMapMutex};
        CustomDimensionConfig::setDimensionConfigPath();
        CustomDimensionConfig::loadConfigFile();

        // The config mirror is used only to remember the previous id and data, to
        // deduplicate, and to warn when the engine table has drifted. The id itself is
        // always allocated by the engine DimensionManager, see step 3 of addDimension.

        for (auto& [name, info] : CustomDimensionConfig::getConfig().dimensionList)
        {
            if (info.dimId < 3)
            {
                hostLogger().error(
                    "[dim] dimension_config: '{}' has id {} while a custom dimension id starts "
                    "at 3, so this entry will be reallocated",
                    name, info.dimId
                );
                continue;
            }
            if (!impl->usedIds.insert(info.dimId).second)
            {
                hostLogger().error(
                    "[dim] dimension_config: id {} is declared by more than one dimension, '{}' will be reallocated", info.dimId, name
                );
                continue;
            }

            auto nbtTag = CompoundTag::fromSnbt(info.sNbt);
            if (!nbtTag)
            {
                hostLogger().error(
                    "[dim] dimension_config: the data of '{}' (id {}) could not be read; the id "
                    "is kept and the data is regenerated at registration",
                    name, info.dimId
                );
                impl->salvagedIds.emplace(name, info.dimId);
                continue;
            }
            impl->customDimensionMap.emplace(name, Impl::DimensionInfo{DimensionType{info.dimId}, *nbtTag});
        }


        // From 26.20 the engine supports custom dimensions natively and no packet
        // rewriting of the FakeDimensionId kind is involved. The client learns these
        // dimensions through DimensionDataPacket, and chunks, subchunks and transitions
        // all carry the real dimension id along the vanilla flow.
        hook_list::HookReg::hook();

        // Diagnostics, not installed by default. See chunk_trace.h.
        registerChunkTraceHooks();

        // Per-dimension behavior rules, installed unconditionally. A dimension with no
        // rule set goes straight to origin(), so installing them has no effect on the
        // vanilla dimensions.
        registerDimensionRuleHooks();
    }

    CustomDimensionManager::~CustomDimensionManager()
    {
        unregisterDimensionRuleHooks();
        unregisterChunkTraceHooks();
        hook_list::HookReg::unhook();
    }

    CustomDimensionManager& CustomDimensionManager::getInstance()
    {
        static CustomDimensionManager instance{};
        return instance;
    }

    DimensionType CustomDimensionManager::addDimension(
        std::string const& dimName,
        std::function<DimensionFactoryT> factory,
        std::function<CompoundTag()> const& data
    )
    {
        std::lock_guard lock{impl->mMapMutex};

        if (!ll::service::getLevel())
        {
            throw std::runtime_error("Level is not ready, cannot register dimension " + dimName);
        }

        Impl::DimensionInfo info;

        //  1. Prepare the payload, meaning seed, layout and the rest
        //
        // The payload must exist before an id is allocated, because native registration
        // reads the generator type out of it to construct the DimensionDefinition.

        bool const knownLocally = impl->customDimensionMap.contains(dimName);
        if (knownLocally)
        {
            info = impl->customDimensionMap.at(dimName);
        }
        else if (auto salvaged = impl->salvagedIds.find(dimName); salvaged != impl->salvagedIds.end())
        {
            // The SNBT of this config entry is broken while the id survives. The data is
            // regenerated and the id kept, so the DimensionId in a player save stays
            // valid.
            info.id = DimensionType{salvaged->second};
            info.nbt = data();
            impl->salvagedIds.erase(salvaged);
            hostLogger().warn("[dim] '{}' lost its data, regenerating it and keeping id {}", dimName, info.id.value());
        }
        else
        {
            info.nbt = data();
        }

        // 2. The factory closure must be in place before native registration.
        //    serverRegisterCustomDimension reaches DimensionFactory::create(name), and
        //    create() looks the closure up in mFactoryMap by name. Registering first and
        //    writing the map afterwards leaves the name absent at that moment, the engine
        //    completes registration without a closure, DimensionRegistry gains an entry
        //    pointing at nothing, the surrounding catch(...) swallows the exception, and
        //    the server feeds broken data as soon as a player enters.
        auto shared = std::make_shared<Impl::DimensionInfo>(info);

        // insert_or_assign and not emplace. A second registration within one boot, or
        // one after a reload, must replace the old closure, otherwise the engine builds
        // the dimension with the closure from the previous round.
        ll::service::getLevel()->getDimensionFactory().mFactoryMap.insert_or_assign(
            dimName,
            [dimName, shared, factory = std::move(factory)](
                DerivedDimensionArguments&& arguments) -> OwnerPtr<Dimension>
            {
                DimensionType id = shared->id;
                if (id.value() < 3)
                {
                    // Not written back yet, which means this is a re-entrant call from
                    // inside serverRegisterCustomDimension. The engine is asked
                    // directly.
                    if (auto engineId = native::engineDimensionId(dimName))
                    {
                        id = DimensionType{*engineId};
                    }
                    else
                    {
                        hostLogger().error(
                            "[dim] the factory for '{}' was called before its id was fixed and "
                            "the engine does not know it either; refusing to build the "
                            "dimension rather than using a default that would most likely be "
                            "the overworld, 0",
                            dimName
                        );
                        return {};
                    }
                }
                return factory(DimensionFactoryInfo{arguments, shared->nbt, id});
            }
        );

        // 3. The id is allocated only by native registration. From BDS 26.20 the
        //    DimensionManager carries its own NameIdStore, the engine persists the id
        //    into the save, and getOrCreateDimension recognizes nothing else. The
        //    MoreDimensions style fake-dimension path, allocating an id locally, touching
        //    only DimensionMap and the factory map, and rewriting Undefined() into
        //    something that looks like a real id, builds no dimension on 26.20 and
        //    corrupts the engine's own comparisons against Undefined(). A failed
        //    registration throws, which the GUARD in Slots.cpp turns into -1.

        auto const nativeId =
            native::registerCustomDimension(dimName, kWorldMinY, kWorldMaxY, readGeneratorType(info.nbt));

        if (!nativeId)
        {
            hostLogger().error(
                "[dim] '{}' could not be registered natively through the engine "
                "DimensionManager, so registration failed and no fake-dimension fallback is "
                "attempted; check the registerCustomDimension lines above for whether Level "
                "was ready and whether DimensionDefinitionGroup accepted the definition",
                dimName
            );
            throw std::runtime_error("native registration of dimension '" + dimName + "' failed");
        }

        if (knownLocally && info.id.value() != *nativeId)
        {
            // The id the engine gave differs from the one in the config. The engine
            // wins and the config is corrected. Such an id can only come from a config
            // written by locally-allocating logic, and those ids were never in effect on
            // the engine side, so there is no save compatibility concern.
            hostLogger().warn(
                "[dim] '{}': the config records id {} while the engine allocated {}; the engine wins",
                dimName, info.id.value(), *nativeId
            );
        }
        info.id = DimensionType{*nativeId};
        impl->usedIds.insert(*nativeId);

        // Written back. The closure reads this copy, so it must be updated before any
        // dimension is actually created.
        shared->id = info.id;
        shared->nbt = info.nbt;

        impl->customDimensionMap.insert_or_assign(dimName, info);
        rememberDimension(dimName, info.id.value());

        ll::memory::modify(VanillaDimensions::DimensionMap(), [&](auto& dimMap)
        {
            // insert_or_assign on a BidirectionalUnorderedMap only overwrites the two
            // entries it touches. If this name previously mapped to a different id, the
            // old id-to-name entry stays in mLeft and keeps resolving to a dimension that
            // no longer exists.
            if (auto it = dimMap.mRight.find(dimName); it != dimMap.mRight.end())
            {
                if (it->second.value() != info.id.value()) dimMap.mLeft.erase(it->second);
            }
            dimMap.insert_or_assign(dimName, info.id);
        });

        // Undefined() is a sentinel the engine uses itself and is never rewritten. The
        // MoreDimensions rewrite compensates for versions without native support, and
        // this path is native only.

        impl->registeredDimension.emplace(dimName);

        // Persisted whenever any field differs. Writing only when the dimension looks
        // new, and doing it with emplace(), which is a no-op on an existing key, leaves a
        // config that has drifted from the runtime id permanently uncorrectable.
        {
            auto& list = CustomDimensionConfig::getConfig().dimensionList;
            auto snbt = info.nbt.toSnbt(SnbtFormat::Minimize);
            auto cur = list.find(dimName);
            if (cur == list.end() || cur->second.dimId != info.id.value() || cur->second.sNbt != snbt)
            {
                list.insert_or_assign(dimName, CustomDimensionConfig::DimensionInfo{info.id.value(), snbt});
                if (!CustomDimensionConfig::saveConfigFile())
                {
                    hostLogger().error(
                        "[dim] writing dimension_config.json failed; '{}' may receive a new id on the next boot", dimName
                    );
                }
            }
        }

        try
        {
            ll::command::CommandRegistrar::getInstance(false).addEnumValues(
                "Dimension", {{dimName, info.id}}, Bedrock::type_id<CommandRegistry, DimensionType>()
            );
        }
        catch (...)
        {
            // The command enum only affects forms that select a dimension by name, such
            // as `/execute in <name>`, while the dimension itself keeps working. This
            // degrades rather than fails, but it must say so, otherwise a working
            // dimension whose name cannot be typed in a command becomes a report with no
            // findable cause.
            hostLogger().warn("[dim] '{}' failed to register its command enum; the dimension works, but /execute in cannot use its name", dimName);
        }

        try
        {
            // The engine NameIdStore is the only test. `VanillaDimensions::fromString`
            // has a std::string ABI problem in this build and reads back garbage for a
            // custom dimension, observed as -1870061440, so it is not trusted for the
            // self-check.
            if (auto const engineId = native::engineDimensionId(dimName); !engineId || *engineId != info.id.value())
            {
                hostLogger().error(
                    "[dim] '{}' (id {}) is not present in the engine DimensionManager, so "
                    "teleporting will fail; the engine read back: {}",
                    dimName, info.id.value(),
                    engineId ? std::to_string(*engineId) : std::string{"(not registered)"}
                );
            }
            else
            {
                hostLogger().debug(
                    "[dim] '{}' passed its self-check with id {}, engine active={}",
                    dimName, info.id.value(), native::isActive(info.id.value())
                );
            }
        }
        catch (...)
        {
            hostLogger().warn("[dim] the self-check for '{}' threw; a failed self-check does not affect the dimension itself", dimName);
        }

        // The only authoritative test: the id reported by the Dimension the engine
        // really built. The name-to-id table, DimensionMap, this package's ledger and the
        // config file can all disagree, and this object is the one a player is actually
        // teleported into.
        auto* probe = native::getOrCreateByName(dimName);
        if (!probe)
        {
            hostLogger().error("[dim] '{}' registered but no instance could be built, registration failed", dimName);
            throw std::runtime_error("dimension '" + dimName + "' could not be instantiated");
        }

        int const realId = probe->getDimensionId().value();
        if (realId != info.id.value())
        {
            hostLogger().error(
                "[dim] the ledger id of '{}' is {} while the instance the engine built "
                "reports {}; teleporting a player into {} would make the engine throw on a "
                "chunk thread and abort, so registration failed",
                dimName, info.id.value(), realId, info.id.value()
            );
            // The ledger is already dirty and is rolled back, so dimensionSelector can
            // no longer find it.
            rememberDimension(dimName, -1);
            throw std::runtime_error("the id of dimension '" + dimName + "' does not match the engine instance");
        }

        announceReady(dimName, realId);
        return info.id;
    }
} // namespace pier::dimensions

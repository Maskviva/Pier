#include "pier/dimensions/base/native_dimensions.h"

#include <cstdlib>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "ll/api/service/Bedrock.h"

#include "mc/world/level/DimensionManager.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionDefinitionGroup.h"
#include "mc/world/level/dimension/DimensionType.h"
#include "mc/world/level/dimension/VanillaDimensions.h"

#include "pier/support/log.h"

namespace pier::dimensions
{
    namespace
    {
        using pier::hostLogger;

        /**
         * The height advertised to the client. It affects only the copy written into the
         * DimensionDefinition, not the Dimension::mHeightRange the server generates and validates
         * against.
         * A diagnostic knob and not a feature. With the same -64..320 definition, the overworld
         * client requests subchunks -4..4, which is correct, while a custom dimension client
         * requests -32..-24, treating the bottom as subchunk -32, y=-512. More than one formula
         * fits a single data point, so this pair can be overridden through PIER_DIM_DEF_MIN and
         * PIER_DIM_DEF_MAX to fix the line with a second data point.
         *
         * It is not persisted: dimension_config.json holds the seed and the layout, the dimension
         * definition is rebuilt on every boot, a change can simply be changed back, and blocks in
         * the save are unaffected. / */
        std::pair<int, int> advertisedRange(int minY, int maxY)
        {
            static auto const override_ = []() -> std::optional<std::pair<int, int>>
            {
                auto const* lo = std::getenv("PIER_DIM_DEF_MIN");
                auto const* hi = std::getenv("PIER_DIM_DEF_MAX");
                if (!lo || !hi) return std::nullopt;
                try
                {
                    return std::pair<int, int>{std::stoi(lo), std::stoi(hi)};
                }
                catch (...)
                {
                    return std::nullopt;
                }
            }();

            if (!override_) return {minY, maxY};

            hostLogger().warn(
                "[dim] diagnostic override active: the height advertised to the client is "
                "{}..{} while the server still uses {}..{}; this exists only to locate a "
                "subchunk index mismatch, unset both variables once done",
                override_->first, override_->second, minY, maxY
            );
            return *override_;
        }

        DimensionManager* managerOrNull()
        {
            auto level = ll::service::getLevel();
            if (!level) return nullptr;
            return &level->getDimensionManager();
        }

        std::mutex& ledgerMutex()
        {
            static std::mutex m;
            return m;
        }

        std::map<std::string, int>& ledgerByName()
        {
            static std::map<std::string, int> m;
            return m;
        }

        std::map<int, std::string>& ledgerById()
        {
            static std::map<int, std::string> m;
            return m;
        }
    } // namespace

    //  The ledger

    void rememberDimension(std::string const& name, int id)
    {
        std::lock_guard lock{ledgerMutex()};

        // The same name under a new id. It should not happen, but if it does the old
        // reverse entry must be cleared, otherwise dimensionNameOf(old id) keeps pointing
        // at a dimension that no longer exists.
        if (auto it = ledgerByName().find(name); it != ledgerByName().end() && it->second != id)
        {
            ledgerById().erase(it->second);
        }
        ledgerByName()[name] = id;
        ledgerById()[id] = name;
    }

    std::string dimensionNameOf(int id)
    {
        std::lock_guard lock{ledgerMutex()};
        auto it = ledgerById().find(id);
        return it == ledgerById().end() ? std::string{} : it->second;
    }

    int dimensionIdOf(std::string_view name)
    {
        std::lock_guard lock{ledgerMutex()};
        auto it = ledgerByName().find(std::string{name});
        return it == ledgerByName().end() ? -1 : it->second;
    }

    void forEachRegisteredDimension(std::function<void(std::string const&, int)> const& fn)
    {
        std::lock_guard lock{ledgerMutex()};
        for (auto const& [name, id] : ledgerByName()) fn(name, id);
    }

    std::string describeRegisteredDimensions()
    {
        std::string out;
        forEachRegisteredDimension([&](std::string const& name, int id)
        {
            if (!out.empty()) out += ", ";
            out += name + "=" + std::to_string(id);
        });
        return out.empty() ? std::string{"(none)"} : out;
    }

    //  Native registration

    namespace native
    {
        bool available() { return managerOrNull() != nullptr; }

        std::optional<int> engineDimensionId(std::string const& name)
        {
            auto* mgr = managerOrNull();
            if (!mgr) return std::nullopt;
            try
            {
                int const v = mgr->getDimensionId(std::string_view{name}).value();

                // A custom dimension is always 3 or above.
                if (v < 3) return std::nullopt;

                // Being 3 or above does not mean registered: an unregistered name comes
                // back as Undefined(). Testing only for less than 3 relies on Undefined()
                // being rewritten at runtime to a large number. The native path never
                // rewrites it, so it stays at 3 and every unregistered name would be read
                // as an existing id 3.
                if (v == ::VanillaDimensions::Undefined().value()) return std::nullopt;

                return v;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        bool isActive(int dimId)
        {
            auto* mgr = managerOrNull();
            if (!mgr) return false;
            try
            {
                return mgr->isDimensionTypeActive(DimensionType{dimId});
            }
            catch (...)
            {
                return false;
            }
        }

        std::optional<int>
        registerCustomDimension(std::string const& name, int minY, int maxY, GeneratorType gen)
        {
            auto* mgr = managerOrNull();
            if (!mgr)
            {
                hostLogger().error("[dim] native registration: Level is not open, DimensionManager is unavailable");
                return std::nullopt;
            }

            // The id NameIdStore restored from the save, when there is one.
            //
            // A restored name-to-id table does not mean the dimension is usable this
            // session: NameIdStore is persisted while wiring the dimension into
            // DimensionRegistry and the factory is not. Returning here would leave the
            // factory binding permanently unestablished after every restart.
            auto const preexisting = engineDimensionId(name);

            // 1) A known name: only the factory binding is restored and NameIdStore is
            //    left alone. Success ends the call.
            if (preexisting)
            {
                try
                {
                    mgr->_registerCustomDimensionWithFactory(
                        std::string_view{name}, DimensionType{*preexisting});
                }
                catch (std::exception const& e)
                {
                    hostLogger().warn(
                        "[dim] '{}' (id {}) threw while rebinding the engine factory: {}", name, *preexisting, e.what());
                }
                catch (...)
                {
                    hostLogger().warn("[dim] '{}' (id {}) threw an unknown exception while rebinding the engine factory", name, *preexisting);
                }

                // The definition is restored unconditionally and an existing one is left
                // alone. DimensionDefinitionGroup is not persisted, is rebuilt empty every
                // boot, and only dimensions completing step 2 add an entry, while this
                // branch is taken whenever the name is already in NameIdStore, meaning
                // every boot after the first. Returning here leaves the group and
                // DimensionDataPacket without the definition, so the client drops chunks
                // of a dimension id it lacks a definition for while the server reaches
                // Loaded and the player sees nothing.
                try
                {
                    auto& group = mgr->getDimensionDefinitionGroup();
                    if (!group.getDimensionDefinition(name).has_value())
                    {
                        auto const [advMin, advMax] = advertisedRange(minY, maxY);
                        DimensionDefinitionGroup::DimensionDefinition def{
                            advMin, advMax, gen, DimensionType{*preexisting}
                        };
                        if (group.tryAddDimensionDefinition(name, def))
                        {
                            hostLogger().info(
                                "[dim] '{}' (id {}) had no definition in DimensionDefinitionGroup "
                                "this boot and one was added (height {}..{}); without it the "
                                "client does not recognize the dimension and its chunks do not "
                                "render",
                                name, *preexisting, minY, maxY
                            );
                        }
                        else
                        {
                            hostLogger().error(
                                "[dim] the definition of '{}' (id {}) could not be added to "
                                "DimensionDefinitionGroup; the client will most likely not "
                                "receive chunks of this dimension",
                                name, *preexisting
                            );
                        }
                    }
                }
                catch (std::exception const& e)
                {
                    hostLogger().error("[dim] '{}' threw while restoring its DimensionDefinition: {}", name, e.what());
                }
                catch (...)
                {
                    hostLogger().error("[dim] '{}' threw an unknown exception while restoring its DimensionDefinition", name);
                }

                // Nothing is probed here. getOrCreateDimension really builds the
                // dimension, and at this moment the id is not final, since the caller's
                // factory closure still reads the old shared->id. Probing here leaves an
                // instance built under the wrong id that no later id change can displace.
                // Whether it can be built is verified once by the caller after the id is
                // written back.
                rememberDimension(name, *preexisting);
                return preexisting;
            }

            // 2) serverRegisterCustomDimension reads the geometry from
            //    DimensionDefinitionGroup by name, so the definition must be there first;
            //    a behavior-pack JSON dimension takes the same path and this adds an
            //    equivalent one by hand. The group goes whole into DimensionDataPacket,
            //    telling the client the dimension exists, how tall it is and which
            //    generator it uses, so it accepts chunks with the real id.
            //    DimensionDataPacket must not be intercepted: FakeDimensionId excludes it,
            //    and both together give slow switches, a crash after loading, empty chunks.
            try
            {
                auto& group = mgr->getDimensionDefinitionGroup();
                if (!group.getDimensionDefinition(name).has_value())
                {
                    // A known id is written in directly, which is more reliable than
                    // waiting for the engine to write it back. Only a brand new dimension
                    // uses the -1 placeholder, as a JSON-loaded dimension does, since its
                    // id cannot be known in advance either.
                    auto const [advMin, advMax] = advertisedRange(minY, maxY);
                    DimensionDefinitionGroup::DimensionDefinition def{
                        advMin,
                        advMax,
                        gen,
                        DimensionType{preexisting ? *preexisting : -1}
                    };
                    if (!group.tryAddDimensionDefinition(name, def))
                    {
                        hostLogger().warn(
                            "[dim] the definition of '{}' could not be added to "
                            "DimensionDefinitionGroup; registration continues and falls back "
                            "if it fails",
                            name
                        );
                    }
                }
            }
            catch (std::exception const& e)
            {
                hostLogger().error("[dim] '{}' threw while writing to DimensionDefinitionGroup: {}", name, e.what());
                return std::nullopt;
            }
            catch (...)
            {
                hostLogger().error("[dim] '{}' threw an unknown exception while writing to DimensionDefinitionGroup", name);
                return std::nullopt;
            }

            // 3) The real registration. The engine allocates the id, writes NameIdStore
            //    and records the factory.
            std::optional<DimensionType> assigned;
            try
            {
                assigned = mgr->serverRegisterCustomDimension(std::string_view{name});
            }
            catch (std::exception const& e)
            {
                hostLogger().error("[dim] serverRegisterCustomDimension('{}') threw: {}", name, e.what());
                return std::nullopt;
            }
            catch (...)
            {
                hostLogger().error("[dim] serverRegisterCustomDimension('{}') threw an unknown exception", name);
                return std::nullopt;
            }

            if (!assigned)
            {
                // When the name is already in NameIdStore the engine most likely returns
                // empty, meaning it is already registered. Dropping the id there is wrong:
                // the caller would fall back to allocating one locally, pick an id that
                // disagrees with the save, and everything players already built is lost.
                if (preexisting)
                {
                    hostLogger().warn(
                        "[dim] serverRegisterCustomDimension('{}') returned empty while "
                        "NameIdStore already holds id {}, which is kept (engine active={})",
                        name, *preexisting, isActive(*preexisting)
                    );
                    rememberDimension(name, *preexisting);
                    return preexisting;
                }

                hostLogger().error(
                    "[dim] serverRegisterCustomDimension('{}') returned empty, so the engine "
                    "refused the registration; the usual causes are calling too early, "
                    "before NameIdStore is loaded from the save, or too late, after every "
                    "dimension has been created",
                    name
                );
                return std::nullopt;
            }

            int const id = assigned->value();

            if (preexisting && *preexisting != id)
            {
                // The engine returned an id different from the one in the save, which
                // orphans every chunk and every piece of player data referencing the old
                // id. That is not something to swallow silently.
                hostLogger().error(
                    "[dim] '{}' changed id from {} to {} on re-registration; every chunk and "
                    "player position stored under the old id in the save is orphaned",
                    name, *preexisting, id
                );
            }

            // 4) The read-back check, which catches a registration that looks successful
            //    while the id does not match, the silent mismatch that finally surfaces as
            //    a failed teleport.
            auto const readBack = engineDimensionId(name);
            if (!readBack || *readBack != id)
            {
                hostLogger().error(
                    "[dim] '{}' registered as id {} while getDimensionId reads back {}; the "
                    "engine ledger is inconsistent and the native path is abandoned",
                    name, id, readBack ? std::to_string(*readBack) : std::string{"(none)"}
                );
                return std::nullopt;
            }

            // 5) The real id is written back into that definition in
            //    DimensionDefinitionGroup. Step 2 filled mDimensionType with the -1
            //    placeholder, and the engine usually writes it back itself inside
            //    _registerCustomDimensionWithDimensionDefinitionGroup, but that cannot be
            //    relied on: the group is serialized whole into DimensionDataPacket and a
            //    dimension type of -1 is enough to make the client fail to parse it.
            try
            {
                auto& defs = *mgr->getDimensionDefinitionGroup().mDimensionDefinitions;
                if (auto it = defs.find(name);
                    it != defs.end() && it->second.mDimensionType->value() != id)
                {
                    hostLogger().debug(
                        "[dim] mDimensionType in the DimensionDefinition of '{}' was {}, rewritten to {}",
                        name, it->second.mDimensionType->value(), id
                    );
                    it->second.mDimensionType = DimensionType{id};
                }
            }
            catch (...)
            {
                hostLogger().warn("[dim] writing the id back into the DimensionDefinition of '{}' failed; registration itself is unaffected", name);
            }

            hostLogger().debug("[dim] '{}' registered natively by the engine with id {} (height {}..{}), active={}",
                               name, id, minY, maxY, isActive(id));
            rememberDimension(name, id);
            return id;
        }

        Dimension* getOrCreateByName(std::string const& name)
        {
            auto* mgr = managerOrNull();
            if (!mgr)
            {
                hostLogger().error("[dim] getOrCreateByName('{}'): Level is not open", name);
                return nullptr;
            }

            // There are three distinct failure causes, and folding them into one
            // catch(...) makes none of them diagnosable:
            //   a) the name is not in NameIdStore  -> registration never took effect
            //   b) present but active=false        -> the factory binding is missing, so
            //                                         it was not registered this session
            //   c) both fine but lock() is empty   -> the factory closure returned empty
            auto const id = engineDimensionId(name);
            if (!id)
            {
                hostLogger().error("[dim] getOrCreateByName('{}'): the engine NameIdStore does not have this name", name);
                return nullptr;
            }
            // active=false must not block: it is observed to be false whenever the
            // dimension instance has not been built yet, and building it is precisely what
            // getOrCreateDimension is for. Returning here would block the only real
            // attempt, so this only records the fact.
            if (!isActive(*id))
            {
                hostLogger().debug(
                    "[dim] getOrCreateByName('{}'): id {} is currently active=false, creating anyway", name, *id);
            }

            try
            {
                auto ref = mgr->getOrCreateDimension(std::string_view{name});
                auto ptr = ref.lock();
                if (!ptr)
                {
                    hostLogger().error(
                        "[dim] getOrCreateByName('{}'): id {} is ready but getOrCreateDimension "
                        "returned an empty reference, so the closure in mFactoryMap returned "
                        "empty; check that the factory was in place before registration",
                        name, *id
                    );
                    return nullptr;
                }
                return &*ptr;
            }
            catch (std::exception const& e)
            {
                hostLogger().error("[dim] getOrCreateByName('{}') threw: {}", name, e.what());
                return nullptr;
            }
            catch (...)
            {
                hostLogger().error("[dim] getOrCreateByName('{}') threw an unknown exception", name);
                return nullptr;
            }
        }
    } // namespace native
} // namespace pier::dimensions

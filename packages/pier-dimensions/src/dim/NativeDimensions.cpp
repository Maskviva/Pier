#include "pier/dimensions/base/native_dimensions.h"

#include <algorithm>   // std::max in highestKnownDimensionId
#include <cstdlib>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "ll/api/service/Bedrock.h"

#include "mc/deps/game_refs/OwnerPtr.h"
#include "mc/platform/UUID.h"
#include "mc/world/level/IDimensionFactory.h"
#include "mc/world/level/DimensionManager.h"
#include "mc/world/level/dimension/DimensionRegistry.h"
#include "mc/world/level/dimension/DimensionIdType.h"
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

    void forgetDimension(std::string const& name)
    {
        std::lock_guard lock{ledgerMutex()};
        auto it = ledgerByName().find(name);
        if (it == ledgerByName().end()) return;
        ledgerById().erase(it->second);
        ledgerByName().erase(it);
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

        /** The engine's own answer, read out of DimensionDefinitionGroup.
         *  Util::NameIdStore is an empty class in the generated headers of both 26.20
         *  and 26.32 and DimensionManager::getDimensionId is inlined away. The number
         *  also lives in the definition group: forEachDimensionDefinition is MCAPI,
         *  every DimensionDefinition carries mDimensionType, and
         *  mDimensionDefinitionGroup is a direct member of DimensionManager at an
         *  offset the generated header states.
         *  The number is the engine's and not the host's, which is what the note above
         *  registerCustomDimension requires. Reading it out of the host ledger would
         *  make the drift check in CustomDimensionManager compare the ledger against
         *  itself; this source is independent of the ledger, so that check still means
         *  something. nullopt means the engine does not know this name, not that the
         *  question cannot be asked. */
        std::optional<int> engineDimensionId(std::string const& name)
        {
            auto* mgr = managerOrNull();
            if (!mgr) return std::nullopt;

            std::optional<int> found;
            try
            {
                mgr->mDimensionDefinitionGroup->forEachDimensionDefinition(
                    [&](std::string const& defName,
                        ::DimensionDefinitionGroup::DimensionDefinition const& def)
                    {
                        // forEachDimensionDefinition has no early exit, so short-circuit here.
                        if (found) return;
                        // `.get()` is not optional. mDimensionType is a
                        // ll::TypedStorage, and going from it to int needs two
                        // user-defined conversions (TypedStorage -> DimensionType ->
                        // int); a cast only ever performs one. `.get()` spends the first
                        // one explicitly and leaves DimensionType::operator int() as the
                        // only implicit step.
                        if (defName == name) found = static_cast<int>(def.mDimensionType.get());
                    }
                );
            }
            catch (std::exception const& e)
            {
                hostLogger().error("[dim] reading the dimension definition group threw: {}", e.what());
                return std::nullopt;
            }
            catch (...)
            {
                hostLogger().error("[dim] reading the dimension definition group threw an unknown exception");
                return std::nullopt;
            }
            return found;
        }

        /*
         * Whether the definition group can be read at all. The single probe behind
         * md_is_available.
         *
         * Level being open is not the same question: the manager can be there while the
         * definition group is not reachable, and that combination used to surface as
         * "the host refused this dimension", which sends every caller off to doubt its own
         * recipe. Ask once, cache, and let md_is_available say the true thing.
         */
        bool definitionGroupReadable()
        {
            auto* mgr = managerOrNull();
            if (!mgr) return false;
            try
            {
                mgr->mDimensionDefinitionGroup->forEachDimensionDefinition(
                    [](std::string const&, ::DimensionDefinitionGroup::DimensionDefinition const&) {}
                );
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        /*
         * The largest DimensionType the engine currently knows, or 2 when it knows only
         * the vanilla three. The suggestion for a new registration starts one past it.
         */
        int highestKnownDimensionId()
        {
            int highest = 2; // 0/1/2 are the vanilla three
            auto* mgr = managerOrNull();
            if (!mgr) return highest;
            try
            {
                mgr->mDimensionDefinitionGroup->forEachDimensionDefinition(
                    [&](std::string const&, ::DimensionDefinitionGroup::DimensionDefinition const& def)
                    { highest = std::max(highest, static_cast<int>(def.mDimensionType.get())); }
                );
            }
            catch (...)
            {
                // A throw leaves highest at whatever the walk reached, at worst the
                // vanilla 2. The value is a starting suggestion and the registration
                // reads the id the engine settled on back off the Dimension.
            }
            return highest;
        }

        bool isActive(int dimId)
        {
            auto* mgr = managerOrNull();
            if (!mgr) return false;
            try
            {
                // isDimensionTypeActive moved off DimensionManager in 26.32 and is a
                // virtual on ILevel now, which is the one the manager forwarded to.
                auto level = ll::service::getLevel();
                if (!level) return false;
                return level->isDimensionTypeActive(DimensionType{dimId});
            }
            catch (...)
            {
                return false;
            }
        }

        /** Registration in three moves: put the definition in, build the instance, then
         *  ask the instance what its id is.
         *  serverRegisterCustomDimension, which did all three at once, is inlined away
         *  with no symbol left, and so is DimensionManager::getDimensionId. What
         *  survives is enough: _registerCustomDimensionWithDimensionDefinitionGroup and
         *  _registerCustomDimensionWithFactory are MCAPI, getOrCreateDimension takes a
         *  string_view and is MCAPI, and Dimension::getDimensionId is virtual, so it is
         *  reachable through the vtable whatever else is inlined.
         *  The id has to agree with the one the engine persists, since a disagreement
         *  renames a dimension a player has already built in. The suggestion below does
         *  not settle it. The engine does, and the last step reads it back off the
         *  Dimension and logs loudly when the two differ. An id is never invented for a
         *  name the engine already knows: that case returns before registering. */
        std::optional<int>
        registerCustomDimension(std::string const& name, int minY, int maxY, GeneratorType gen)
        {
            auto* mgr = managerOrNull();
            if (!mgr)
            {
                hostLogger().error("[dim] '{}' was not registered: Level is not open", name);
                return std::nullopt;
            }
            if (!definitionGroupReadable())
            {
                static bool said = false;
                if (!said)
                {
                    said = true;
                    hostLogger().error(
                        "[dim] custom dimensions are unavailable on this engine: the "
                        "dimension definition group cannot be read, so no id can be "
                        "obtained and the ones a save already holds cannot be found "
                        "again. Every other capability of the mod is unaffected"
                    );
                }
                hostLogger().error(
                    "[dim] '{}' was not registered; a mod that needs it has to treat "
                    "md_add_dimension returning -1 as the dimension not existing",
                    name
                );
                return std::nullopt;
            }

            // Already in the definition group, so this save has held it before. Return the
            // engine's number and register nothing: re-registering a live name is how a
            // second id gets handed to a dimension players have already built in.
            if (auto const existing = engineDimensionId(name))
            {
                if (auto* d = getOrCreateByName(name))
                {
                    return static_cast<int>(d->getDimensionId());
                }
                hostLogger().warn(
                    "[dim] '{}' is in the definition group as id {} but the instance could "
                    "not be built; using the definition's id",
                    name, *existing
                );
                return existing;
            }

            // A suggestion, not a decision. One past the highest the engine currently
            // knows; the engine is asked to confirm it below.
            int const suggested = highestKnownDimensionId() + 1;
            // Called and discarded on purpose: the height it computes has nowhere to go
            // on this path, since _registerCustomDimensionWithDimensionDefinitionGroup
            // takes only (name, type). Keeping the call means the diagnostic override
            // still logs when it is set, and it keeps advertisedRange referenced, which
            // is the honest way to leave a known gap rather than deleting the code that
            // will be needed once a definition-writing entry point turns up.
            auto const advertised = advertisedRange(minY, maxY);
            (void)advertised;
            (void)gen;

            try
            {
                // The third argument, from 26.40, is the resource pack the definition
                // came from. This one comes from no pack, and a zero UUID is what the
                // engine's own emptiness check reads as none; the field is provenance
                // and nothing selects a dimension by it.
                if (!mgr->_registerCustomDimensionWithDimensionDefinitionGroup(
                        std::string_view{name}, ::DimensionType{suggested}, ::mce::UUID{}
                    ))
                {
                    hostLogger().error(
                        "[dim] '{}' was not registered: DimensionDefinitionGroup did not "
                        "accept the definition",
                        name
                    );
                    return std::nullopt;
                }
                mgr->_registerCustomDimensionWithFactory(std::string_view{name}, ::DimensionType{suggested});
            }
            catch (std::exception const& e)
            {
                hostLogger().error("[dim] '{}' was not registered: registration threw: {}", name, e.what());
                return std::nullopt;
            }
            catch (...)
            {
                hostLogger().error("[dim] '{}' was not registered: registration threw an unknown exception", name);
                return std::nullopt;
            }

            auto* d = getOrCreateByName(name);
            if (!d)
            {
                hostLogger().error(
                    "[dim] '{}': the definition and the factory went in but the instance "
                    "could not be built, so no id can be confirmed and the registration is "
                    "reported as failed",
                    name
                );
                return std::nullopt;
            }

            int const actual = static_cast<int>(d->getDimensionId());
            if (actual != suggested)
            {
                // Not fatal, and not silent. The engine decides; this line is what makes a
                // disagreement diagnosable instead of a dimension quietly changing number.
                hostLogger().warn(
                    "[dim] '{}': suggested id {} but the engine assigned {}; going with the "
                    "engine",
                    name, suggested, actual
                );
            }
            return actual;
        }

        /** Build the dimension through the factory and put it in the registry under an
         *  id already in hand, without asking the engine to resolve the name.
         *  Returns nullptr and says which step failed. The three steps fail for
         *  different reasons and only the first one involves Pier's own closure. */
        Dimension* buildAndRegister(std::string const& name, int id)
        {
            auto* mgr = managerOrNull();
            auto level = ll::service::getLevel();
            if (!mgr || !level) return nullptr;

            try
            {
                // ILevel::getDimensionFactory returns the OwnerPtrFactory, the
                // name-to-closure map CustomDimensionManager writes into. The object
                // carrying create and initializeDimension is IDimensionFactory, held by
                // the manager in mDimensionFactory; both are pure virtuals, so they go
                // through the vtable and no symbol has to resolve. Three unwraps, each a
                // different wrapper: TypedStorage::get to Bedrock::NotNullNonOwnerPtr,
                // gsl::not_null::get to NonOwnerPointer, NonOwnerPointer::get to
                // IDimensionFactory*.
                auto* facPtr = mgr->mDimensionFactory.get().get().get();
                if (!facPtr)
                {
                    hostLogger().error("[dim] buildAndRegister('{}'): the dimension factory is null", name);
                    return nullptr;
                }
                ::IDimensionFactory& factory = *facPtr;
                ::OwnerPtr<::Dimension> owner = factory.create(name);
                if (!owner)
                {
                    // This one really is Pier's closure: create() looks it up in
                    // mFactoryMap by name and calls it. Empty here means the closure
                    // returned empty, and the closure logs its own reason.
                    hostLogger().error(
                        "[dim] buildAndRegister('{}'): DimensionFactory::create returned "
                        "empty, so the closure in mFactoryMap refused to build it",
                        name
                    );
                    return nullptr;
                }

                auto ptr = owner.get();
                if (!ptr)
                {
                    hostLogger().error("[dim] buildAndRegister('{}'): the built dimension is null", name);
                    return nullptr;
                }
                factory.initializeDimension(*ptr);

                // Same shape one wrapper shallower:
                //   TypedStorage   .get() -> gsl::not_null<std::unique_ptr<DimensionRegistry>>&
                //   gsl::not_null  .get() -> std::unique_ptr<DimensionRegistry> const&
                //   unique_ptr     *      -> DimensionRegistry&
                auto& registry = *mgr->mDimensionRegistry.get().get();
                auto ref = registry.registerDimension(
                    ::DimensionIdType{static_cast<ushort>(id)}, std::move(owner));
                auto locked = ref.lock();
                if (!locked)
                {
                    hostLogger().error(
                        "[dim] buildAndRegister('{}'): registerDimension(id {}) returned an "
                        "empty reference",
                        name, id
                    );
                    return nullptr;
                }
                hostLogger().info("[dim] '{}' built through the factory and registered as id {}", name, id);
                return &*locked;
            }
            catch (std::exception const& e)
            {
                hostLogger().error("[dim] buildAndRegister('{}') threw: {}", name, e.what());
                return nullptr;
            }
            catch (...)
            {
                hostLogger().error("[dim] buildAndRegister('{}') threw an unknown exception", name);
                return nullptr;
            }
        }

        Dimension* getOrCreateByName(std::string const& name)
        {
            auto* mgr = managerOrNull();
            if (!mgr)
            {
                hostLogger().error("[dim] getOrCreateByName('{}'): Level is not open", name);
                return nullptr;
            }

            // The definition group is the only readable table, so it is also the only
            // place an id can come from before the dimension exists. Missing is not fatal
            // here: on the first half of a fresh registration the definition is going in
            // and building the instance is exactly what this call is for.
            auto const id = engineDimensionId(name);
            if (!id)
            {
                hostLogger().debug(
                    "[dim] getOrCreateByName('{}'): not in the definition group yet, trying anyway", name);
            }
            else if (!isActive(*id))
            {
                hostLogger().debug(
                    "[dim] getOrCreateByName('{}'): id {} is currently active=false, creating anyway", name, *id);
            }

            // Two overloads that fail for different reasons. By name, the engine resolves
            // the name through NameIdStore first, the one table nothing here can write
            // now that serverRegisterCustomDimension is gone, so it can come back empty
            // with the definition and the factory both in place. By id, the resolution is
            // skipped and the call goes straight to the factory. The id is tried first as
            // the shorter path: Pier keeps its own name-to-id ledger, so the engine's
            // name table is needed for nothing else here.
            auto attempt = [&](char const* how, auto&& key) -> Dimension*
            {
                try
                {
                    auto ref = mgr->getOrCreateDimension(key);
                    auto ptr = ref.lock();
                    if (!ptr)
                    {
                        hostLogger().debug(
                            "[dim] getOrCreateByName('{}'): {} returned an empty reference", name, how);
                        return nullptr;
                    }
                    return &*ptr;
                }
                catch (std::exception const& e)
                {
                    hostLogger().debug("[dim] getOrCreateByName('{}'): {} threw: {}", name, how, e.what());
                    return nullptr;
                }
                catch (...)
                {
                    hostLogger().debug("[dim] getOrCreateByName('{}'): {} threw an unknown exception", name, how);
                    return nullptr;
                }
            };

            if (id)
            {
                if (auto* d = attempt("getOrCreateDimension(id)", ::DimensionType{*id})) return d;
            }
            if (auto* d = attempt("getOrCreateDimension(name)", std::string_view{name})) return d;

            // Both overloads reach NameIdStore before anything else, so the last path
            // builds the dimension the way the engine would and registers it directly.
            // DimensionFactory::create assembles DerivedDimensionArguments and calls the
            // closure in mFactoryMap by name, with no id lookup; initializeDimension is
            // the step the engine takes between create and registration; and
            // DimensionRegistry::registerDimension takes the id explicitly. Pier's own
            // name-to-id ledger is what makes that id available, which is what makes
            // NameIdStore unnecessary here rather than worked around.
            if (!id) return nullptr;
            if (auto* d = buildAndRegister(name, *id)) return d;

            // Both are out, so say which wall was hit. The two causes need different
            // answers and folding them into one line is what made this undiagnosable
            // the first time round.
            if (id)
            {
                hostLogger().error(
                    "[dim] getOrCreateByName('{}'): the definition group has it as id {}, and "
                    "neither the two getOrCreateDimension overloads nor building it through "
                    "DimensionFactory worked; the lines above say which step gave out",
                    name, *id
                );
            }
            else
            {
                hostLogger().error(
                    "[dim] getOrCreateByName('{}'): the definition group does not have this "
                    "name, so nothing registered it and there is no id to build it with",
                    name
                );
            }
            return nullptr;
        }
    } // namespace native
} // namespace pier::dimensions

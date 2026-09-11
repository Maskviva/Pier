#include "pier/dimensions/base/native_dimensions.h"

#include <algorithm>   // std::max in highestKnownDimensionId
#include <cstdlib>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

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

        /** Make the definition the engine hands out agree with the dimension that gets built.
         *
         *  `_registerCustomDimensionWithDimensionDefinitionGroup` takes a name and an id and
         *  nothing else, so the definition it creates carries the engine's own defaults for
         *  height and generator while the Dimension is constructed from the stored spec.
         *  Different parts of the engine read the two, and a server that entered such a
         *  dimension died on a chunk worker a third of a second later, every time, on an
         *  indirect call through a value that is not a function: the shape of what one side
         *  writes is not the shape the other side allocated for.
         *
         *  The entry is reached through the engine's own iterator rather than by walking the
         *  map from here, and it is written only once its `mDimensionType` reads back as the
         *  id just registered, which is what proves the fields are where the header says. */
        bool alignDefinitionShape(DimensionManager* mgr, std::string const& name, int id, int minY, int maxY, GeneratorType gen)
        {
            using Definition = ::DimensionDefinitionGroup::DimensionDefinition;
            auto& group = mgr->mDimensionDefinitionGroup.get();

            Definition* entry = nullptr;
            bool found = false;
            group.forEachDimensionDefinition([&](std::string const& n, Definition const& d)
            {
                if (n != name) return;
                found = true;
                if (d.mDimensionType.get().mValue != id)
                {
                    hostLogger().error(
                        "[dim] '{}': the definition reads as id {} where {} was registered, so the "
                        "definition fields are not where they are expected and none of them will be "
                        "written; the dimension keeps whatever height the engine gave it",
                        name, d.mDimensionType.get().mValue, id
                    );
                    return;
                }
                hostLogger().info(
                    "[dim] '{}': the engine's definition says height {}..{}, generator {}; the spec "
                    "asks for {}..{}, generator {}",
                    name, d.mHeightMinimum, d.mHeightMaximum, static_cast<int>(d.mGeneratorType),
                    minY, maxY, static_cast<int>(gen)
                );
                entry = const_cast<Definition*>(&d);
            });

            if (!found)
            {
                hostLogger().error(
                    "[dim] '{}': the engine has no definition for this name right after registering "
                    "one, so its height and generator cannot be made to agree with the dimension",
                    name
                );
                return false;
            }
            if (!entry) return false;

            entry->mHeightMinimum = minY;
            entry->mHeightMaximum = maxY;
            entry->mGeneratorType = gen;

            bool agrees = false;
            group.forEachDimensionDefinition([&](std::string const& n, Definition const& d)
            {
                if (n != name) return;
                agrees = d.mHeightMinimum == minY && d.mHeightMaximum == maxY && d.mGeneratorType == gen;
            });
            if (!agrees)
            {
                hostLogger().error(
                    "[dim] '{}': the definition did not take the height {}..{}; the dimension and the "
                    "engine disagree about its shape and entering it is not safe",
                    name, minY, maxY
                );
                return false;
            }
            hostLogger().info("[dim] '{}': definition set to height {}..{}", name, minY, maxY);
            return true;
        }

        DimensionManager* managerOrNull()
        {
            auto level = ll::service::getLevel();
            if (!level) return nullptr;
            return &level->getDimensionManager();
        }

        /** A Dimension carries two numbers. `mId` is a DimensionType, a signed int, and
         *  is what commands, saves and this host speak. `mRegistryId` is a
         *  DimensionIdType, an unsigned short, and is the key of
         *  DimensionRegistry::mDimensions, which is what registerDimension takes. The
         *  only thing mapping between them is DimensionManager::mDimensionNameIdStore,
         *  the table `serverRegisterCustomDimension` wrote until 26.20 and that nothing
         *  exported writes now, so a dimension built for a name that table has never
         *  held comes out with the two disagreeing.
         *
         *  The field is declared `DimensionIdType const`, and ll::TypedStorage runs the
         *  type through std::remove_cv, so the storage member is not const and the value
         *  is writable through it. */
        int registryIdOf(Dimension& d) { return static_cast<int>(d.mRegistryId.get().mValue); }

        void setRegistryId(Dimension& d, int id) { d.mRegistryId.get().mValue = static_cast<ushort>(id); }

        /** The dimensions the level is holding right now, by both of their numbers.
         *
         *  Through ILevel::forEachDimension, a pure virtual, so it goes through the
         *  vtable and reads no private field. It sees instances and not definitions: the
         *  nether and the end are built on first use, so a boot with nobody in them lists
         *  the overworld alone and that is healthy. The overworld is the one that must
         *  always be there, and `found` reports on it alone. */
        std::string dimensionCensus(bool* overworldPresent, Dimension const* lookFor, bool* found)
        {
            if (overworldPresent) *overworldPresent = false;
            if (found) *found = false;
            auto level = ll::service::getLevel();
            if (!level) return "(Level is not open)";

            std::string out;
            bool overworld = false;
            try
            {
                level->forEachDimension([&](Dimension& d) -> bool
                {
                    int const type = static_cast<int>(d.mId.get());
                    int const reg = static_cast<int>(d.mRegistryId.get().mValue);
                    if (type == 0) overworld = true;
                    if (found && lookFor == &d) *found = true;
                    if (!out.empty()) out += ", ";
                    out += d.mName.get() + " type=" + std::to_string(type) + " registry=" + std::to_string(reg);
                    return true;
                });
            }
            catch (...)
            {
                return "(walking the level's dimensions threw)";
            }
            if (overworldPresent) *overworldPresent = overworld;
            return out.empty() ? std::string{"(none)"} : out;
        }

        /** Dimensions that were built and initialized and that the registry then refused.
         *
         *  initializeDimension is where the engine wires a Dimension into the level, and
         *  what it hands out there it does not own. Dropping the last OwnerPtr after that
         *  step leaves those references pointing at freed memory, and the level tick
         *  reached one a second later and took the process down with an abort. Holding
         *  the object costs a dimension's worth of memory until the server stops, which
         *  is the cheaper of the two. */
        void quarantine(::OwnerPtr<::Dimension> d)
        {
            static std::mutex mtx;
            static std::vector<::OwnerPtr<::Dimension>> held;
            std::lock_guard lock{mtx};
            held.push_back(std::move(d));
        }

        /** A counter that moves one step every time a dimension becomes available.
         *
         *  A client is told which dimensions exist once, in the dimension data it gets
         *  while joining, and it has no way to be told again during a session. Comparing
         *  the counter a dimension was registered at against the one a player joined at
         *  is therefore the same question as whether that player's game knows it. */
        std::mutex& sessionMutex()
        {
            static std::mutex m;
            return m;
        }

        unsigned long long& availabilityEpoch()
        {
            static unsigned long long epoch = 0;
            return epoch;
        }

        std::map<int, unsigned long long>& dimensionEpoch()
        {
            static std::map<int, unsigned long long> m;
            return m;
        }

        std::map<long long, unsigned long long>& sessionEpoch()
        {
            static std::map<long long, unsigned long long> m;
            return m;
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
        if (it != ledgerByName().end()) return it->second;
        // The engine calls by the qualified name, native::engineNameOf, while the ledger
        // is keyed by the one the caller chose. A detour that answers only to the second
        // hands the engine's own call straight back to the engine.
        auto const sep = name.find(':');
        if (sep == std::string_view::npos) return -1;
        it = ledgerByName().find(std::string{name.substr(sep + 1)});
        return it == ledgerByName().end() ? -1 : it->second;
    }

    void noteDimensionAvailable(int id)
    {
        std::lock_guard lock{sessionMutex()};
        dimensionEpoch()[id] = ++availabilityEpoch();
    }

    void noteClientSession(long long playerId)
    {
        std::lock_guard lock{sessionMutex()};
        sessionEpoch()[playerId] = availabilityEpoch();
    }

    bool clientKnowsDimension(long long playerId, int id)
    {
        std::lock_guard lock{sessionMutex()};
        auto const dim = dimensionEpoch().find(id);
        if (dim == dimensionEpoch().end()) return true;
        auto const session = sessionEpoch().find(playerId);
        // An unrecorded player is answered yes. Refusing a teleport on a session this
        // host never saw start would break a working server to guard against a screen
        // that reads wrong.
        if (session == sessionEpoch().end()) return true;
        return dim->second <= session->second;
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

        std::string engineNameOf(std::string const& name)
        {
            if (name.find(':') != std::string::npos) return name;
            return "pier:" + name;
        }

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
        std::optional<int> engineDimensionId(std::string const& rawName)
        {
            auto const name = engineNameOf(rawName);
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
         * The largest DimensionType the engine currently knows, or one below the first
         * custom id when it knows only the vanilla three. The suggestion for a new
         * registration starts one past it.
         */
        int highestKnownDimensionId()
        {
            int highest = native::firstCustomDimensionId - 1;
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

        /** The key a Dimension of this host's must go into the registry under, with the
         *  object's own registry id set to match. nullopt for anything else, which is
         *  left exactly as the engine built it.
         *
         *  The test is the dimension's own DimensionType against the floor, not the host
         *  ledger: the ledger is written after a registration returns, so during the
         *  first one for a name it does not hold it yet, while the DimensionType is set
         *  by the factory closure before the object exists. It is the same test the
         *  resolution hooks already make.
         *
         *  Said once per id. The engine calls the factory on paths this host does not
         *  see, and a line per chunk load would be the only thing in the log. */
        std::optional<int> claimRegistryKey(Dimension& d)
        {
            int const type = static_cast<int>(d.mId.get());
            if (type < firstCustomDimensionId) return std::nullopt;

            int const reg = registryIdOf(d);
            if (reg != type)
            {
                static std::mutex mtx;
                static std::unordered_set<int> said;
                bool first = false;
                {
                    std::lock_guard lock{mtx};
                    first = said.insert(type).second;
                }
                if (first)
                {
                    hostLogger().warn(
                        "[dim] '{}': the engine built this dimension with registry id {} while its "
                        "dimension id is {}. Its name is not in the engine's name table, which is "
                        "the only thing that maps between the two, so the constructor had nothing "
                        "to read. The registry is keyed by the first number, so leaving it would "
                        "put this dimension in slot {} and destroy whatever is in it; it is set to "
                        "{} instead",
                        d.mName.get(), reg, type, reg, type
                    );
                }
                setRegistryId(d, type);
            }
            return type;
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
        std::optional<int> registerCustomDimension(
            std::string const& rawName, int minY, int maxY, GeneratorType gen,
            std::unordered_set<int> const& taken, std::function<void()> const& bindFactory,
            int preferred
        )
        {
            auto const name = engineNameOf(rawName);
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
            if (auto const existing = engineDimensionId(rawName))
            {
                // The engine may have bound a factory of its own for the name by now;
                // the caller's closure has to be the one create() finds.
                bindFactory();
                // In the ledger before anything builds it: the detours on the resolution
                // calls decide whether an id is this host's by asking the ledger, and one
                // that is not in it yet is handed straight to the engine, which then builds
                // and registers the dimension on a path this host never sees.
                rememberDimension(rawName, *existing);
                if (auto* d = getOrCreateByName(rawName))
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

            // A suggestion, not a decision: the engine confirms it below and the id is
            // read back off the Dimension it built. One past the highest the engine knows,
            // never below the floor that keeps it off VanillaDimensions::Undefined(), and
            // never one `taken` already holds. That last one matters because the definition
            // group holds only what this boot has registered so far, so a dimension named
            // in dimension_config.json but not yet registered is invisible to the walk, and
            // handing its number to another puts two dimensions on one id, each reading the
            // other's chunks.
            int suggested = std::max(highestKnownDimensionId() + 1, native::firstCustomDimensionId);
            while (taken.contains(suggested)) ++suggested;
            // The number this dimension already holds wins when nothing else has it: its
            // saved chunks are under that number and nowhere else.
            if (preferred >= native::firstCustomDimensionId && !taken.contains(preferred))
            {
                suggested = preferred;
            }
            // The height the definition will carry. Written into the definition below,
            // because the registration call takes only a name and an id.
            auto const advertised = advertisedRange(minY, maxY);

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
                // After, not before: that call registers the engine's own factory for the
                // name over whatever was in the map. On a live server the closure bound
                // ahead of it never ran, and what create() built for the name was a
                // vanilla-class dimension for the definition, which is what the player
                // then entered.
                bindFactory();
                // Before the instance is built: the Dimension takes its height from the
                // spec and the engine takes it from here, and they have to be one shape.
                alignDefinitionShape(mgr, name, suggested, advertised.first, advertised.second, gen);
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

            // In the ledger before the instance is built, for the reason above. Taken back
            // out when nothing could be built, so a name that is not registered does not
            // keep answering to an id.
            rememberDimension(rawName, suggested);

            auto* d = getOrCreateByName(rawName);
            if (!d)
            {
                forgetDimension(rawName);
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

        /** Three answers about the instance registered under an id, and the difference
         *  between the last two decides whether anything may be built.
         *
         *  Present:   the engine holds a Dimension under this id and it must be used.
         *  Absent:    the engine answered and holds nothing, so building is safe.
         *  Unknown:   the engine could not be asked, so building is not safe either. */
        enum class Registered
        {
            Present,
            Absent,
            Unknown
        };

        /** The instances this host registered, by id.
         *
         *  The engine cannot find them: DimensionManager::getDimension resolves a
         *  DimensionType through the engine name table, and nothing exported writes a row
         *  there for a dimension registered from outside, so it answers empty for one that
         *  is registered and running. Without a memory of its own every resolution reported
         *  the dimension as absent and built another, and registerDimension assigns into
         *  its map: the new instance replaced the one the level, the players and the chunk
         *  threads were holding, and a chunk worker fastfailed seconds later on a call
         *  through a pointer in the destroyed object. A live server built the same
         *  dimension six times in three seconds this way.
         *
         *  A WeakRef and not a raw pointer, so a dimension the engine does drop is seen to
         *  be gone and built again rather than handed out after the fact. */
        std::mutex& instanceMutex()
        {
            static std::mutex m;
            return m;
        }

        std::map<int, ::OwnerPtr<::Dimension>>& instanceLedger()
        {
            static std::map<int, ::OwnerPtr<::Dimension>> ledger;
            return ledger;
        }

        void rememberInstance(int id, ::OwnerPtr<::Dimension> owner)
        {
            std::lock_guard lock{instanceMutex()};
            instanceLedger().insert_or_assign(id, std::move(owner));
        }

        /** The dimension this host registered under an id, null when there is none.
         *
         *  Held and not merely pointed at. A weak reference here answers empty the moment
         *  the registry lets go, and every path that reads it treats empty as permission
         *  to build: a live server built one dimension twice within a millisecond that
         *  way, and the second registration destroyed the object the first one had
         *  already handed to the engine. The dimension a host registered stays until it
         *  is retired, and answering Present for one that is really gone is the safer of
         *  the two mistakes, because the other one ends the process. */
        Dimension* rememberedPointer(int id)
        {
            std::lock_guard lock{instanceMutex()};
            auto it = instanceLedger().find(id);
            return it == instanceLedger().end() ? nullptr : it->second.get();
        }

        ::WeakRef<::Dimension> rememberedInstance(int id)
        {
            std::lock_guard lock{instanceMutex()};
            auto it = instanceLedger().find(id);
            if (it == instanceLedger().end()) return {};
            return ::WeakRef<::Dimension>{it->second};
        }

        /** The instance under an id, without creating one: the engine's own lookup first,
         *  then what this host registered itself, for the reason above.
         *
         *  Present must be used, Absent may be built, Unknown may not be built either.
         *  registerDimension assigns into the registry, so building for an id that already
         *  has an instance destroys the Dimension the level and the chunk threads hold. */
        Registered registeredInstance(DimensionManager* mgr, int id, Dimension** out)
        {
            if (out) *out = nullptr;
            // The host's own memory first for the ids it registered. It is the authority
            // for them, and asking the engine ahead of it is what let a second instance be
            // built for an id while the first was live and in use.
            if (id >= firstCustomDimensionId)
            {
                if (auto* held = rememberedPointer(id))
                {
                    if (out) *out = held;
                    return Registered::Present;
                }
            }
            if (!mgr) return Registered::Unknown;
            try
            {
                auto ref = mgr->getDimension(::DimensionType{id});
                auto locked = ref.lock();
                if (auto* ptr = locked.get())
                {
                    if (out) *out = ptr;
                    return Registered::Present;
                }
            }
            catch (std::exception const& e)
            {
                hostLogger().error(
                    "[dim] asking the engine whether id {} already has an instance threw: "
                    "{}. Nothing will be built for it, because building over a live "
                    "dimension destroys the one in use",
                    id, e.what()
                );
                return Registered::Unknown;
            }
            catch (...)
            {
                hostLogger().error(
                    "[dim] asking the engine whether id {} already has an instance threw. "
                    "Nothing will be built for it, because building over a live dimension "
                    "destroys the one in use",
                    id
                );
                return Registered::Unknown;
            }

            return Registered::Absent;
        }

        /** Build the dimension through the factory and put it in the registry under an
         *  id already in hand, without asking the engine to resolve the name.
         *  Returns nullptr and says which step failed. The three steps fail for
         *  different reasons and only the first one involves Pier's own closure.
         *  An id that already carries an instance is never built again: see
         *  registeredInstance. */
        Dimension* buildAndRegister(std::string const& rawName, int id)
        {
            auto const name = engineNameOf(rawName);
            auto* mgr = managerOrNull();
            auto level = ll::service::getLevel();
            if (!mgr || !level) return nullptr;

            Dimension* live = nullptr;
            switch (registeredInstance(mgr, id, &live))
            {
            case Registered::Present:
                // Reached when a caller asked to build a dimension that is already
                // running. Handing back the live one is correct and building a second
                // is not survivable, so the request is refused rather than served.
                hostLogger().warn(
                    "[dim] buildAndRegister('{}', id {}): an instance is already registered "
                    "under this id, so the existing one is returned and nothing is built",
                    name, id
                );
                return live;
            case Registered::Unknown:
                return nullptr; // registeredInstance said why
            case Registered::Absent:
                break;
            }

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
                // The object must carry the id it is about to be registered under. One
                // registered under a key other than its own id is found by the key and
                // reports the id, and the two disagree from then on: a live server ended
                // up with two instances for one name that way and died in the level tick.
                if (auto const built = static_cast<int>(ptr->getDimensionId()); built != id)
                {
                    hostLogger().error(
                        "[dim] buildAndRegister('{}'): the factory built the dimension as id {} "
                        "while it is being registered as id {}; refusing to register it under "
                        "a key that is not its own id. The instance is released",
                        name, built, id
                    );
                    return nullptr;
                }
                int const builtRegistryId = registryIdOf(*ptr);
                // The key and the object's own registry id have to be one number, and
                // until this line they are not. registerDimension assigns into the
                // registry, so a dimension carrying registry id 0, which is what the
                // constructor produces for a name the engine's table has never held, is
                // stored over the overworld's slot and destroys what was there.
                // claimRegistryKey sets it; the read below is the refusal that has to
                // exist if it did not take, because that store is not survivable.
                claimRegistryKey(*ptr);
                if (int const after = registryIdOf(*ptr); after != id)
                {
                    hostLogger().error(
                        "[dim] '{}': the registry id would not take the value {} and still reads {}; "
                        "refusing to register, because the slot it would go into belongs to another "
                        "dimension",
                        name, id, after
                    );
                    return nullptr;
                }

                factory.initializeDimension(*ptr);

                // Same shape one wrapper shallower:
                //   TypedStorage   .get() -> gsl::not_null<std::unique_ptr<DimensionRegistry>>&
                //   gsl::not_null  .get() -> std::unique_ptr<DimensionRegistry> const&
                //   unique_ptr     *      -> DimensionRegistry&
                auto& registry = *mgr->mDimensionRegistry.get().get();
                // A copy and not a move. On the failure path the reference this call
                // consumed would be the last one, and the object is initialized by now,
                // which means the engine holds references to it that it does not own.
                auto keep = owner;
                auto ref = registry.registerDimension(
                    ::DimensionIdType{static_cast<ushort>(id)}, std::move(owner));
                // Whether the registration took is a different question from what the
                // call returned, and the level answers it. Nothing is ever put into the
                // registry by hand when it declines: registerDimension does more than
                // store a pointer, and a Dimension placed beside it is one the engine
                // never finished wiring.
                bool overworldPresent = false;
                bool stored = false;
                auto const census = dimensionCensus(&overworldPresent, ptr, &stored);
                auto locked = ref.lock();

                if (!overworldPresent)
                {
                    hostLogger().error(
                        "[dim] the overworld is gone from the level after registering '{}' as id "
                        "{}. It holds: {}. Stop the server rather than let anyone join",
                        name, id, census
                    );
                }

                if (locked.get() || stored)
                {
                    if (!locked.get())
                    {
                        hostLogger().warn(
                            "[dim] buildAndRegister('{}'): registerDimension(id {}) returned an "
                            "empty reference while the level is holding this dimension, so the "
                            "registration took and the return value is what is unreliable",
                            name, id
                        );
                    }
                    // Remembered before anything else can ask: the next question about this
                    // id arrives on the chunk threads within the millisecond.
                    rememberInstance(id, keep);
                    hostLogger().info(
                        "[dim] '{}' built through the factory and registered as id {}, registry id {}",
                        name, id, builtRegistryId
                    );
                    hostLogger().debug("[dim] the level now holds: {}", census);
                    return ptr;
                }

                Dimension* live = nullptr;
                if (registeredInstance(mgr, id, &live) == Registered::Present && live)
                {
                    hostLogger().warn(
                        "[dim] buildAndRegister('{}'): registerDimension(id {}) returned an empty "
                        "reference while an instance is there; going by the instance",
                        name, id
                    );
                    return live;
                }

                // Held rather than dropped: see quarantine. The dimension is unusable
                // either way, but a refused registration must not also end the process.
                quarantine(std::move(keep));
                hostLogger().error(
                    "[dim] buildAndRegister('{}'): registerDimension(id {}) stored nothing, so this "
                    "dimension is not available. The level holds: {}. The instance is kept rather "
                    "than released, because the engine was told about it while it was being "
                    "initialized and would follow those references into freed memory",
                    name, id, census
                );
                return nullptr;
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

        Dimension* ensureBuilt(std::string const& rawName, int id)
        {
            auto* mgr = managerOrNull();
            if (!mgr) return nullptr;

            Dimension* live = nullptr;
            switch (registeredInstance(mgr, id, &live))
            {
            case Registered::Present:
                return live;
            case Registered::Unknown:
                return nullptr; // registeredInstance said why
            case Registered::Absent:
                break;
            }
            return buildAndRegister(rawName, id);
        }

        bool hasInstance(int dimId)
        {
            auto* mgr = managerOrNull();
            if (!mgr) return false;
            return registeredInstance(mgr, dimId, nullptr) == Registered::Present;
        }

        ::WeakRef<::Dimension> instanceRef(int dimId)
        {
            if (auto ref = rememberedInstance(dimId); ref.lock()) return ref;
            auto* mgr = managerOrNull();
            if (!mgr) return {};
            try
            {
                auto ref = mgr->getDimension(::DimensionType{dimId});
                if (ref.lock()) return ref;
            }
            catch (...)
            {
                // Not fatal here: the host's own memory was asked first and the throw
                // itself is reported by registeredInstance, which every build path goes
                // through before this one.
                hostLogger().debug("[dim] instanceRef({}): the engine lookup threw", dimId);
            }
            return {};
        }

        ::WeakRef<::Dimension> rememberedInstanceRef(int dimId) { return rememberedInstance(dimId); }

        void forgetInstance(int dimId)
        {
            ::OwnerPtr<::Dimension> released;
            {
                std::lock_guard lock{instanceMutex()};
                auto it = instanceLedger().find(dimId);
                if (it == instanceLedger().end()) return;
                released = std::move(it->second);
                instanceLedger().erase(it);
            }
            // Handed to the quarantine rather than dropped. This host is the one holding
            // the dimension, and a player may be standing in it at this moment; the
            // engine keeps references it does not own, so releasing the last one here
            // would leave them pointing into freed memory.
            quarantine(std::move(released));
        }

        void forgetAllInstances()
        {
            std::map<int, ::OwnerPtr<::Dimension>> released;
            {
                std::lock_guard lock{instanceMutex()};
                released.swap(instanceLedger());
            }
            // Released while the mod is still loaded and the level is still standing,
            // rather than at process exit among the static destructors, where the order
            // against the level is not defined and a Dimension outliving it would run its
            // destructor against a level that is already gone.
            released.clear();
        }

        Dimension* getOrCreateByName(std::string const& rawName)
        {
            auto const name = engineNameOf(rawName);
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
            auto const id = engineDimensionId(rawName);
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

            // The plain lookup first, before any of the create paths. On this engine both
            // getOrCreateDimension overloads come back empty for a custom dimension, so
            // every call used to reach buildAndRegister and register a second instance
            // over the first: at startup that discarded the dimension the probe had just
            // built, and on the first teleport it destroyed the one the player was being
            // sent into, which faulted on a chunk worker thread seconds later.
            if (id)
            {
                Dimension* live = nullptr;
                switch (registeredInstance(mgr, *id, &live))
                {
                case Registered::Present:
                    return live;
                case Registered::Unknown:
                    return nullptr; // registeredInstance said why
                case Registered::Absent:
                    break;
                }
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
            if (auto* d = buildAndRegister(rawName, *id)) return d;

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

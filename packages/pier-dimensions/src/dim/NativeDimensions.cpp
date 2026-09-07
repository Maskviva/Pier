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

        /*
         * Unanswerable on 26.32, see the note above registerCustomDimension.
         *
         * DimensionManager::getDimensionId read NameIdStore and is inlined away, and
         * Util::NameIdStore is an empty class in the generated headers of both 26.20 and
         * 26.32, so its table cannot be read from here at all. Answering out of the host
         * ledger instead would make the drift check in CustomDimensionManager compare the
         * ledger against itself and pass on every boot, which is worse than not knowing.
         */
        std::optional<int> engineDimensionId(std::string const&) { return std::nullopt; }

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

        /*
         * Refused on 26.32: the engine no longer offers an id. The two entry points this
         * flow needed are inlined away with no symbol left. serverRegisterCustomDimension
         * allocated the id and wrote it into the save's NameIdStore, and getDimensionId
         * read that table back on the next boot. The pieces around them survive, so the
         * definition and the factory can still be registered, but nothing hands out an id
         * and nothing reports the one a save already holds. Allocating one here was
         * rejected: the number has to agree with what the engine persists, and a
         * disagreement renames a dimension a player has already built in.
         *
         * Every md_* slot that creates a dimension now reports failure, which is the
         * answer abi.h documents for a refused registration. No other slot is affected.
         */
        std::optional<int>
        registerCustomDimension(std::string const& name, int, int, GeneratorType)
        {
            static bool said = false;
            if (!said)
            {
                said = true;
                hostLogger().error(
                    "[dim] custom dimensions are unavailable on this engine: the id "
                    "allocation the registration needs is not reachable, so no dimension "
                    "can be created and the ones a save already holds cannot be found "
                    "again. Every other capability of the mod is unaffected"
                );
            }
            // Names the caller, since the once-per-process line above carries the
            // reason but not which registration hit it.
            hostLogger().error(
                "[dim] '{}' was not registered; a mod that needs it has to treat "
                "md_add_dimension returning -1 as the dimension not existing",
                name
            );
            return std::nullopt;
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

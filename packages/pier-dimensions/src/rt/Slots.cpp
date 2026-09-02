/**
 * pier-dimensions/rt/Slots.cpp: fills this package's capability into the ABI table, the
 * md_* family.
 *
 * ABI adaptation only. The logic lives in the implementation files, since deciding the
 * same thing in two places eventually diverges. Every function runs on the server
 * thread.
 *
 * All slots sit in one namespace. Each capability package fills the table itself
 * through a SlotPack, and this package is not compiled into the client target at all,
 * so those slots stay NULL and the SDK reports unsupported under the empty-slot rule.
 * No stub functions are needed for a client build.
 */
#include <cstdint>
#include <string>
#include <string_view>

#include "magic_enum.hpp"

#include "mc/world/level/GeneratorType.h"

#include "sdk/abi.h"

#include "pier/dimensions/base/simple_custom_dimension.h"
#include "pier/dimensions/dim/custom_dimension_config.h"
#include "pier/dimensions/dim/custom_dimension_manager.h"
#include "pier/dimensions/dim/dimension_rules.h"
#include "pier/dimensions/plot/plot_confine.h"
#include "pier/dimensions/plot/plot_dimension.h"
#include "pier/dimensions/plot/plot_layout.h"

#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::dimensions::rt
{
    /** The three-source name resolution in Bridge.cpp. */
    int idByName(std::string const& name);

    namespace
    {
        using pier::hostLogger;
        using pier::ps;
        using pier::sv;
        using pier::toString;

        /**
         * Creates a SimpleCustomDimension. Returns the allocated id, 3 or above, and -1
         * on failure.
         * generatorType carries the ::GeneratorType values verbatim: 1 Overworld, 2 Flat,
         * 3 Nether, 4 TheEnd, 5 Void. It is not a numbering of this package, and being
         * off by one gives a caller asking for superflat a nether. Legacy(0) and
         * Undefined(6) are refused rather than passed through, since neither has a branch
         * in SimpleCustomDimension::createGenerator and both fall into the default,
         * quietly building a void world.
         * Idempotent: the same name yields the same id, because CustomDimensionManager
         * reuses the persisted entry, so a caller registers unconditionally at startup
         * instead of probing. The entry lives in worlds/<levelName>/dimension_config.json
         * and follows the save rather than configs/, because re-registering allocates a
         * new id and invalidates the DimensionId in every player's save. */
        int32_t api_md_add_simple_dimension(PierStr name, uint32_t seed, int32_t generatorTypeInt)
        {
            PIER_API_GUARD_BEGIN
                std::string const dimName = toString(name);
                switch (static_cast<GeneratorType>(generatorTypeInt))
                {
                case GeneratorType::Overworld:
                case GeneratorType::Flat:
                case GeneratorType::Nether:
                case GeneratorType::TheEnd:
                case GeneratorType::Void:
                    break;
                default:
                    hostLogger().error(
                        "[dim] add_simple_dimension('{}') refused: generatorType={} is not a "
                        "supported ::GeneratorType, the accepted values are 1 Overworld, "
                        "2 Flat, 3 Nether, 4 TheEnd, 5 Void. The dimension was not created; "
                        "failing now beats building a world with the wrong generator, because "
                        "the generator name is written into dimensions.json and cannot be "
                        "changed afterwards",
                        dimName, generatorTypeInt
                    );
                    return -1;
                }
                auto genType = static_cast<GeneratorType>(generatorTypeInt);
                hostLogger().debug(
                    "[dim] add_simple_dimension('{}'): seed={} generatorType={}({})",
                    dimName, seed, magic_enum::enum_name(genType), generatorTypeInt
                );
                auto id = CustomDimensionManager::getInstance().addDimension<SimpleCustomDimension>(
                    dimName, seed, genType
                );
                return id.value();
            PIER_API_GUARD_END_VAL(-1)
        }

        /**
         * Creates a PlotDimension, a custom dimension whose chunk generator lays out the
         * plot grid of plots, roads and borders during generation.
         *
         * `layoutSnbt` is the SNBT of a CompoundTag, see `PlotLayout::fromSnbt`. The
         * values are clamped on that side, because an index from a caller can never be
         * trusted: it feeds into a fixed-size chunk buffer.
         *
         * Returns the allocated dimension id, 3 or above, and -1 on failure.
         */
        int32_t api_md_add_plot_dimension(PierStr name, uint32_t seed, PierStr layoutSnbt)
        {
            PIER_API_GUARD_BEGIN
                // PierStr is {ptr,len} and `ptr` is not terminated by \0. Passing
                // name.data() to a %s reads on to the next \0 that happens to be there,
                // which is undefined behavior and produces run-together log lines ending
                // in garbage. Everything goes through toString or sv first.
                std::string const dimName = toString(name);
                hostLogger().debug(
                    "add_plot_dimension: name='{}' seed={} layout={}",
                    dimName, seed, sv(layoutSnbt)
                );
                auto layout = PlotLayout::fromSnbt(toString(layoutSnbt));
                if (!layout)
                {
                    // The layout is persisted with the dimension forever, so a failed
                    // parse must not be substituted with defaults.
                    hostLogger().error(
                        "[dim] add_plot_dimension('{}') refused: the layout SNBT failed to parse, received: {}",
                        dimName, sv(layoutSnbt)
                    );
                    return -1;
                }
                auto id = CustomDimensionManager::getInstance().addDimension<PlotDimension>(
                    dimName, seed, *layout
                );
                return id.value();
            PIER_API_GUARD_END_VAL(-1)
        }

        /** Name to id, or -1 when it is not found. The three sources, and why
         *  VanillaDimensions::toString must not be touched, are covered in Bridge.cpp. */
        int32_t api_md_get_dimension_id(PierStr name)
        {
            PIER_API_GUARD_BEGIN
                return idByName(toString(name));
            PIER_API_GUARD_END_VAL(-1)
        }

        /* The three dimension rule entry points. Implemented in DimensionRules.cpp. */

        void api_md_set_dimension_rule(int32_t dimension, int32_t rule, bool allow)
        {
            PIER_API_GUARD_BEGIN
                setDimensionRule(dimension, rule, allow);
            PIER_API_GUARD_END_VOID
        }

        bool api_md_get_dimension_rule(int32_t dimension, int32_t rule, bool* outAllow)
        {
            PIER_API_GUARD_BEGIN
                return getDimensionRule(dimension, rule, outAllow);
            PIER_API_GUARD_END
        }

        void api_md_clear_dimension_rules(int32_t dimension)
        {
            PIER_API_GUARD_BEGIN
                clearDimensionRules(dimension);
            PIER_API_GUARD_END_VOID
        }

        /**
         * Lists every registered custom dimension.
         *
         * The source is `dimensionList` in `CustomDimensionConfig`, the in-memory mirror
         * of `dimensions.json`. The config is read deliberately rather than the engine
         * dimension table: the `dimId` in the config is the number the engine allocated
         * once and then persisted, so it is the same after a restart, while the engine
         * table also holds the three vanilla dimensions and a shifting `Undefined()`.
         *
         * One JSON object is emitted at a time and the caller assembles the array.
         * Emitting per entry keeps one malformed sNbt from ruining anything beyond its
         * own line.
         */
        void api_md_list_dimensions(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return;
                auto const& cfg = CustomDimensionConfig::getConfig();
                for (auto const& [name, info] : cfg.dimensionList)
                {
                    // Both the name and the sNbt may contain quotes and backslashes, and
                    // without escaping one malformed record ruins the whole JSON array.
                    std::string line = "{\"name\":\"" + pier::snbtEscape(name)
                        + "\",\"dim\":" + std::to_string(info.dimId)
                        + ",\"snbt\":\"" + pier::snbtEscape(info.sNbt) + "\"}";
                    sink(ctx, ps(line));
                }
            PIER_API_GUARD_END_VOID
        }

        /* Plot boundary confinement. The logic lives in PlotConfine.cpp. */

        void api_md_set_plot_grid(int32_t dimension, int32_t plotSize, int32_t roadWidth)
        {
            PIER_API_GUARD_BEGIN
                setPlotGrid(dimension, plotSize, roadWidth);
            PIER_API_GUARD_END_VOID
        }

        void api_md_clear_plot_grid(int32_t dimension)
        {
            PIER_API_GUARD_BEGIN
                clearPlotGrid(dimension);
            PIER_API_GUARD_END_VOID
        }

        void api_md_set_plot_merges(int32_t dimension, int32_t const* entries, int32_t count)
        {
            PIER_API_GUARD_BEGIN
                // An empty table is valid input, meaning this world has no merges at
                // all, but count above zero with a null pointer is a caller bug and must
                // not be used for pointer arithmetic.
                if (entries == nullptr) count = 0;
                setPlotMerges(dimension, entries, count);
            PIER_API_GUARD_END_VOID
        }

        /**
         * Whether this capability is available in this build.
         *
         * Always true, because this function exists only when the package was compiled
         * in. Without it the slot is NULL, the SDK reports unsupported under the
         * empty-slot rule and never reaches here, so there is no path that returns false.
         * The slot exists to give the SDK a way to ask without checking for null itself.
         */
        bool api_md_is_available()
        {
            PIER_API_GUARD_BEGIN
                return true;
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.md_add_simple_dimension = &api_md_add_simple_dimension;
            api.md_add_plot_dimension = &api_md_add_plot_dimension;
            api.md_get_dimension_id = &api_md_get_dimension_id;
            api.md_set_dimension_rule = &api_md_set_dimension_rule;
            api.md_get_dimension_rule = &api_md_get_dimension_rule;
            api.md_clear_dimension_rules = &api_md_clear_dimension_rules;
            api.md_list_dimensions = &api_md_list_dimensions;
            api.md_set_plot_grid = &api_md_set_plot_grid;
            api.md_clear_plot_grid = &api_md_clear_plot_grid;
            api.md_set_plot_merges = &api_md_set_plot_merges;
            api.md_is_available = &api_md_is_available;
        }

        spi::SlotPackReg reg{{"dimensions", &fill}};
    } // namespace
} // namespace pier::dimensions::rt

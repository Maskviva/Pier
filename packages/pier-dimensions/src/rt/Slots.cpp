/**
 * pier-dimensions/rt/Slots.cpp: fills this package's capability into the ABI table.
 *
 * ABI adaptation only. One live entry for creating dimensions, md_add_dimension. The four
 * retired slots keep their place in the table (contract 2.2) and are filled with stubs that
 * log once and return the failure value, so a mod built against the old surface fails
 * visibly instead of silently doing something else. Every function runs on the server
 * thread.
 */
#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/biome/registry/BiomeRegistry.h"

#include "ll/api/service/Bedrock.h"

#include "sdk/abi.h"

#include "pier/dimensions/dim/custom_dimension_config.h"
#include "pier/dimensions/dim/custom_dimension_manager.h"
#include "pier/dimensions/dim/dimension_rules.h"
#include "pier/dimensions/gen/cell_confine.h"
#include "pier/dimensions/spec/dimension_spec.h"
#include "pier/dimensions/spec/spec_dimension.h"

#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::dimensions::rt
{
    int idByName(std::string const& name);

    namespace
    {
        using pier::hostLogger;
        using pier::ps;
        using pier::toString;
        using spec::DimensionSpec;

        /** Every biome named by the spec has to exist now. Refusing at registration is the
         *  only moment it costs nothing: afterwards the spec is persisted with the dimension. */
        bool biomesExist(std::string const& dimName, DimensionSpec const& s)
        {
            auto level = ll::service::getLevel();
            if (!level) return true;
            auto& reg = level->getBiomeRegistry();
            auto check = [&](std::string const& b)
            {
                if (reg.lookupByName(b)) return true;
                hostLogger().error("[dim] add_dimension('{}') refused: biome '{}' is not in the registry. Custom biomes come from a behavior pack loaded before mods; check the pack and the spelling", dimName, b);
                return false;
            };
            if (auto const* l = std::get_if<spec::Layers>(&s.terrain)) return check(l->biome);
            if (auto const* n = std::get_if<spec::Noise>(&s.terrain))
            {
                for (auto const& b : n->biomes)
                    if (!check(b.biome)) return false;
            }
            return true;
        }

        int32_t api_md_add_dimension(PierStr name, PierStr specSnbt)
        {
            PIER_API_GUARD_BEGIN
                std::string const dimName = toString(name);
                std::string const raw = toString(specSnbt);
                auto [s, problems] = DimensionSpec::fromSnbt(raw);
                for (auto const& p : problems) hostLogger().warn("[dim] add_dimension('{}'): {}", dimName, p);
                if (!s)
                {
                    hostLogger().error("[dim] add_dimension('{}') refused: the spec could not be read (see the lines above). The dimension was not created; a wrong spec persists with the dimension and terrain generated from it cannot be regenerated", dimName);
                    return -1;
                }
                if (!biomesExist(dimName, *s)) return -1;
                auto id = CustomDimensionManager::getInstance().addDimension<SpecDimension>(dimName, raw);
                // The grid is one definition for terrain and confinement. Registering it
                // here, from the spec, is what keeps the two from ever disagreeing.
                if (auto const* l = std::get_if<spec::Layers>(&s->terrain); l && l->grid && l->grid->confine)
                    setCellGrid(id.mValue, l->grid->cell, l->grid->gap);
                return id.mValue;
            PIER_API_GUARD_END_VAL(-1)
        }

        bool api_md_retire_dimension(PierStr name)
        {
            PIER_API_GUARD_BEGIN
                std::string const dimName = toString(name);
                auto const id = CustomDimensionManager::getInstance().retireDimension(dimName);
                if (!id) return false;
                // The cell grid goes with it. Leaving it behind keeps confining actors in
                // a dimension the host no longer registers.
                if (*id >= 0) clearCellGrid(*id);
                return true;
            PIER_API_GUARD_END_VAL(false)
        }

        /** One line per retired slot per process: enough to find the caller, not enough to
         *  flood a log when a loop keeps calling it. */
        void retired(std::atomic<bool>& said, char const* slot, char const* use)
        {
            if (said.exchange(true)) return;
            hostLogger().error("[dim] {} is retired since 26.20.3 and no longer served; use {}", slot, use);
        }
        std::atomic<bool> gSaidSimple{false}, gSaidPlot{false}, gSaidGrid{false}, gSaidClearGrid{false};

        int32_t api_md_add_simple_dimension(PierStr, uint32_t, int32_t)
        {
            retired(gSaidSimple, "md_add_simple_dimension", "md_add_dimension with terrain:{kind:\"native\"}");
            return -1;
        }
        int32_t api_md_add_plot_dimension(PierStr, uint32_t, PierStr)
        {
            retired(gSaidPlot, "md_add_plot_dimension", "md_add_dimension with terrain:{kind:\"layers\", grid:{...}}");
            return -1;
        }
        void api_md_set_plot_grid(int32_t, int32_t, int32_t)
        {
            retired(gSaidGrid, "md_set_plot_grid", "terrain.grid.confine:true in the dimension spec");
        }
        void api_md_clear_plot_grid(int32_t)
        {
            retired(gSaidClearGrid, "md_clear_plot_grid", "nothing: the grid is withdrawn with the dimension");
        }

        int32_t api_md_get_dimension_id(PierStr name)
        {
            PIER_API_GUARD_BEGIN
                return idByName(toString(name));
            PIER_API_GUARD_END_VAL(-1)
        }

        bool api_md_is_available()
        {
            PIER_API_GUARD_BEGIN
                return true;
            PIER_API_GUARD_END
        }

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
            PIER_API_GUARD_END_VAL(false)
        }
        void api_md_clear_dimension_rules(int32_t dimension)
        {
            PIER_API_GUARD_BEGIN
                clearDimensionRules(dimension);
            PIER_API_GUARD_END_VOID
        }
        /** Live: which cells are merged is runtime data of the mod that owns the cells.
         *  The name is history, the mechanism is cell merging. */
        void api_md_set_plot_merges(int32_t dimension, int32_t const* entries, int32_t count)
        {
            PIER_API_GUARD_BEGIN
                // An empty table is valid input, meaning this world has no merges at all,
                // but count above zero with a null pointer is a caller bug and must not be
                // used for pointer arithmetic.
                if (entries == nullptr) count = 0;
                setCellMerges(dimension, entries, count);
            PIER_API_GUARD_END_VOID
        }
        void api_md_list_dimensions(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return;
                for (auto const& [name, info] : CustomDimensionConfig::getConfig().dimensionList)
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

        void fill(PierApi& api)
        {
            api.md_add_dimension = &api_md_add_dimension;
            api.md_retire_dimension = &api_md_retire_dimension;
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

/**
 * pier-dimensions/rt/Slots.cpp: fills this package's capability into the ABI table.
 *
 * ABI adaptation only. Two entries create dimensions: md_add_dimension for a native
 * terrain and md_add_dimension_pack for a terrain pack, which checks three sources
 * against each other, the spec's terrain kind, the config's type and the binary's
 * magic, before anything is stored. md_pack_inspect answers what a pack asks for
 * without registering anything. Every function runs on the server thread.
 */
#include <atomic>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/CompoundTagVariant.h"
#include "mc/deps/nbt/Tag.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/biome/registry/BiomeRegistry.h"

#include "ll/api/service/Bedrock.h"

#include "sdk/abi.h"

#include "pier/dimensions/base/native_dimensions.h"
#include "pier/dimensions/dim/custom_dimension_config.h"
#include "pier/dimensions/dim/custom_dimension_manager.h"
#include "pier/dimensions/dim/dimension_rules.h"
#include "pier/dimensions/gen/cell_confine.h"
#include "pier/dimensions/gen/supplied_generator.h"
#include "pier/dimensions/pack/pack_inspect.h"
#include "pier/dimensions/pack/pack_locate.h"
#include "pier/dimensions/pack/template_pack.h"
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
        namespace pk = pier::dimensions::pack;

        bool biomeExists(std::string const& dimName, std::string const& biome)
        {
            auto level = ll::service::getLevel();
            if (!level) return true;
            if (level->getBiomeRegistry().lookupByName(biome)) return true;
            hostLogger().error("[dim] '{}' refused: biome '{}' is not in the registry. Custom biomes come from a behavior pack loaded before mods; check the pack and the spelling", dimName, biome);
            return false;
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
                if (s->isPack())
                {
                    hostLogger().error("[dim] add_dimension('{}') refused: a {} terrain is registered through md_add_dimension_pack, which verifies the pack before the spec is stored", dimName, std::get<spec::Pack>(s->terrain).kind);
                    return -1;
                }
                if (s->isSupplied())
                {
                    // A payload with no terrain section belongs to the other slot. Taking
                    // it here would register a dimension nothing fills, and the world
                    // would come up void with the registration having reported success.
                    hostLogger().error("[dim] add_dimension('{}') refused: the payload has no terrain section, which means the terrain comes from the calling mod; use md_add_dimension_generated", dimName);
                    return -1;
                }
                // get_if and not get: the terrain has three alternatives and this slot
                // serves two of them, so a get here throws for the third rather than
                // falling through. A layers terrain reached this line and the throw came
                // out of the registration as a refusal with no reason attached.
                if (auto const* n = std::get_if<spec::Native>(&s->terrain))
                {
                    if (n->generator == GeneratorType::Void && !biomeExists(dimName, n->biome)) return -1;
                }
                else if (auto const* l = std::get_if<spec::Layers>(&s->terrain))
                {
                    // Checked here for the same reason the pack path checks it before
                    // storing: TemplateGenerator treats a biome it cannot find as the
                    // registry having changed under it, and says so, which is the wrong
                    // thing to tell someone who simply mistyped it in a recipe.
                    if (!biomeExists(dimName, l->biome)) return -1;
                }
                // Named here rather than left to the outer guard. That one catches every
                // slot, so all it can say is the exception type, and a registration that
                // fails with `bad variant access` and nothing else names neither the
                // dimension nor the recipe that produced it.
                try
                {
                    auto id = CustomDimensionManager::getInstance().addDimension<SpecDimension>(dimName, raw);
                    return id.mValue;
                }
                catch (std::exception const& e)
                {
                    hostLogger().error("[dim] add_dimension('{}') threw: {}. The spec was: {}", dimName, e.what(), raw);
                    return -1;
                }
            PIER_API_GUARD_END_VAL(-1)
        }

        int32_t code(pk::PackStatus s) { return static_cast<int32_t>(s); }

        int32_t code(pk::MountFailure f)
        {
            switch (f)
            {
            case pk::MountFailure::Params: return code(pk::PackStatus::Params);
            case pk::MountFailure::Constraint: return code(pk::PackStatus::Constraint);
            case pk::MountFailure::Height: return code(pk::PackStatus::Height);
            default: return code(pk::PackStatus::Host);
            }
        }

        std::string normalizedPath(std::string p)
        {
            for (auto& c : p)
                if (c == '\\') c = '/';
            return p;
        }

        /** The stored spec of a name, when the host already knows it. */
        std::optional<DimensionSpec> storedSpecOf(std::string const& dimName, std::vector<std::string>& problems)
        {
            auto const& list = CustomDimensionConfig::getConfig().dimensionList;
            auto it = list.find(dimName);
            if (it == list.end()) return std::nullopt;
            auto [s, p] = DimensionSpec::fromSnbt(it->second.sNbt);
            for (auto& line : p) problems.push_back("stored spec: " + line);
            return s;
        }

        int32_t api_md_add_dimension_pack(PierStr name, PierStr configPath, PierStr specSnbt)
        {
            PIER_API_GUARD_BEGIN
                std::string const dimName = toString(name);
                std::string const configRel = normalizedPath(toString(configPath));
                std::vector<std::string> problems;
                auto fail = [&](int32_t rc)
                {
                    for (auto const& p : problems) hostLogger().error("[dim] add_dimension_pack('{}', '{}'): {}", dimName, configRel, p);
                    hostLogger().error("[dim] add_dimension_pack('{}') refused with {}; nothing was stored", dimName, rc);
                    return rc;
                };
                auto [s, specProblems] = DimensionSpec::fromSnbt(toString(specSnbt));
                for (auto const& p : specProblems) hostLogger().warn("[dim] add_dimension_pack('{}'): {}", dimName, p);
                if (!s)
                {
                    problems.push_back("the spec could not be read");
                    return fail(code(pk::PackStatus::Spec));
                }
                if (!s->isPack())
                {
                    problems.push_back("the spec's terrain is native; md_add_dimension serves that");
                    return fail(code(pk::PackStatus::Spec));
                }
                auto given = std::get<spec::Pack>(s->terrain);
                if (given.path != configRel && !given.path.empty())
                    hostLogger().warn("[dim] add_dimension_pack('{}'): the spec names pack '{}' and the call names '{}'; the call wins", dimName, given.path, configRel);

                pk::PackStatus status = pk::PackStatus::Ok;
                auto loc = pk::locatePack(configRel, "", status, problems);
                if (!loc) return fail(code(status));
                if (loc->kind != given.kind)
                {
                    problems.push_back("the spec says " + given.kind + " but the pack config says " + loc->kind);
                    return fail(code(pk::PackStatus::KindMismatch));
                }
                // A name the host already knows is served from its stored spec; the pack
                // on disk has to be the one it was created with, and the caller's
                // parameters are the stored ones or they are ignored with a line.
                auto stored = storedSpecOf(dimName, problems);
                spec::Pack effective = given;
                if (stored)
                {
                    if (!stored->isPack())
                    {
                        problems.push_back("the name already exists with a native terrain; retire it first");
                        return fail(code(pk::PackStatus::StoredMismatch));
                    }
                    effective = std::get<spec::Pack>(stored->terrain);
                    if (effective.sha256 != loc->sha256)
                    {
                        problems.push_back("this world was created from a binary with hash " + effective.sha256 + " and the pack config now names " + loc->sha256 + "; terrain from a different binary cannot continue this world");
                        return fail(code(pk::PackStatus::StoredMismatch));
                    }
                    if (given.params != effective.params || given.roles != effective.roles)
                        hostLogger().warn("[dim] add_dimension_pack('{}'): parameters or roles differ from the stored ones; the stored ones apply", dimName);
                    s->minY = stored->minY;
                    s->maxY = stored->maxY;
                }
                effective.kind = loc->kind;
                effective.path = configRel;
                effective.sha256 = loc->sha256;

                std::optional<pk::MountedTemplate> mounted;
                if (loc->kind == "template")
                {
                    auto tpl = pk::PackCache::instance().templateAt(configRel, loc->sha256, "", status, problems);
                    if (!tpl) return fail(code(status));
                    if (!biomeExists(dimName, tpl->biome)) return fail(code(pk::PackStatus::Host));
                    pk::MountFailure why = pk::MountFailure::None;
                    mounted = pk::mountTemplate(*tpl, effective.params, effective.roles, s->minY, s->maxY, problems, &why);
                    if (!mounted) return fail(code(why));
                    // Every bound value is stored, derived ones excluded, so the persisted
                    // spec does not depend on the pack's defaults staying what they are.
                    effective.params.clear();
                    for (std::size_t i = 0; i < tpl->params.size(); ++i)
                        if (tpl->params[i].kind != pk::ParamKind::Derived) effective.params[tpl->params[i].name] = mounted->values[i];
                }
                else
                {
                    auto vol = pk::PackCache::instance().volumeAt(configRel, loc->sha256, "", status, problems);
                    if (!vol) return fail(code(status));
                    // Every missing name in one pass, not the first. A pack carries a
                    // handful, and refusing on the first turns fixing them into one
                    // rebuild and one restart each.
                    bool allPresent = true;
                    for (auto const& b : vol->biomeNames) allPresent &= biomeExists(dimName, b);
                    if (!allPresent) return fail(code(pk::PackStatus::Host));
                    if (!effective.params.empty() || !effective.roles.empty())
                        hostLogger().warn("[dim] add_dimension_pack('{}'): a volume pack takes no parameters or roles; the given ones are dropped", dimName);
                    effective.params.clear();
                    effective.roles.clear();
                    // A volume pack fixes its height; the spec's is replaced, not clamped
                    // against, since the pack was built for exactly this range.
                    if (!stored && (s->minY != vol->minY() || s->maxY != vol->maxY()))
                        hostLogger().warn("[dim] add_dimension_pack('{}'): the height is taken from the pack, [{}, {})", dimName, vol->minY(), vol->maxY());
                    s->minY = vol->minY();
                    s->maxY = vol->maxY();
                }

                auto tag = CompoundTag::fromSnbt(toString(specSnbt));
                if (!tag)
                {
                    problems.push_back("the spec that parsed a moment ago does not parse now");
                    return fail(code(pk::PackStatus::Spec));
                }
                tag->remove("terrain");
                tag->putCompound("terrain", spec::packTerrainTag(effective));
                CompoundTag height;
                height.putInt("min", s->minY);
                height.putInt("max", s->maxY);
                tag->remove("height");
                tag->putCompound("height", std::move(height));
                auto id = CustomDimensionManager::getInstance().addDimension<SpecDimension>(dimName, tag->toSnbt(SnbtFormat::Minimize));
                if (id.mValue < 0)
                {
                    problems.push_back("the dimension manager refused the registration (see the lines above)");
                    return fail(code(pk::PackStatus::Host));
                }
                // The confinement grid is the pack's own geometry, registered from the
                // same mount that produced the terrain, which is what keeps the two from
                // ever disagreeing about where a cell ends.
                if (mounted && mounted->confine) setCellGrid(id.mValue, mounted->confine->first, mounted->confine->second);
                return id.mValue;
            PIER_API_GUARD_END_VAL(PIER_PACK_HOST)
        }

        int32_t api_md_pack_inspect(PierStr configPath, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                std::string const configRel = normalizedPath(toString(configPath));
                std::vector<std::string> problems;
                pk::PackStatus status = pk::PackStatus::Ok;
                auto emit = [&](std::string const& json)
                {
                    if (sink) sink(ctx, ps(json));
                };
                auto loc = pk::locatePack(configRel, "", status, problems);
                if (!loc)
                {
                    emit(pk::refusalJson(status, problems));
                    return code(status);
                }
                if (loc->kind == "template")
                {
                    auto tpl = pk::PackCache::instance().templateAt(configRel, loc->sha256, "", status, problems);
                    if (!tpl)
                    {
                        emit(pk::refusalJson(status, problems));
                        return code(status);
                    }
                    emit(pk::templateJson(*tpl, configRel));
                    return 0;
                }
                auto vol = pk::PackCache::instance().volumeAt(configRel, loc->sha256, "", status, problems);
                if (!vol)
                {
                    emit(pk::refusalJson(status, problems));
                    return code(status);
                }
                emit(pk::volumeJson(*vol, configRel));
                return 0;
            PIER_API_GUARD_END_VAL(PIER_PACK_HOST)
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

        int32_t api_md_get_dimension_id(PierStr name)
        {
            PIER_API_GUARD_BEGIN
                return idByName(toString(name));
            PIER_API_GUARD_END_VAL(-1)
        }

        bool api_md_is_available()
        {
            PIER_API_GUARD_BEGIN
                // Not `true`, and not `Level is open` either: the question that decides
                // whether any dimension can be created is whether the definition group
                // can be read, because that is where the id comes from. Cached after the
                // first answer that Level was open for, since the probe walks the group
                // and this slot is on the path of every dimension call.
                static std::optional<bool> cached;
                if (cached) return *cached;
                if (!native::available()) return false;
                cached = native::definitionGroupReadable();
                return *cached;
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

        std::int32_t api_md_add_dimension_generated(PierStr name, PierStr specSnbt, PierStr materialPalette,
                                                    PierStr biomePalette, PierGenerateChunkFn fn, void* user)
        {
            PIER_API_GUARD_BEGIN
                auto const dimName = toString(name);
                std::string const raw = toString(specSnbt);
                auto [s, problems] = DimensionSpec::fromSnbt(raw);
                for (auto const& p : problems) hostLogger().warn("[dim] add_dimension_generated('{}'): {}", dimName, p);
                if (!s) return -1;
                if (!s->isSupplied())
                {
                    hostLogger().error(
                        "[dim] add_dimension_generated('{}') refused: the spec carries a terrain section, and this "
                        "slot does not read one. Accepting a field that is ignored is how a spec comes to describe "
                        "a world nobody generates",
                        dimName
                    );
                    return -1;
                }
                std::vector<std::string> palette;
                auto terrain = resolveSuppliedTerrain(dimName, toString(materialPalette), toString(biomePalette),
                                                      fn, user, dimName, palette);
                for (auto const& p : palette) hostLogger().error("[dim] add_dimension_generated('{}'): {}", dimName, p);
                if (!terrain) return -1;

                // Remembered before the registration and dropped if it fails: a
                // generator built between the two would find nothing and the world
                // would come up void with no line saying why.
                rememberSuppliedTerrain(dimName, terrain);
                try
                {
                    auto id = CustomDimensionManager::getInstance().addDimension<SpecDimension>(dimName, raw);
                    return id.mValue;
                }
                catch (std::exception const& e)
                {
                    forgetSuppliedTerrain(dimName);
                    hostLogger().error("[dim] add_dimension_generated('{}') threw: {}. The spec was: {}", dimName, e.what(), raw);
                    return -1;
                }
            PIER_API_GUARD_END_VAL(-1)
        }

        bool api_md_set_dimension_cells(std::int32_t dimId, std::int32_t cell, std::int32_t gap)
        {
            PIER_API_GUARD_BEGIN
                // Checked first: setCellGrid takes any number, so an id this host never
                // registered would install geometry that nothing ever asks about, and
                // the caller would be told it worked.
                bool known = false;
                for (auto const& [name, info] : CustomDimensionConfig::getConfig().dimensionList)
                {
                    (void)name;
                    if (info.dimId == dimId) { known = true; break; }
                }
                if (!known) return false;
                setCellGrid(dimId, cell, gap);
                return true;
            PIER_API_GUARD_END_VAL(false)
        }

        void fill(PierApi& api)
        {
            api.md_add_dimension_generated = &api_md_add_dimension_generated;
            api.md_set_dimension_cells = &api_md_set_dimension_cells;
            api.md_add_dimension = &api_md_add_dimension;
            api.md_add_dimension_pack = &api_md_add_dimension_pack;
            api.md_pack_inspect = &api_md_pack_inspect;
            api.md_retire_dimension = &api_md_retire_dimension;
            api.md_get_dimension_id = &api_md_get_dimension_id;
            api.md_set_dimension_rule = &api_md_set_dimension_rule;
            api.md_get_dimension_rule = &api_md_get_dimension_rule;
            api.md_clear_dimension_rules = &api_md_clear_dimension_rules;
            api.md_list_dimensions = &api_md_list_dimensions;
            api.md_set_plot_merges = &api_md_set_plot_merges;
            api.md_is_available = &api_md_is_available;
        }

        spi::SlotPackReg reg{{"dimensions", &fill}};
    } // namespace
} // namespace pier::dimensions::rt

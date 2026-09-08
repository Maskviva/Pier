/** template_pack.h: a PIERTPL pack decoded into owned tables, and the result of
 * mounting it with bound parameters and roles.
 * TemplatePack is the file after validation, with every index range checked, and it
 * carries no parameter values. MountedTemplate is one dimension's instance of it: the
 * expression values, the zone of every offset along each axis, one material column per
 * combined zone, the layers that are the same in every zone, the resolved shape
 * parameters and bounding boxes, and the picks. Materials are block names indexed by a
 * small integer; the generator turns them into Block pointers once. Everything here is
 * engine-free so the same code runs under the equivalence tests. */
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "pier/dimensions/pack/pack_format.h"
#include "pier/dimensions/pack/pack_reader.h"

namespace pier::dimensions::pack
{
    struct TplParam
    {
        std::string name;
        ParamKind kind = ParamKind::Free;
        std::int32_t def = 0;
        std::int32_t min = 0;
        std::int32_t max = 0;
        std::int32_t step = 1;
        std::uint32_t aux = kNone;
    };

    struct TplRole
    {
        std::string name;
        std::string defaultBlock;
    };

    struct TplVoxel
    {
        std::uint32_t sx = 0, sy = 0, sz = 0;
        std::vector<std::string> palette;
        std::vector<std::uint16_t> cells;
        std::vector<std::uint16_t> liquid;
    };

    struct TplPick
    {
        Pick pick{};
        std::vector<PickRoot> roots;
    };

    struct TemplatePack
    {
        Sha256 fileHash{};
        std::string sourceName;
        std::string biome;
        std::int32_t heightMin = 0;
        std::int32_t heightMax = 0;
        bool heightFixed = false;
        std::vector<TplParam> params;
        std::vector<std::vector<std::int32_t>> choiceLists;
        std::vector<ExprNode> expr;
        std::vector<TplRole> roles;
        std::vector<std::string> zoneNames;
        std::uint32_t periodXExpr = kNone;
        std::uint32_t periodZExpr = kNone;
        std::vector<Span> spansX;
        std::vector<Span> spansZ;
        std::vector<std::uint32_t> combine;
        std::vector<Stack> stacks;
        std::vector<Constraint> constraints;
        std::vector<std::string> constraintMessages;
        std::vector<ShapeNode> shapes;
        std::vector<std::vector<ChooseEntry>> chooseLists;
        std::vector<TplPick> picks;
        std::vector<TplVoxel> voxels;
        std::optional<Confine> confine;

        [[nodiscard]] std::size_t zoneCount() const { return zoneNames.size(); }
        [[nodiscard]] std::optional<std::size_t> paramIndex(std::string const& name) const;
    };

    /** Decodes every section and validates every index; nullopt with the reasons in
     *  problems. The file must be a template pack. */
    std::optional<TemplatePack> decodeTemplate(PackFile const& file, std::vector<std::string>& problems);

    struct IBox
    {
        std::int32_t x0 = 0, y0 = 0, z0 = 0, x1 = 0, y1 = 0, z1 = 0;
        [[nodiscard]] bool empty() const { return x0 >= x1 || y0 >= y1 || z0 >= z1; }
    };

    struct MountedPick
    {
        std::uint32_t salt = 0;
        std::int32_t x0 = 0, z0 = 0, w = 0, d = 0, anchorY = 0;
        std::vector<PickRoot> roots;
    };

    /** Material 0 is air. A role maps to one material, a voxel palette entry to one
     *  material, and the material table holds the block names. */
    struct MountedTemplate
    {
        TemplatePack const* pack = nullptr;
        std::vector<std::int64_t> values;
        std::vector<std::int64_t> vals;
        std::int32_t minY = 0;
        std::int32_t maxY = 0;
        std::int32_t periodX = 1;
        std::int32_t periodZ = 1;
        std::vector<std::uint32_t> zoneOfX;
        std::vector<std::uint32_t> zoneOfZ;
        std::vector<std::string> materials;
        std::vector<std::uint16_t> roleMaterial;
        std::vector<std::vector<std::uint16_t>> voxelMaterial;
        std::vector<std::vector<std::uint16_t>> zoneColumns;
        std::vector<std::uint8_t> staticLayer;
        std::vector<std::array<std::int64_t, 3>> shapeParams;
        std::vector<std::optional<IBox>> shapeBox;
        std::vector<MountedPick> picks;
        /** The confinement geometry when the pack asks for it: cell and gap. */
        std::optional<std::pair<std::int32_t, std::int32_t>> confine;

        [[nodiscard]] std::int32_t height() const { return maxY - minY; }
    };

    /** Which step of a mount refused: 0 none, 1 parameters or roles, 2 a constraint or
     *  the layout, 3 the height. Slots.cpp maps these to PIER_PACK_* codes. */
    enum class MountFailure : int
    {
        None = 0,
        Params = 1,
        Constraint = 2,
        Height = 3,
    };

    /** Binds params and roles, evaluates the table, checks the constraints and the
     *  height, and expands the layout. A refusal returns nullopt and every reason. The
     *  given height is the dimension's; a fixed-height pack requires the same values. */
    std::optional<MountedTemplate> mountTemplate(TemplatePack const& pack,
                                                 std::map<std::string, std::int64_t> const& params,
                                                 std::map<std::string, std::string> const& roles,
                                                 std::int32_t minY, std::int32_t maxY,
                                                 std::vector<std::string>& problems,
                                                 MountFailure* failure = nullptr);

    /** Writes the materials of one chunk into out, laid out (x * 16 + z) * height + y,
     *  x and z 0..15 and y 0..height-1 from minY. The zone columns first, then every
     *  pick whose turned root reaches into the chunk. */
    void generateTemplateChunk(MountedTemplate const& m, std::int32_t chunkX, std::int32_t chunkZ,
                               std::vector<std::uint16_t>& out);

    /** A deterministic hash of a cell and a salt; MurmurHash3 fmix32 over the mixed
     *  coordinates, the same as the tool. */
    std::uint32_t cellHash(std::int32_t cx, std::int32_t cz, std::uint32_t salt);
} // namespace pier::dimensions::pack

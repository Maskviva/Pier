/** pack_format.h: the byte layout of the two terrain pack files, PIERTPL and PIERVOL.
 * This header is the only description of the format on the C++ side and
 * tools/pier-pack/pierpack/format.py mirrors it field by field; the layout test there
 * compares struct sizes against the constants below. Every integer is little-endian,
 * every section starts on an 8-byte boundary, and every offset inside a section is
 * relative to the start of that section. A node array is topologically sorted so that
 * each operand index is smaller than the index of the node using it; that single
 * invariant is what the reader checks, and it rules out cycles and forward references
 * in one pass. String fields hold an index into the STRS table, never a byte offset.
 * Nothing here is read on a chunk thread: a pack is decoded once when a dimension is
 * mounted and the generators work from the decoded form. */
#pragma once

#include <cstddef>
#include <cstdint>

namespace pier::dimensions::pack
{
    //  Container

    inline constexpr std::size_t kMagicSize = 8;
    inline constexpr char kMagicTemplate[kMagicSize + 1] = "PIERTPL";
    inline constexpr char kMagicVolume[kMagicSize + 1] = "PIERVOL";
    inline constexpr std::uint32_t kFormatVersion = 1;
    inline constexpr std::size_t kHeaderSize = 32;
    inline constexpr std::size_t kSectionEntrySize = 56;
    inline constexpr std::size_t kSectionAlign = 8;
    inline constexpr std::uint32_t kNone = 0xFFFFFFFFu;

    /** Little-endian four-character section tag. */
    constexpr std::uint32_t fourcc(char a, char b, char c, char d)
    {
        return static_cast<std::uint32_t>(static_cast<unsigned char>(a))
            | (static_cast<std::uint32_t>(static_cast<unsigned char>(b)) << 8)
            | (static_cast<std::uint32_t>(static_cast<unsigned char>(c)) << 16)
            | (static_cast<std::uint32_t>(static_cast<unsigned char>(d)) << 24);
    }

    struct Header
    {
        char magic[kMagicSize];
        std::uint32_t formatVersion;
        std::uint32_t flags;
        std::uint32_t sectionCount;
        std::uint32_t headerSize;
        std::uint64_t totalSize;
    };
    static_assert(sizeof(Header) == kHeaderSize);

    struct SectionEntry
    {
        std::uint32_t type;
        std::uint32_t version;
        std::uint64_t offset;
        std::uint64_t length;
        std::uint8_t sha256[32];
    };
    static_assert(sizeof(SectionEntry) == kSectionEntrySize);

    //  Sections shared by both packs

    inline constexpr std::uint32_t kSecStrings = fourcc('S', 'T', 'R', 'S');
    inline constexpr std::uint32_t kSecInfo = fourcc('I', 'N', 'F', 'O');

    /** STRS: u32 count, then count pairs, then the bytes. Index 0 is always the empty
     *  string. */
    struct StringRef
    {
        std::uint32_t offset;
        std::uint32_t length;
    };
    static_assert(sizeof(StringRef) == 8);

    inline constexpr std::uint32_t kInfoHeightFixed = 1u << 0;

    /** INFO: one entry. biomeStr is the biome the template's fixed biome source uses and
     *  is empty for a volume pack, which places biomes itself. */
    struct Info
    {
        std::uint32_t toolVersionStr;
        std::uint32_t builtAtStr;
        std::uint32_t sourceNameStr;
        std::int32_t heightMin;
        std::int32_t heightMax;
        std::uint32_t flags;
        std::uint32_t biomeStr;
        std::uint32_t reserved;
    };
    static_assert(sizeof(Info) == 32);

    //  PIERTPL sections

    inline constexpr std::uint32_t kSecParams = fourcc('P', 'A', 'R', 'M');
    inline constexpr std::uint32_t kSecChoices = fourcc('P', 'C', 'H', 'C');
    inline constexpr std::uint32_t kSecExpr = fourcc('E', 'X', 'P', 'R');
    inline constexpr std::uint32_t kSecRoles = fourcc('R', 'O', 'L', 'E');
    inline constexpr std::uint32_t kSecZones = fourcc('Z', 'O', 'N', 'E');
    inline constexpr std::uint32_t kSecStacks = fourcc('S', 'T', 'A', 'K');
    inline constexpr std::uint32_t kSecConstraints = fourcc('C', 'N', 'S', 'T');
    inline constexpr std::uint32_t kSecShapes = fourcc('S', 'H', 'A', 'P');
    inline constexpr std::uint32_t kSecPick = fourcc('P', 'I', 'C', 'K');
    inline constexpr std::uint32_t kSecVoxels = fourcc('V', 'O', 'X', 'L');
    inline constexpr std::uint32_t kSecConfine = fourcc('C', 'O', 'N', 'F');

    enum class ParamKind : std::uint16_t
    {
        Free = 0,
        Fixed = 1,
        Choice = 2,
        Derived = 3,
    };

    /** PARM: u32 count, then count entries. A FREE parameter takes any value in
     *  [min, max] that is def + k*step; FIXED is always def; CHOICE takes a value out of
     *  PCHC list aux; DERIVED is EXPR node aux and is never supplied by a caller. */
    struct Param
    {
        std::uint32_t nameStr;
        std::uint16_t kind;
        std::uint16_t flags;
        std::int32_t def;
        std::int32_t min;
        std::int32_t max;
        std::int32_t step;
        std::uint32_t aux;
        std::uint32_t reserved;
    };
    static_assert(sizeof(Param) == 32);

    /** PCHC: u32 listCount, then listCount ListRef, then the i32 values. */
    struct ListRef
    {
        std::uint32_t first;
        std::uint32_t count;
    };
    static_assert(sizeof(ListRef) == 8);

    enum class ExprOp : std::uint16_t
    {
        Const = 0,
        Param = 1,
        Add = 2,
        Sub = 3,
        Mul = 4,
        Div = 5,
        Mod = 6,
        Min = 7,
        Max = 8,
        Neg = 9,
        Abs = 10,
        Lt = 11,
        Le = 12,
        Eq = 13,
        And = 14,
        Or = 15,
        Not = 16,
        Count_ = 17,
    };

    /** EXPR: u32 count, then count nodes. Div and Mod use floor semantics, the same as
     *  Python; a zero divisor is a mount-time refusal. The comparisons and the logical
     *  operators yield 0 or 1 and treat any non-zero operand as true. imm holds the
     *  value for Const and the parameter index for Param. */
    struct ExprNode
    {
        std::uint16_t op;
        std::uint16_t reserved;
        std::uint32_t a;
        std::uint32_t b;
        std::int32_t imm;
    };
    static_assert(sizeof(ExprNode) == 16);

    /** ROLE: u32 count, then count entries. */
    struct Role
    {
        std::uint32_t nameStr;
        std::uint32_t defaultBlockStr;
    };
    static_assert(sizeof(Role) == 8);

    /** ZONE: u32 zoneCount; u32 nameStr[zoneCount]; u32 periodXExpr; u32 periodZExpr;
     *  u32 spanCountX; Span[spanCountX]; u32 spanCountZ; Span[spanCountZ];
     *  u32 combine[zoneCount * zoneCount] indexed as zoneX * zoneCount + zoneZ. The spans
     *  of one axis cover [0, period) in order and their lengths sum to the period. */
    struct Span
    {
        std::uint32_t zone;
        std::uint32_t lenExpr;
    };
    static_assert(sizeof(Span) == 8);

    /** STAK: u32 count, then count entries in painter's order. zone is a combined 2D
     *  zone id or kNone for every zone; role kNone means air; the range is [from, to). */
    struct Stack
    {
        std::uint32_t zone;
        std::uint32_t fromExpr;
        std::uint32_t toExpr;
        std::uint32_t role;
    };
    static_assert(sizeof(Stack) == 16);

    /** CNST: u32 count, then count entries. The expression must evaluate to a non-zero
     *  value once the parameters are bound. */
    struct Constraint
    {
        std::uint32_t expr;
        std::uint32_t messageStr;
    };
    static_assert(sizeof(Constraint) == 8);

    enum class ShapeOp : std::uint16_t
    {
        Nothing = 0,
        Box = 1,
        Cylinder = 2,
        Wedge = 3,
        Translate = 4,
        RotateY = 5,
        Mirror = 6,
        Repeat = 7,
        Union = 8,
        Difference = 9,
        Intersect = 10,
        Shell = 11,
        Paint = 12,
        Choose = 13,
        Voxels = 14,
        Count_ = 15,
    };

    inline constexpr std::uint16_t kShapeP0IsExpr = 1u << 0;
    inline constexpr std::uint16_t kShapeP1IsExpr = 1u << 1;
    inline constexpr std::uint16_t kShapeP2IsExpr = 1u << 2;

    /** SHAP: u32 count; ShapeNode[count]; u32 chooseListCount; ListRef[chooseListCount];
     *  ChooseEntry[...]. Shapes sit with their bounding box at the origin, Translate
     *  moves them, RotateY and Mirror keep the minimum corner where it is, Repeat lays
     *  count copies period apart along an axis. Union keeps the role of a where both
     *  are inside. Choose picks one entry of list aux by the cell hash (p0 = 0, p1 =
     *  salt) or by a Choice parameter (p0 = 1, p1 = parameter index). */
    struct ShapeNode
    {
        std::uint16_t op;
        std::uint16_t flags;
        std::uint32_t a;
        std::uint32_t b;
        std::int32_t p0;
        std::int32_t p1;
        std::int32_t p2;
        std::uint16_t role;
        std::uint16_t reserved;
        std::uint32_t aux;
    };
    static_assert(sizeof(ShapeNode) == 32);

    struct ChooseEntry
    {
        std::uint32_t node;
        std::uint32_t weight;
    };
    static_assert(sizeof(ChooseEntry) == 8);

    /** PICK: u32 count; Pick[count]; PickRoot[...]. One root is chosen per period cell
     *  by hash(cellX, cellZ, salt) weighted over the roots; its origin lands on
     *  (x0, anchorY, z0) of the cell and it is rotated inside the w by d rectangle. */
    struct Pick
    {
        std::uint32_t salt;
        std::uint32_t x0Expr;
        std::uint32_t z0Expr;
        std::uint32_t wExpr;
        std::uint32_t dExpr;
        std::uint32_t anchorYExpr;
        std::uint32_t rootFirst;
        std::uint32_t rootCount;
    };
    static_assert(sizeof(Pick) == 32);

    struct PickRoot
    {
        std::uint32_t shapeNode;
        std::uint32_t weight;
        std::uint32_t rotMask;
        std::uint32_t reserved;
    };
    static_assert(sizeof(PickRoot) == 16);

    inline constexpr std::uint8_t kVoxelRaw = 0;
    inline constexpr std::uint8_t kVoxelRle = 1;
    inline constexpr std::uint16_t kVoxelKeep = 0;
    inline constexpr std::uint16_t kVoxelAir = 1;

    /** VOXL: u32 count; Voxel[count]; u32 paletteRefs[...]; the byte data. The palette
     *  of an entry is paletteRefs[palFirst .. palFirst + palCount) of STRS indices, and
     *  index 0 resolves to the empty string, meaning keep, index 1 to minecraft:air.
     *  Cells are stored column by column, y fastest: cell (x, y, z) is at
     *  (x * sz + z) * sy + y. Raw data is u16 per cell; RLE data holds, per column,
     *  varint pairs of run length and palette index summing to sy, and colOff is a table
     *  of sx * sz u32 byte offsets into the main data where each column starts. */
    struct Voxel
    {
        std::uint32_t sx;
        std::uint32_t sy;
        std::uint32_t sz;
        std::uint32_t palCount;
        std::uint32_t palFirst;
        std::uint8_t encoding;
        std::uint8_t hasLiquid;
        std::uint8_t reserved[2];
        std::uint64_t mainOff;
        std::uint64_t mainLen;
        std::uint64_t colOff;
        std::uint64_t liqOff;
        std::uint64_t liqLen;
        std::uint64_t liqColOff;
    };
    static_assert(sizeof(Voxel) == 72);

    /** CONF: one entry. The confinement grid is the geometry of setCellGrid: a cell is
     *  [0, cell) of a period cell + gap on both axes, so the expressions must add up to
     *  the x period and the z period alike, which the mount checks. */
    struct Confine
    {
        std::uint32_t cellExpr;
        std::uint32_t gapExpr;
    };
    static_assert(sizeof(Confine) == 8);

    //  PIERVOL sections

    inline constexpr std::uint32_t kSecSettings = fourcc('S', 'E', 'T', 'T');
    inline constexpr std::uint32_t kSecNoises = fourcc('N', 'O', 'I', 'S');
    inline constexpr std::uint32_t kSecSplines = fourcc('S', 'P', 'L', 'N');
    inline constexpr std::uint32_t kSecFloatConsts = fourcc('F', 'C', 'O', 'N');
    inline constexpr std::uint32_t kSecFunctions = fourcc('F', 'U', 'N', 'C');
    inline constexpr std::uint32_t kSecRouter = fourcc('R', 'O', 'U', 'T');
    inline constexpr std::uint32_t kSecSurface = fourcc('S', 'U', 'R', 'F');
    inline constexpr std::uint32_t kSecBiomes = fourcc('B', 'I', 'O', 'M');
    inline constexpr std::uint32_t kSecRandoms = fourcc('R', 'A', 'N', 'D');

    inline constexpr std::uint32_t kSettingsAquifers = 1u << 0;
    inline constexpr std::uint32_t kSettingsOreVeins = 1u << 1;
    inline constexpr std::uint32_t kSettingsLegacyRandom = 1u << 2;

    /** SETT: one entry. height is a multiple of 16. sizeHorizontal and sizeVertical are
     *  the interpolation cell sizes in quarters, as in Java. surfaceRand names the RAND
     *  entry of the surface system, surfaceNoise and surfaceSecondaryNoise its NOIS
     *  entries; kNone when the pack has no surface rules. */
    struct Settings
    {
        std::int32_t minY;
        std::int32_t height;
        std::int32_t seaLevel;
        std::uint32_t defaultBlockStr;
        std::uint32_t defaultFluidStr;
        std::uint32_t sizeHorizontal;
        std::uint32_t sizeVertical;
        std::uint32_t flags;
        std::uint32_t surfaceRand;
        std::uint32_t surfaceNoise;
        std::uint32_t surfaceSecondaryNoise;
        std::uint32_t reserved;
    };
    static_assert(sizeof(Settings) == 48);

    /** RAND: u32 count; u32 nameStr[count]. The names of the positional random
     *  factories the surface rules use; the host derives each from the world seed as
     *  fromHashOf(name).forkPositional(). */

    inline constexpr std::uint32_t kNoiseNormal = 0;
    inline constexpr std::uint32_t kNoiseSimplex = 1;
    inline constexpr std::uint32_t kNoiseBlended = 2;
    inline constexpr std::uint32_t kNoiseLegacyBiome = 1u << 0;
    inline constexpr std::uint32_t kNoiseLegacyIndexShift = 8;

    /** NOIS: u32 count; Noise[count]; f64 amplitudes[...]. The parameters of a noise,
     *  never its tables: the host builds the tables from the world seed the way Java
     *  does, so one pack serves any seed. A normal noise is seeded from
     *  fromHashOf(name); with kNoiseLegacyBiome it is the legacy nether biome noise
     *  built from LegacyRandom(seed + index) with the index in flags above bit 8. The
     *  simplex noise is the end island noise, LegacyRandom(seed) after 17292 draws. A
     *  blended noise is seeded from fromHashOf("minecraft:terrain") and its five
     *  parameters, xz_scale, y_scale, xz_factor, y_factor and smear_scale_multiplier,
     *  sit in the amplitude array. valueFactor is the tool's precomputed
     *  NormalNoise factor. */
    struct Noise
    {
        std::uint32_t nameStr;
        std::int32_t firstOctave;
        std::uint32_t octaveCount;
        std::uint32_t ampFirst;
        double valueFactor;
        std::uint32_t kind;
        std::uint32_t flags;
    };
    static_assert(sizeof(Noise) == 32);

    inline constexpr std::uint32_t kSplinePointConst = 0;
    inline constexpr std::uint32_t kSplinePointSpline = 1;

    /** SPLN: u32 count; Spline[count]; SplinePoint[...]. A point value is a float
     *  (kind 0, ref holds the bits) or a nested spline (kind 1, ref is its index). */
    struct Spline
    {
        std::uint32_t coordNode;
        std::uint32_t pointFirst;
        std::uint32_t pointCount;
        std::uint32_t reserved;
    };
    static_assert(sizeof(Spline) == 16);

    struct SplinePoint
    {
        float location;
        float derivative;
        std::uint32_t kind;
        std::uint32_t ref;
    };
    static_assert(sizeof(SplinePoint) == 16);

    enum class FuncOp : std::uint16_t
    {
        Const = 0,
        Add = 1,
        Mul = 2,
        Min = 3,
        Max = 4,
        Abs = 5,
        Square = 6,
        Cube = 7,
        HalfNegative = 8,
        QuarterNegative = 9,
        Squeeze = 10,
        Clamp = 11,
        RangeChoice = 12,
        YClampedGradient = 13,
        NoiseOp = 14,
        ShiftedNoise = 15,
        Shift = 16,
        ShiftA = 17,
        ShiftB = 18,
        SplineOp = 19,
        EndIslands = 20,
        WeirdScaledSampler = 21,
        BlendAlpha = 22,
        BlendOffset = 23,
        BlendDensity = 24,
        Interpolated = 25,
        FlatCache = 26,
        Cache2D = 27,
        CacheOnce = 28,
        CacheAllInCell = 29,
        OldBlendedNoise = 30,
        Count_ = 31,
    };

    /** FUNC: u32 count; FuncNode[count]. a, b, c are operands; p0 and p1 are the two
     *  inline floats; aux names a noise, a spline, or an FCON index for the operators
     *  that need more than two floats. FCON: u32 count; f32 values[count].
     *  OldBlendedNoise has no operands and aux is a NOIS entry of kind kNoiseBlended. */
    struct FuncNode
    {
        std::uint16_t op;
        std::uint16_t flags;
        std::uint32_t a;
        std::uint32_t b;
        std::uint32_t c;
        float p0;
        float p1;
        std::uint32_t aux;
        std::uint32_t reserved;
    };
    static_assert(sizeof(FuncNode) == 32);

    /** ROUT: one entry of FUNC node indices, kNone where the channel is unused. */
    struct Router
    {
        std::uint32_t finalDensity;
        std::uint32_t temperature;
        std::uint32_t vegetation;
        std::uint32_t continents;
        std::uint32_t erosion;
        std::uint32_t depth;
        std::uint32_t ridges;
        std::uint32_t initialDensityWithoutJaggedness;
        std::uint32_t barrier;
        std::uint32_t fluidLevelFloodedness;
        std::uint32_t fluidLevelSpread;
        std::uint32_t lava;
        std::uint32_t veinToggle;
        std::uint32_t veinRidged;
        std::uint32_t veinGap;
        std::uint32_t reserved;
    };
    static_assert(sizeof(Router) == 64);

    enum class SurfOp : std::uint16_t
    {
        Block = 0,
        Sequence = 1,
        Condition = 2,
        Bandlands = 3,
        Biome = 10,
        NoiseThreshold = 11,
        VerticalGradient = 12,
        YAbove = 13,
        Water = 14,
        Temperature = 15,
        Steep = 16,
        Not = 17,
        Hole = 18,
        AbovePreliminarySurface = 19,
        StoneDepth = 20,
    };

    inline constexpr std::uint16_t kAnchorAbsolute = 0;
    inline constexpr std::uint16_t kAnchorAboveBottom = 1;
    inline constexpr std::uint16_t kAnchorBelowTop = 2;
    inline constexpr std::uint16_t kSurfAddStoneDepth = 1u << 4;
    inline constexpr std::uint16_t kSurfCeiling = 1u << 5;

    /** SURF: u32 count; SurfNode[count]; u32 listCount; ListRef[listCount]; u32 items[];
     *  u32 root. flags bits 0-1 give the anchor kind of i0 and bits 2-3 that of i1.
     *  Sequence and Biome name a list of items: node indices for Sequence, STRS indices
     *  for Biome. root is the rule applied per cell, kNone when the pack has none. */
    struct SurfNode
    {
        std::uint16_t op;
        std::uint16_t flags;
        std::uint32_t a;
        std::uint32_t b;
        float p0;
        float p1;
        std::int32_t i0;
        std::int32_t i1;
        std::uint32_t aux;
    };
    static_assert(sizeof(SurfNode) == 32);

    /** BIOM: u32 count; BiomeTarget[count]. The six climate intervals in Java's order,
     *  then the offset; the target with the least distance wins. baseTemperature is the
     *  biome's own temperature, read by the temperature surface condition. */
    struct BiomeTarget
    {
        std::uint32_t biomeStr;
        float temperature[2];
        float humidity[2];
        float continentalness[2];
        float erosion[2];
        float depth[2];
        float weirdness[2];
        float offset;
        float baseTemperature;
    };
    static_assert(sizeof(BiomeTarget) == 60);
} // namespace pier::dimensions::pack

#pragma once

#include <cstdint>

/**
 * plot_confine.h —— 「这两个坐标算不算同一块地皮」。
 *
 * # 为什么这件事必须在 C++ 侧回答
 *
 * 已有的 `DimRule::PistonPush` 是**整维度一刀切**：这个维度里活塞搬不动任何
 * 方块。而「飞行器 / 活塞 / 实体不出地皮」要的是按**边界**判 —— 地皮内部照常
 * 推，跨界才拦。判定发生在 `PistonBlockActor::_checkAttachedBlocks` 和
 * `Actor::move` 里，那是引擎的 tick 路径，每秒几百上千次，不可能一次次跨
 * ABI 边界回模组那头问。
 *
 * 所以网格几何和合并关系由模组侧**推过来**（`md_set_plot_grid` /
 * `md_set_plot_merges`），这里只存和查。
 *
 * # 归属判定必须和模组侧逐条一致
 *
 * 对应的是地皮插件里的 `owning_plot`：
 *
 * ```text
 *   地皮内部          → 该地皮
 *   南北向的路（缝）  → 西侧那块**声明了朝东合并**时归它，否则不属于任何地皮
 *   东西向的路（缝）  → 北侧那块**声明了朝南合并**时归它，否则不属于任何地皮
 *   路口              → 围它的 2×2 四条边全合并时归西北角那块，否则不属于任何地皮
 * ```
 *
 * 两侧不一致的后果不是「判错一格」，而是**主人在自己合并出来的地皮上被拦住**：
 * 模组侧说这条缝是地皮（放行放方块），这一侧说它不是地皮（拦住活塞），
 * 玩家看到的是「我能手放，活塞就是推不过去」。
 *
 * # 没注册网格的维度完全不受影响
 *
 * 和 `dimension_rules.h` 同一条硬约束：这些 hook 装在全局，任何一个没被管理
 * 的维度都必须完全感觉不到它们的存在。没有网格 → `sameArea` 一律返回 true。
 */
namespace pier::dimensions
{
    /** 一块地皮的网格编号。和模组侧的 `PlotId` 同构。 */
    struct PlotXZ
    {
        int32_t x{0};
        int32_t z{0};

        friend bool operator==(PlotXZ const& a, PlotXZ const& b) { return a.x == b.x && a.z == b.z; }
        friend bool operator!=(PlotXZ const& a, PlotXZ const& b) { return !(a == b); }
    };

    /** 合并标记的方向位。和模组侧 `merged` 数组的下标一致：0=N 1=E 2=S 3=W。 */
    enum MergeBit : uint32_t
    {
        kMergeNorth = 1u << 0,
        kMergeEast = 1u << 1,
        kMergeSouth = 1u << 2,
        kMergeWest = 1u << 3,
    };

    /**
     * 注册 / 更新一个维度的地皮网格。
     *
     * `plotSize <= 0` 视为「这个维度没有地皮网格」，等价于 `clearPlotGrid`。
     * 传进来的数值**不被信任**：负的 roadWidth 会让取模变成除零，这里夹一遍。
     */
    void setPlotGrid(int dimension, int plotSize, int roadWidth);

    /** 撤销一个维度的网格（世界被删除 / 改成非地皮模型时调）。连合并表一起清。 */
    void clearPlotGrid(int dimension);

    /**
     * 整表替换一个维度的合并标记。
     *
     * `entries` 是 `count` 组 `(x, z, mask)`，共 `count * 3` 个 int32。
     * **只需要传有合并标记的地皮** —— 没有条目的地皮按「四面都没合并」处理，
     * 所以一个几千块地皮、只有十来处合并的服务器，这张表也就几十个整数。
     *
     * 整表替换而不是增量：增量要求两侧对「现在有哪些条目」的看法始终一致，
     * 而拆分（unlink）是先清邻居再存自己、中途可能失败的，一旦对不上就再也
     * 没有自愈的机会。整表替换每次都把状态拉回一致。
     */
    void setPlotMerges(int dimension, int32_t const* entries, int32_t count);

    /**
     * 这个维度当前有没有地皮网格。hook 的第一道快速路径。
     *
     * 单独暴露是因为它比 `sameArea` 便宜得多：没有网格时不需要算任何坐标。
     */
    bool hasPlotGrid(int dimension);

    /**
     * **唯一的问句**：`(x1,z1)` 和 `(x2,z2)` 是否属于同一片可自由活动的区域。
     *
     * * 没有网格的维度 → 恒 `true`（不管）。
     * * 两点都不属于任何地皮（都在道路上）→ `true`。道路是公共区域，
     *   在道路上活动不是越界。
     * * 一点在地皮上、另一点不在 → `false`。这一条同时挡住「推出去」和
     *   「推进来」，方向是对称的：把苦力怕塞进别人地皮和把别人的箱子推出来，
     *   是同一类操作。
     * * 两点都在地皮上 → 归属的**合并组根**相同才算同一片。
     */
    bool sameArea(int dimension, int x1, int z1, int x2, int z2);

    /**
     * 诊断用：这一格归哪块地皮。`out` 只在返回 true 时被写。
     *
     * 拦截路径本身不用它（`sameArea` 一次算完两侧、共享备忘），这里暴露出来
     * 是为了让「为什么这次被拦了」能被人工复现。
     */
    bool owningPlot(int dimension, int x, int z, PlotXZ* out);
} // namespace pier::dimensions

#pragma once

/**
 * dimension_rules.h —— 按维度生效的行为规则。
 *
 * 和 gamerule 的区别见 `DimensionRules.cpp` 的文件头：gamerule 是全服一份的，
 * 这里的规则钩在真正干活的函数上、按维度判定。
 *
 * **没有设过规则的维度完全不受影响** —— 所有 hook 都会直接 `origin()`。
 * 这条不变量是这套设计能全局装 hook 的前提，任何改动都要先满足它。
 */

namespace pier::dimensions
{
    /**
     * 规则编号。**必须和 `sdk/abi.h` 的 `PierDimRule` 逐值一致** ——
     * ABI 传过来的就是那个整数，这里只是给它一个有名字的形状。
     * 只能追加，不能重排（ABI 常量的纪律，契约 §2.2）。
     *
     * 一致性由 `DimensionRules.cpp` 里的 `static_assert` 在编译期钉死：
     * 两处各写一遍而没有强制，迟早会有人只改一边 —— 而错位的症状是
     * 「设了不刷怪，结果关掉的是火焰蔓延」，从现象看不出根因在编号上。
     */
    enum class DimRule : int
    {
        SpawnMonster = 0,
        SpawnAnimal = 1,
        SpawnSpawner = 2,
        ExplodeBlocks = 3,
        FireSpread = 4,
        MobGriefing = 5,
        Projectile = 6,
        PistonPush = 7,
        LiquidFlow = 8,
        FarmlandDecay = 9,
        Ride = 10,
        /**
         * 活塞把方块推**过地皮边界**。
         *
         * 和 `PistonPush` 是两件事，别混：`PistonPush=false` 是整个维度里活塞
         * 搬不动任何方块；`PistonCrossPlot=false` 是地皮内部照常推、跨界才拦。
         * 两条都设时，任意一条禁止就推不动。
         *
         * 需要 `setPlotGrid` 注册过网格才有意义；没有网格的维度这一条恒放行。
         */
        PistonCrossPlot = 11,
        /** 实体越过地皮边界（玩家和载人的载具不受此限，见 PlotConfine.cpp）。 */
        EntityCrossPlot = 12,
    };

    inline constexpr int kDimRuleCount = 13;

    void setDimensionRule(int dimension, int rule, bool allow);

    /**
     * 查一条规则。
     *
     * 返回值是「**问得出来吗**」，`outAllow` 才是「答案是不是允许」——
     * 两件事分开（契约 §5.2）。压成一个 bool 的话，「这个维度没设过这条规则」
     * 和「这条规则被设成了禁止」会给出同一个 false，调用方只能猜。
     */
    bool getDimensionRule(int dimension, int rule, bool* outAllow);

    void clearDimensionRules(int dimension);

    void registerDimensionRuleHooks();
    void unregisterDimensionRuleHooks();
} // namespace pier::dimensions

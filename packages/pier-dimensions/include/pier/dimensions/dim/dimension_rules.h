#pragma once

/**
 * dimension_rules.h: behavior rules that apply per dimension.
 *
 * The file header of `DimensionRules.cpp` explains how these differ from a gamerule: a
 * gamerule is one value for the whole server, while these rules hook the functions that
 * do the work and decide per dimension.
 *
 * A dimension with no rule set is entirely unaffected, since every hook goes straight to
 * `origin()`. That invariant is what allows these hooks to be installed globally, and any
 * change has to satisfy it first.
 */

namespace pier::dimensions
{
    /**
     * Rule numbers. They must match `PierDimRule` in `sdk/abi.h` value for value, since
     * what arrives across the ABI is that integer and this only gives it a named shape.
     * Append only, never reorder, which is the discipline for an ABI constant
     * (contract §2.2).
     *
     * A `static_assert` in `DimensionRules.cpp` pins the agreement at compile time.
     * Written twice without enforcement, someone eventually changes one side only, and
     * the symptom of a mismatch is setting mob spawning off and turning off fire spread
     * instead, which shows nothing about the numbering.
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
         * A piston pushing a block across a plot boundary.
         *
         * Distinct from `PistonPush`: `PistonPush=false` means a piston moves no block
         * anywhere in the dimension, while `PistonCrossPlot=false` allows pushing inside
         * a plot and blocks only a crossing. With both set, either one forbidding stops
         * the push.
         *
         * It is meaningful only once `setPlotGrid` has registered a grid, and a dimension
         * without one always allows it.
         */
        PistonCrossPlot = 11,
        /** An actor crossing a plot boundary. Players and ridden vehicles are exempt,
         *  see PlotConfine.cpp. */
        EntityCrossPlot = 12,
    };

    inline constexpr int kDimRuleCount = 13;

    void setDimensionRule(int dimension, int rule, bool allow);

    /**
     * Looks up one rule.
     *
     * The return value says whether the question could be answered, while `outAllow`
     * carries whether the answer is allow. The two are kept apart (contract §5.2).
     * Collapsed into one bool, a dimension that never set the rule and a rule set to
     * forbid would both yield false and a caller could only guess.
     */
    bool getDimensionRule(int dimension, int rule, bool* outAllow);

    void clearDimensionRules(int dimension);

    void registerDimensionRuleHooks();
    void unregisterDimensionRuleHooks();
} // namespace pier::dimensions

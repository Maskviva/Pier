/**
 * pier-dimensions/rt/Slots.cpp —— 把本包的能力装进 ABI 表（`md_*` 一族）。
 *
 * 这里只做 ABI 转接，逻辑全在各自的实现文件里 —— 两处各判一遍迟早会分叉。
 * 所有函数都在服务器线程上跑。
 *
 * # 关于「客户端也要有这些槽」的历史
 *
 * 旧仓把 `md_list_dimensions` / `md_set_plot_grid` / `md_clear_plot_grid` /
 * `md_set_plot_merges` 放在另一个命名空间里，理由是它们落在 ABI 的**公共尾
 * 部**，客户端构建的静态 ApiTable 也会引用它们，得有桩。新架构没有这个问
 * 题：表由各能力包**自己填**（SlotPack），本包在客户端目标上根本不编入，那
 * 些槽位保持 NULL，SDK 按空槽纪律报「不支持」。所以这里不再需要跨命名空间
 * 的安排，全部收在一处。
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
    /** Bridge.cpp 里的三层名字解析。 */
    int idByName(std::string const& name);

    namespace
    {
        using pier::hostLogger;
        using pier::ps;
        using pier::sv;
        using pier::toString;

        /**
         * 新建一个 SimpleCustomDimension。
         *
         * `generatorType` 是 `::GeneratorType` 的值**原样**：1=Overworld、
         * 2=Flat、3=Nether、4=TheEnd、5=Void。它不是我们自己的一套编号 ——
         * 而这条注释曾经声称它是（写着「0=Overworld, 1=Nether, 2=TheEnd,
         * 3=Flat, 4=Void」），那份表和引擎在两个方向上同时错位一格：它把 Flat
         * 变成了 Nether、把 Void 变成了 TheEnd。选「超平坦」的调用方拿到的是
         * 一个下界世界。
         *
         * Legacy(0) 和 Undefined(6) 是**拒绝**而不是透传：两者在
         * `SimpleCustomDimension::createGenerator` 里都没有对应分支，会落进
         * default 分支安静地建出一个虚空世界。
         *
         * 返回分配到的维度 id（成功 >= 3，失败 -1）。
         *
         * 幂等：下次开服用同一个名字再调一次会拿到同一个 id
         *（CustomDimensionManager 复用持久化下来的条目）。调用方应该每次启动
         * 都无条件注册，而不是先探测。
         *
         * 持久化位置是 **`worlds/<levelName>/dimension_config.json`**，跟着存
         * 档走，不在 `configs/` 下面。这一条值得写准：旧仓的注释说它在
         * `configs/levilamina-rust-loader/dimensions.json`，而代码从来没往那
         * 里写过。按那句话去找文件的人会找不到，进而以为维度没被持久化、动手
         * 重新注册 —— 而重新注册会重新分配 id，玩家存档里的 DimensionId 当场
         * 失效。维度 id 必须跟着存档，所以放在 worlds 下面是对的。
         */
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
                        "add_simple_dimension('{}') 拒绝：generatorType={} 不是受支持的 "
                        "::GeneratorType（可用值 1=Overworld 2=Flat 3=Nether 4=TheEnd 5=Void）。"
                        "这个维度**没有**被创建 —— 与其建出一个生成器不对的世界，不如现在失败，"
                        "因为生成器名会被写进 dimensions.json，之后改不回来。",
                        dimName, generatorTypeInt
                    );
                    return -1;
                }
                auto genType = static_cast<GeneratorType>(generatorTypeInt);
                hostLogger().debug(
                    "add_simple_dimension('{}')：seed={} generatorType={}({})",
                    dimName, seed, magic_enum::enum_name(genType), generatorTypeInt
                );
                auto id = CustomDimensionManager::getInstance().addDimension<SimpleCustomDimension>(
                    dimName, seed, genType
                );
                return id.value();
            PIER_API_GUARD_END_VAL(-1)
        }

        /**
         * 新建一个 PlotDimension：区块生成器在生成时铺出地皮网格（地皮 / 道路
         * / 边界）的自定义维度。
         *
         * `layoutSnbt` 是一个 CompoundTag 的 SNBT，见 `PlotLayout::fromSnbt`。
         * 各项数值在**那一侧**钳制 —— 绝不能相信调用方给的下标，它们要喂进固
         * 定大小的区块缓冲。
         *
         * 返回分配到的维度 id（>= 3），失败 -1。
         */
        int32_t api_md_add_plot_dimension(PierStr name, uint32_t seed, PierStr layoutSnbt)
        {
            PIER_API_GUARD_BEGIN
                // 注意：PierStr 是 {ptr,len}，`ptr` **不以 \0 结尾**。早先这里
                // 用 printf("%s", name.data()) 会一路读到下一个偶然出现的 \0
                // —— 日志里那些粘在一起、末尾带乱码方块的行就是这么来的，而且
                // 是 UB。一律经 toString/sv 过一道。
                std::string const dimName = toString(name);
                hostLogger().debug(
                    "add_plot_dimension: name='{}' seed={} layout={}",
                    dimName, seed, sv(layoutSnbt)
                );
                auto layout = PlotLayout::fromSnbt(toString(layoutSnbt));
                auto id = CustomDimensionManager::getInstance().addDimension<PlotDimension>(
                    dimName, seed, layout
                );
                return id.value();
            PIER_API_GUARD_END_VAL(-1)
        }

        /** 名字 → id。查不到返回 -1。三层数据源与「为什么不能碰
         *  VanillaDimensions::toString」的血泪史见 Bridge.cpp。 */
        int32_t api_md_get_dimension_id(PierStr name)
        {
            PIER_API_GUARD_BEGIN
                return idByName(toString(name));
            PIER_API_GUARD_END_VAL(-1)
        }

        /* 维度规则三入口。实现在 DimensionRules.cpp。 */

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
         * 列出全部已注册的自定义维度。
         *
         * 数据源是 `CustomDimensionConfig` 的 `dimensionList` —— 也就是
         * `dimensions.json` 的内存镜像。**故意读配置而不是读引擎的维度表**：
         * 配置里的 `dimId` 是引擎当初分配、然后持久化下来的那个号，重启之后
         * 仍然是同一个；引擎表里还混着原版三维度和一个会变的 `Undefined()`。
         *
         * 输出一次一条 JSON 对象，由调用方拼成数组。分条发是为了让一条畸形的
         * sNbt 只毁掉它自己那一条，而不是整个列表。
         */
        void api_md_list_dimensions(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return;
                auto const& cfg = CustomDimensionConfig::getConfig();
                for (auto const& [name, info] : cfg.dimensionList)
                {
                    // 名字和 sNbt 都可能带引号和反斜杠，不转义的话一条畸形数据
                    // 会把整个 JSON 数组毁掉。
                    std::string line = "{\"name\":\"" + pier::snbtEscape(name)
                        + "\",\"dim\":" + std::to_string(info.dimId)
                        + ",\"snbt\":\"" + pier::snbtEscape(info.sNbt) + "\"}";
                    sink(ctx, ps(line));
                }
            PIER_API_GUARD_END_VOID
        }

        /* 地皮边界约束。逻辑全在 PlotConfine.cpp。 */

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
                // 空表是合法输入（「这个世界一处合并都没有」），但 count>0 配
                // 空指针是调用方的 bug，别拿它去做指针算术。
                if (entries == nullptr) count = 0;
                setPlotMerges(dimension, entries, count);
            PIER_API_GUARD_END_VOID
        }

        /**
         * 这套能力在本次构建里可用吗。
         *
         * 恒 true：这个函数**只有编进了本包才存在**。本包不编入时槽位是
         * NULL，SDK 按空槽纪律直接报「不支持」，根本不会调到这里 —— 所以让它
         * 返回 false 的那条路不存在。留着这个槽是为了给 SDK 一个不必自己判空
         * 的问法。
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

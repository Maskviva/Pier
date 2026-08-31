#pragma once

/**
 * custom_dimension_manager.h —— 自定义维度的注册中枢。
 *
 * 本包唯一分配维度 id 的地方：三个数据源（引擎的 NameIdStore、dimension_config.json、宿主自
 * 己的名字台账）在这里汇合并被强制一致。DimensionFactoryInfo 是工厂闭包收到的一包参数：
 * arguments 是引擎给的派生维度构造参数（引用，只在闭包执行期间有效），data 是本包的业务载荷（
 * seed、layout），dimId 是已经确定的维度 id，闭包里不许再去猜它。
 *
 * addDimension 是模板，调用方写 addDimension<PlotDimension>(name, seed, layout)：维度类型在编
 * 译期定下，工厂闭包和 generateNewData 都由它推出来。把类型放进模板参数而不是运行期枚举，是为了
 * 新增一种维度不必改这个文件。
 *
 * 本包在新架构里是编进宿主的 object 包，不导出任何符号，所以没有导出宏。也不要再加转调
 * VanillaDimensions::fromString 的按名查 id 函数：那条路对自定义维度会回读出垃圾值，理由见
 * rt/Bridge.cpp。
 */

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/dimension/DimensionType.h"

class Dimension;
class DerivedDimensionArguments;

namespace pier::dimensions
{
    struct DimensionFactoryInfo
    {
        DerivedDimensionArguments& arguments;
        CompoundTag const& data;
        DimensionType dimId;
    };

    class CustomDimensionManager
    {
        struct Impl;
        std::unique_ptr<Impl> impl;

        CustomDimensionManager();
        ~CustomDimensionManager();

    public:
        using DimensionFactoryT = std::shared_ptr<Dimension>(DimensionFactoryInfo const&);

        CustomDimensionManager(CustomDimensionManager const&) = delete;
        CustomDimensionManager& operator=(CustomDimensionManager const&) = delete;

        /**
         * 单例。构造时读配置、装 hook —— 所以**第一次调用必须在 Level 就绪
         * 之后**（要读 levelName 定位存档目录）。
         */
        static CustomDimensionManager& getInstance();

        /**
         * 注册一个维度。成功返回引擎分配的 id（>= 3）。
         *
         * 失败一律抛异常，不返回一个「看起来像 id」的值。理由是这里的失败
         * 只有一种形状：注册没真正生效，而调用方拿着一个假 id 去传送玩家会让
         * 引擎在区块工作线程上抛未捕获异常、整个进程 fastfail(0xC0000409)。
         * 「问不出来」和「答案是 3」必须分开（契约 §5.2）；ABI 那一头由
         * `PIER_API_GUARD_END_VAL(-1)` 把异常翻译成 -1。
         */
        template <std::derived_from<Dimension> D, class... Args>
        DimensionType addDimension(std::string const& dimName, Args&&... args)
        {
            return addDimension(
                dimName,
                [dimName](DimensionFactoryInfo const& info) -> std::shared_ptr<Dimension>
                {
                    return std::make_shared<D>(dimName, info);
                },
                [&] { return D::generateNewData(std::forward<Args>(args)...); }
            );
        }

    protected:
        DimensionType addDimension(
            std::string const& dimName,
            std::function<DimensionFactoryT> factory,
            std::function<CompoundTag()> const& newData
        );
    };
} // namespace pier::dimensions

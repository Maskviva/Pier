#pragma once

/**
 * native_dimensions.h —— BDS 26.20 引擎原生自定义维度接口的封装。
 *
 * ## 为什么需要这个文件
 *
 * ── 本宿主只走这一条路 ──────────────────────────────────────────────
 * 老的 FakeDimensionId 方案（改写所有出站包的维度 id、拦掉
 * DimensionDataPacket、切维度前先假装去一趟下界）已经整体删除。26.20 上官方
 * 推荐的就是这里的原生路径，两者互斥，不要再把那套加回来。
 *
 * MoreDimensions（以及本仓早期那份手抄件）的做法是：
 *   1. 往 `VanillaDimensions::DimensionMap()` 里插一条 name <-> id；
 *   2. 往 `Level::getDimensionFactory().mFactoryMap` 里插一个按名字建维度的工厂。
 *
 * 在 26.20 上，这两处**都不再是** `Level::getOrCreateDimension(DimensionType)`
 * 的数据源。这一版的 `DimensionManager` 长这样：
 *
 *   Util::NameIdStore<DimensionIdType>  mDimensionNameIdStore;   // 名字 -> 注册 id，存进存档
 *   DimensionRegistry                   mDimensionRegistry;      // key 是 DimensionIdType(ushort)
 *   DimensionDefinitionGroup            mDimensionDefinitionGroup;
 *   Publisher<void(DimensionManager&)>  mOnReadyForCustomDimensionRegistrationPublisher;
 *
 *   std::optional<DimensionType> serverRegisterCustomDimension(std::string_view);
 *   DimensionType                getDimensionId(std::string_view) const;
 *   bool                         isDimensionTypeActive(DimensionType) const;
 *   WeakRef<Dimension>           getOrCreateDimension(std::string_view);
 *
 * 也就是说 Mojang 自己把自定义维度做成了一等公民（数据驱动维度那一套）。引擎
 * 拿一个 id 去建维度时，走的是 `mDimensionNameIdStore` 反查名字。私自往
 * `DimensionMap` 里塞东西，`NameIdStore` 里没有对应条目，
 * `getOrCreateDimension` 就只能返回一个 expired 的 WeakRef —— 这就是维度桥的
 * `blockSourceOf` 返回 nullptr、最终报「传送失败」的原因。
 *
 * ## 这里做什么
 *
 * `registerCustomDimension()` 把注册交还给引擎：先保证
 * `DimensionDefinitionGroup` 里有这个名字的定义（几何范围 + 生成器类型），再
 * 调 `serverRegisterCustomDimension()` 拿到引擎分配的 id。id 由引擎写进存档的
 * NameIdStore，重启自动带回来 —— `dimension_config.json` 从此降级成「我们自
 * 己业务数据（seed / layout）的存放处」，不再是 id 的权威来源。
 *
 * 拿到 id 之后仍然要覆盖 `mFactoryMap` 里那一条：引擎默认会给自定义维度建一
 * 个通用的数据驱动维度，我们要的是 PlotDimension / SimpleCustomDimension。
 * `DimensionFactory::create(std::string const&)` 依然是按名字查这个 map 的，
 * 所以后写入者胜。
 *
 * ## 失败时怎么办
 *
 * 所有函数都不抛异常，失败一律返回 nullopt / false / nullptr 并打日志。调用
 * 方（CustomDimensionManager）在原生路径失败时会退回旧的手抄逻辑，这样万一这
 * 台服务端的 BDS 版本对不上，行为不会比现在更差。
 */

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "mc/world/level/GeneratorType.h"

class Dimension;

namespace pier::dimensions
{
    namespace native
    {
        /** 引擎侧的 DimensionManager 是否拿得到（Level 已开 = true）。 */
        bool available();

        /**
         * 用引擎原生流程注册一个自定义维度。
         *
         * @param name    维度名（同时是工厂 map 的 key）
         * @param minY    世界底部，写进 DimensionDefinition
         * @param maxY    世界顶部
         * @param gen     生成器类型；我们自己接管 createGenerator，所以这里只
         *                影响引擎对该维度的一些默认判断，填 Flat 最保险
         * @return        引擎分配的维度 id；失败返回 nullopt
         */
        std::optional<int>
        registerCustomDimension(std::string const& name, int minY, int maxY, GeneratorType gen);

        /** 问引擎要某个名字的 id。未注册返回 nullopt。 */
        std::optional<int> engineDimensionId(std::string const& name);

        /** 引擎认为这个 id 当前有效吗。 */
        bool isActive(int dimId);

        /**
         * 按**名字**把维度对象逼出来（原生路径）。
         *
         * 之所以有这个而不是直接用 id：id -> 名字的反查在引擎内部走
         * NameIdStore，而按名字进去可以少一次反查，故障面更小。返回的裸指针由
         * DimensionRegistry 持有，调用方不要缓存。
         */
        Dimension* getOrCreateByName(std::string const& name);
    } // namespace native

    // ─────────────── 宿主侧 name <-> id 台账 ───────────────
    //
    // 注册成功时记一笔，之后维度桥的两个面（selectorNameOf / blockSourceOf）
    // 和 md_get_dimension_id 都优先从这里查。它的数据来源是「引擎实际返回的
    // id」，所以不存在私有镜像跟引擎漂移的问题 —— 那正是早期版本按配置文件
    // 解析维度名会失败的原因。

    void rememberDimension(std::string const& name, int id);
    std::string dimensionNameOf(int id);      // 查不到返回空串
    int dimensionIdOf(std::string_view name); // 查不到返回 -1
    void forEachRegisteredDimension(std::function<void(std::string const&, int)> const& fn);

    /** 供日志用：把台账拍平成 "name=id, name=id"。 */
    std::string describeRegisteredDimensions();
} // namespace pier::dimensions

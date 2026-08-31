#pragma once

/**
 * native_dimensions.h —— BDS 26.20 引擎原生自定义维度接口的封装。
 *
 * 本宿主只走原生这一条路。老的 FakeDimensionId 方案（改写出站包的维度 id、拦掉
 * DimensionDataPacket、切维度前先假装去一趟下界）已整体删除，两者互斥。也不要用
 * MoreDimensions 那套：26.20 上 VanillaDimensions::DimensionMap() 和 mFactoryMap 都
 * 不再是 getOrCreateDimension 的数据源，引擎拿 id 建维度时走
 * DimensionManager::mDimensionNameIdStore 反查名字，那里没有对应条目时它只返回
 * expired 的 WeakRef，那就是 blockSourceOf 返回 nullptr、报「传送失败」的原因。
 *
 * registerCustomDimension() 把注册交还给引擎：先保证 DimensionDefinitionGroup 里有
 * 定义，再调 serverRegisterCustomDimension() 拿引擎分配的 id，id 由引擎写进存档的
 * NameIdStore、重启自动带回。拿到 id 之后仍要覆盖 mFactoryMap 那一条，因为引擎默认
 * 建的是通用数据驱动维度，而 DimensionFactory::create 按名字查这个 map，后写入者胜。
 * 所有函数都不抛，失败一律返回 nullopt / false / nullptr 并打日志。
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
         * @param gen     生成器类型；createGenerator 由本包接管，所以这里只
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
         * 按名字把维度对象逼出来（原生路径）。
         *
         * 之所以有这个而不是直接用 id：id -> 名字的反查在引擎内部走
         * NameIdStore，而按名字进去可以少一次反查，故障面更小。返回的裸指针由
         * DimensionRegistry 持有，调用方不要缓存。
         */
        Dimension* getOrCreateByName(std::string const& name);
    } // namespace native

    //  宿主侧 name <-> id 台账
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

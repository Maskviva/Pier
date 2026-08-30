/**
 * CustomDimensionManager.cpp —— 维度注册的全过程。
 *
 * 这个文件里几乎每一段都是一次线上事故的结论。按发生顺序读：
 *
 *   1. 只改 `VanillaDimensions::DimensionMap` 和工厂 map 就以为注册好了 ——
 *      26.20 的权威是引擎的 `NameIdStore`，注册返回 3、传送却失败；
 *   2. 先注册、后写工厂闭包 —— 引擎在闭包缺席时完成注册，注册表里多出一条
 *      指向空的条目，外面一圈 `catch(...)` 把异常吞了，日志上什么都没有；
 *   3. 按 `customDimensionMap.size()` 分配 id —— 任何一条配置加载失败，
 *      运行期 id 和配置就对不上，下游报「维度 N 未注册」；
 *   4. 配置里 SNBT 坏掉就 `continue` —— id 被丢掉，下次开服重新分配，
 *      玩家存档里的 DimensionId 当场失效；
 *   5. 走原生路径时还去改 `VanillaDimensions::Undefined()` —— 引擎自己在用
 *      这个哨兵做判断，改了它引擎内部的比较全乱。
 *
 * 每一条的修法都写在对应位置。
 */
#include "pier/dimensions/dim/custom_dimension_manager.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "magic_enum.hpp"

#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/memory/Memory.h"
#include "ll/api/service/Bedrock.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/server/DedicatedServer.h"
#include "mc/server/PropertiesSettings.h"
#include "mc/util/BidirectionalUnorderedMap.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/GeneratorType.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "mc/world/level/storage/LevelStorage.h"

#include "pier/dimensions/base/native_dimensions.h"
#include "pier/dimensions/base/simple_custom_dimension.h"
#include "pier/dimensions/dim/chunk_trace.h"
#include "pier/dimensions/dim/custom_dimension_config.h"
#include "pier/dimensions/dim/dimension_height.h"
#include "pier/dimensions/dim/dimension_rules.h"

#include "pier/support/log.h"

namespace pier::dimensions
{
    namespace
    {
        using ::pier::hostLogger;

        /**
         * `SimpleCustomDimension` 把生成器类型以 magic_enum 名字的形式写进
         * payload；`PlotDimension` 没有这一项（它自己接管 `createGenerator`）。
         *
         * 这条注释以前写的是「这个值**不**决定地形长什么样」。对
         * PlotDimension 成立，对 SimpleCustomDimension **不成立** —— 它的
         * `createGenerator` 就是拿这个值做 switch 的。所以读不到时的回退不是
         * 无害的：它决定玩家进去看到的是平坦、下界还是虚空，因此每一次回退
         * 都要出声（契约 §5.1：回退要说清回退成了什么）。
         *
         * 另外要记住：这个名字是维度**第一次创建时**由 `generateNewData`
         * 写下的，之后再也不会被参数覆盖。建错了就只能改配置或删维度重建。
         */
        GeneratorType readGeneratorType(CompoundTag const& nbt)
        {
            if (!nbt.contains("generatorType"))
            {
                // PlotDimension 本来就不写这一项，属于正常情况，用 debug 级别。
                hostLogger().debug("维度数据里没有 generatorType，按 Flat 处理");
                return GeneratorType::Flat;
            }
            std::string stored;
            try
            {
                auto const name = static_cast<std::string_view>(nbt.at("generatorType"));
                stored.assign(name);
                if (auto parsed = magic_enum::enum_cast<GeneratorType>(name)) return *parsed;
            }
            catch (...)
            {
                // 读不出来和读出来但解析不了，对调用方是同一件事：拿不到生成器
                // 类型。两条路合并到下面那条 error，日志里带上读到的原文。
                stored.clear();
            }
            hostLogger().error(
                "维度数据里的 generatorType='{}' 无法解析，退回 Flat —— 地形会和创建时选的不一样",
                stored
            );
            return GeneratorType::Flat;
        }

        /** 同一个维度名只播报一次「就绪」。热重载会重复调 addDimension。 */
        void announceReady(std::string const& name, int id)
        {
            static std::mutex mtx;
            static std::unordered_set<std::string> announced;
            {
                std::lock_guard lock{mtx};
                if (!announced.insert(name).second) return;
            }
            hostLogger().info("维度 '{}' 就绪：id {}", name, id);
        }
    } // namespace

    namespace hook_list
    {
        using ll::memory::HookPriority;

        /**
         * 跨维度坐标换算。原版三维度之间的比例换算（主世界 <-> 下界 1:8）由
         * 引擎自己算；只要有一头是自定义维度，就原样搬过去 —— 自定义维度没有
         * 「与主世界的比例」这个概念，任何换算都是瞎猜。
         */
        LL_TYPE_STATIC_HOOK(
            VanillaDimensionsConvertPointHook,
            HookPriority::Normal,
            VanillaDimensions,
            VanillaDimensions::convertPointBetweenDimensions,
            bool,
            Vec3 const& oldPos,
            Vec3& toPos,
            DimensionType oldDim,
            DimensionType toDim,
            DimensionConversionData const& data
        )
        {
            if (oldDim <= 2 && toDim <= 2) return origin(oldPos, toPos, oldDim, toDim, data);
            toPos = oldPos;
            return true;
        };

        /*
         * 不要给 fromSerializedInt 再挂第二个 hook。
         *
         * 上游 MoreDimensions 把这个函数挂了两次（...Hook 和 ...HookI），因为
         * 在它面向的那一代 BDS 上，确实存在两个 mangled 形状相同的重载。
         * 26.20 的 SDK 里只有一个可挂的重载 ——
         * `Bedrock::Result<DimensionType>(Bedrock::Result<int>&&)`。另一个
         * `DimensionType fromSerializedInt(int)` 是 MCFOLD：链接器把它折叠进了
         * 别处一个字节相同的函数体，挂它等于 detour 一堆不相干的函数。
         *
         * 而把**可挂的那一个**注册两次，第二个 detour 会蹦床进第一个，
         * `origin()` 从此不再是代码以为的那个意思。
         */
        LL_TYPE_STATIC_HOOK(
            VanillaDimensionsFromSerializedIntHook,
            HookPriority::Normal,
            VanillaDimensions,
            VanillaDimensions::fromSerializedInt,
            Bedrock::Result<DimensionType>,
            Bedrock::Result<int>&& dim
        )
        {
            // 上一版有两个问题：
            //
            //  1. 无条件 `*dim`。Bedrock::Result 里装的可能是错误，这时候解
            //     引用是未定义行为 —— 存档里一个坏字段就能让服务端崩在这里。
            //  2. 连 0/1/2 都不走 origin，等于把原版三维度的反序列化也接管了。
            //
            // 判据也换了：走原生路径之后权威是引擎的 NameIdStore（我们的台账
            // 是它的镜像），DimensionMap 降级成给 fromString / dimensionSelector
            // 用的兜底镜像。
            if (!dim) return origin(std::move(dim));

            int const value = *dim;
            if (value >= 0 && value <= 2) return origin(std::move(dim));

            if (!dimensionNameOf(value).empty()) return DimensionType{value};
            if (VanillaDimensions::DimensionMap().mLeft.contains(DimensionType{value}))
            {
                return DimensionType{value};
            }
            return VanillaDimensions::Undefined();
        };

        /**
         * 玩家上次退出时所在的维度这次没注册上（配置被删、注册失败……）时，
         * 把 Y 顶到哨兵值让引擎重新找落点，而不是把人放进一个不存在的维度。
         */
        LL_TYPE_INSTANCE_HOOK(
            LevelStorageLoadServerPlayerDataHook,
            HookPriority::Normal,
            LevelStorage,
            &LevelStorage::loadServerPlayerData,
            std::unique_ptr<class CompoundTag>,
            Player const& client,
            bool isXboxLive
        )
        {
            auto result = origin(client, isXboxLive);
            if (!result) return result;

            if (!result->contains("DimensionId")) return result;
            if (!result->contains("Pos")) return result;

            int savedDim = 0;
            try
            {
                savedDim = static_cast<int>(result->at("DimensionId"));
            }
            catch (...)
            {
                // 读不出维度号就当这条存档没说过维度：原样返回，让引擎按它自己
                // 的默认流程处理。这里**不能**假定是主世界 —— 那正是契约 §5.1
                // 反对的那种补默认值。
                return result;
            }

            // 判据同 fromSerializedInt：先问引擎台账，DimensionMap 只兜底，
            // 原版三维度直接放行。
            bool const known = (savedDim >= 0 && savedDim <= 2) || !dimensionNameOf(savedDim).empty()
                || VanillaDimensions::DimensionMap().mLeft.contains(DimensionType{savedDim});
            if (!known)
            {
                hostLogger().warn("玩家存档里的维度 {} 当前不可用，重置落点", savedDim);
                result->at("Pos")[1] = FloatTag{0x7fff};
            }
            return result;
        }

        /*
         * LL_AUTO_* 在静态初始化期就把自己装上了 —— 这正是要的，因为
         * `initializeHttp` 跑在任何维度注册之前很久。所以它**不能**再出现在
         * 下面的 HookRegistrar 里：旧代码两处都列了，detour 被装了两次，
         * unhook 时引用计数回不到零。
         */
        LL_AUTO_TYPE_INSTANCE_HOOK(
            PropertiesSettingsClientSideGenHook,
            HookPriority::Normal,
            DedicatedServer,
            &DedicatedServer::initializeHttp,
            void,
            PropertiesSettings const& properties
        )
        {
            auto& mutableProperties = const_cast<PropertiesSettings&>(properties);
            mutableProperties.mClientSideGenerationEnabled = false;
            return origin(mutableProperties);
        }

        using HookReg = ll::memory::HookRegistrar<
            VanillaDimensionsConvertPointHook,
            VanillaDimensionsFromSerializedIntHook,
            LevelStorageLoadServerPlayerDataHook>;
    } // namespace hook_list

    struct CustomDimensionManager::Impl
    {
        std::atomic<int> mNewDimensionId{3};
        std::mutex mMapMutex;

        struct DimensionInfo
        {
            DimensionType id;
            CompoundTag nbt;
        };

        /** name -> {id, payload}，能完整恢复的每一条。 */
        std::unordered_map<std::string, DimensionInfo> customDimensionMap;

        /**
         * 配置里有这一条、但 SNBT 载荷解析不了的名字。**id 保留**，绝不会被
         * 分给别的维度；载荷在下一次 addDimension 时重新生成。
         *
         * 这是「id 与配置失配」的修法：旧代码在这里直接 `continue`，于是这条
         * 从 customDimensionMap 里消失、却仍留在 dimensionList 里，而 id 又是
         * 按 `3 + customDimensionMap.size()` 分配的 —— 结果是运行期 id 与配置
         * 对不上，dimensionList 查不到，下游表现为「传送失败：维度 N 未注册」。
         */
        std::unordered_map<std::string, int> salvagedIds;

        /** 配置声明过的所有 id，防止一次重载把同一个号发出去两次。 */
        std::unordered_set<int> usedIds;

        std::unordered_set<std::string> registeredDimension;
    };

    CustomDimensionManager::CustomDimensionManager() : impl(std::make_unique<Impl>())
    {
        std::lock_guard lock{impl->mMapMutex};
        CustomDimensionConfig::setDimensionConfigPath();
        CustomDimensionConfig::loadConfigFile();

        // id 从「配置里已声明的最大号 + 1」开始分配，**绝不从表的 size 来**。
        // 一旦有任何一条加载失败，按 size 分配立刻和配置撞号。
        int highest = 2;

        for (auto& [name, info] : CustomDimensionConfig::getConfig().dimensionList)
        {
            if (info.dimId < 3)
            {
                hostLogger().error(
                    "dimension_config：'{}' 的 id 是 {}，而自定义维度的 id 从 3 起 —— 这一条会被重新分配",
                    name, info.dimId
                );
                continue;
            }
            if (!impl->usedIds.insert(info.dimId).second)
            {
                hostLogger().error(
                    "dimension_config：id {} 被不止一个维度声明，'{}' 会被重新分配", info.dimId, name
                );
                continue;
            }
            highest = std::max(highest, info.dimId);

            auto nbtTag = CompoundTag::fromSnbt(info.sNbt);
            if (!nbtTag)
            {
                hostLogger().error(
                    "dimension_config：'{}'（id {}）的数据读不出来 —— 保住 id，注册时重新生成数据",
                    name, info.dimId
                );
                impl->salvagedIds.emplace(name, info.dimId);
                continue;
            }
            impl->customDimensionMap.emplace(name, Impl::DimensionInfo{DimensionType{info.dimId}, *nbtTag});
        }

        impl->mNewDimensionId.store(highest + 1);

        // 26.20 起自定义维度由引擎原生支持，FakeDimensionId 那一整套包改写
        // 已经删除：客户端通过 DimensionDataPacket 真正认识这些维度，区块、
        // 子区块、切换都带真实维度 id 走原版流程。
        hook_list::HookReg::hook();

        // 排查用，默认不装；见 chunk_trace.h。
        registerChunkTraceHooks();

        // 按维度生效的行为规则。**无条件装**：没有设过规则的维度会直接
        // origin()，所以装上去对原版维度没有任何影响。
        registerDimensionRuleHooks();
    }

    CustomDimensionManager::~CustomDimensionManager()
    {
        unregisterDimensionRuleHooks();
        unregisterChunkTraceHooks();
        hook_list::HookReg::unhook();
    }

    CustomDimensionManager& CustomDimensionManager::getInstance()
    {
        static CustomDimensionManager instance{};
        return instance;
    }

    DimensionType CustomDimensionManager::addDimension(
        std::string const& dimName,
        std::function<DimensionFactoryT> factory,
        std::function<CompoundTag()> const& data
    )
    {
        std::lock_guard lock{impl->mMapMutex};

        if (!ll::service::getLevel())
        {
            throw std::runtime_error("Level 尚未就绪，无法注册维度 " + dimName);
        }

        Impl::DimensionInfo info;

        // ── 1. 先把业务数据（seed / layout 等）准备好 ────────────────────
        //
        // payload 必须在分配 id 之前拿到：原生注册要从里头读生成器类型才能
        // 构造 DimensionDefinition。

        bool const knownLocally = impl->customDimensionMap.contains(dimName);
        if (knownLocally)
        {
            info = impl->customDimensionMap.at(dimName);
        }
        else if (auto salvaged = impl->salvagedIds.find(dimName); salvaged != impl->salvagedIds.end())
        {
            // 配置里这条的 SNBT 坏了，但 id 还留着。重新生成数据、保住 id，
            // 玩家存档里的 DimensionId 就不会失效。
            info.id = DimensionType{salvaged->second};
            info.nbt = data();
            impl->salvagedIds.erase(salvaged);
            hostLogger().warn("维度 '{}'：数据已丢失，重新生成，沿用 id {}", dimName, info.id.value());
        }
        else
        {
            info.nbt = data();
        }

        // ── 2. 工厂闭包必须在原生注册**之前**就位 ──────────────────────
        //
        // 这是顺序问题，也是「客户端一连就崩」的根因。
        //
        // `serverRegisterCustomDimension` 内部会走到
        // `_registerCustomDimensionWithFactory` -> `DimensionFactory::create(name)`，
        // 而 `create()` 是拿**名字**去 `mFactoryMap` 里查闭包的。旧代码先注册、
        // 后写 map，新进程里那一刻 map 中根本没有这个名字：引擎在闭包缺席的
        // 情况下完成了注册，DimensionRegistry 里多出一条指向空的条目，而外面
        // 那圈 catch(...) 把异常吞掉了，日志上什么都看不见。玩家一进来碰到
        // 这个维度，服务端喂出去的数据就是坏的。
        //
        // `info.id` 此刻可能还没定（全新维度），所以闭包捕获一个共享的
        // DimensionInfo，拿到引擎分配的 id 之后再回填；万一引擎在回填之前就
        // 回调了闭包，闭包自己再向引擎要一次 id 兜底。
        auto shared = std::make_shared<Impl::DimensionInfo>(info);

        // insert_or_assign 而不是 emplace：同一次开服里的第二次注册（或热重载
        // 之后）必须**替换**掉旧闭包，否则引擎会拿上一轮的闭包去建维度。
        ll::service::getLevel()->getDimensionFactory().mFactoryMap.insert_or_assign(
            dimName,
            [dimName, shared, factory = std::move(factory)](
                DerivedDimensionArguments&& arguments) -> OwnerPtr<Dimension>
            {
                DimensionType id = shared->id;
                if (id.value() < 3)
                {
                    // 还没回填 —— 说明我们正处在 serverRegisterCustomDimension
                    // 内部的重入调用里。直接问引擎。
                    if (auto engineId = native::engineDimensionId(dimName))
                    {
                        id = DimensionType{*engineId};
                    }
                    else
                    {
                        hostLogger().error(
                            "维度 '{}' 的工厂被调用时 id 还没确定，且引擎侧也查不到 —— 拒绝建维度，"
                            "不能拿一个默认值（很可能是主世界 0）去建",
                            dimName
                        );
                        return {};
                    }
                }
                return factory(DimensionFactoryInfo{arguments, shared->nbt, id});
            }
        );

        // ── 3. id 从哪来 ──────────────────────────────────────────────
        //
        // 优先让引擎分配。BDS 26.20 起 DimensionManager 自带 NameIdStore，
        // id 存进存档、由引擎持久化，`getOrCreateDimension` 也只认它 ——
        // 这正是之前「注册返回 3、传送却失败」的根因：我们只改了
        // VanillaDimensions::DimensionMap 和工厂 map，NameIdStore 里没有条目，
        // 引擎拿 id 3 反查不到名字，建不出维度。
        //
        // 原生路径失败才退回旧的手抄分配逻辑，行为不会比现在更差。

        auto const nativeId =
            native::registerCustomDimension(dimName, kWorldMinY, kWorldMaxY, readGeneratorType(info.nbt));

        bool const usedNative = nativeId.has_value();

        if (usedNative)
        {
            if (knownLocally && info.id.value() != *nativeId)
            {
                // 引擎给的 id 跟配置里记的不一样：以引擎为准，并把配置改过来。
                // 旧 id 只可能出现在上一版手抄逻辑写下的配置里，那些 id 从来
                // 就没在引擎侧生效过，所以没有存档兼容性问题。
                hostLogger().warn(
                    "维度 '{}'：配置里记的 id 是 {}，引擎分配的是 {}，以引擎为准",
                    dimName, info.id.value(), *nativeId
                );
            }
            info.id = DimensionType{*nativeId};
            impl->usedIds.insert(*nativeId);
        }
        else if (!knownLocally && info.id.value() < 3)
        {
            int candidate = impl->mNewDimensionId.fetch_add(1);
            while (!impl->usedIds.insert(candidate).second)
            {
                candidate = impl->mNewDimensionId.fetch_add(1);
            }
            info.id = DimensionType{candidate};
            hostLogger().error(
                "维度 '{}' 无法走引擎原生注册，退回旧路径分配 id {} —— "
                "26.20 上这条路径不足以让 getOrCreateDimension 建出维度，"
                "函数末尾的实例校验大概率会直接判定注册失败并抛出",
                dimName, candidate
            );
        }
        else
        {
            hostLogger().warn("维度 '{}' 无法走引擎原生注册，沿用已有 id {}", dimName, info.id.value());
        }

        // 回填。闭包里读的是这一份，所以必须在任何维度被真正创建之前更新。
        shared->id = info.id;
        shared->nbt = info.nbt;

        impl->customDimensionMap.insert_or_assign(dimName, info);
        rememberDimension(dimName, info.id.value());

        ll::memory::modify(VanillaDimensions::DimensionMap(), [&](auto& dimMap)
        {
            // BidirectionalUnorderedMap 的 insert_or_assign 只覆盖它碰到的
            // 那两条。如果这个名字之前映射到别的 id，旧的 id->name 条目会留在
            // mLeft 里，继续解析成一个已经不存在的维度。
            if (auto it = dimMap.mRight.find(dimName); it != dimMap.mRight.end())
            {
                if (it->second.value() != info.id.value()) dimMap.mLeft.erase(it->second);
            }
            dimMap.insert_or_assign(dimName, info.id);
        });

        // Undefined() 只有走旧路径时才需要顶上去：那套逻辑靠「Undefined 永远
        // 大于所有已分配 id」来判断一个 id 是不是自定义维度。
        //
        // 走原生路径时**不能**动它。26.20 的引擎自己在用这个哨兵值做判断，
        // 把它改成一个看起来像真实维度的数字，引擎内部的比较就全乱了。上游
        // MoreDimensions 那个改写是针对没有原生支持的旧版本的补偿手段，在这
        // 一版属于有害无益。
        int const nextFree = std::max(impl->mNewDimensionId.load(), info.id.value() + 1);
        impl->mNewDimensionId.store(nextFree);
        if (!usedNative)
        {
            ll::memory::modify(VanillaDimensions::Undefined(), [&](DimensionType& uid)
            {
                uid = DimensionType{nextFree};
            });
        }

        impl->registeredDimension.emplace(dimName);

        // 持久化。旧代码只在「它认为这个维度是新的」时才写，而且用的是
        // emplace() —— 对已存在的键是空操作。于是一条已经和运行期 id 漂移了
        // 的配置永远改不回来。改成：只要有任何一项不同就写。
        {
            auto& list = CustomDimensionConfig::getConfig().dimensionList;
            auto snbt = info.nbt.toSnbt(SnbtFormat::Minimize);
            auto cur = list.find(dimName);
            if (cur == list.end() || cur->second.dimId != info.id.value() || cur->second.sNbt != snbt)
            {
                list.insert_or_assign(dimName, CustomDimensionConfig::DimensionInfo{info.id.value(), snbt});
                if (!CustomDimensionConfig::saveConfigFile())
                {
                    hostLogger().error(
                        "dimension_config.json 写入失败；'{}' 下次开服可能拿到新的 id", dimName
                    );
                }
            }
        }

        try
        {
            ll::command::CommandRegistrar::getInstance(false).addEnumValues(
                "Dimension", {{dimName, info.id}}, Bedrock::type_id<CommandRegistry, DimensionType>()
            );
        }
        catch (...)
        {
            // 命令枚举只影响 `/execute in <name>` 这类按名字选维度的写法，
            // 维度本身照常工作 —— 所以这里降级而不是失败，但必须出声，否则
            // 「维度好好的、就是命令里打不出名字」会变成一个查不到根因的报障。
            hostLogger().warn("维度 '{}' 注册命令枚举失败（不影响维度本身，但 /execute in 用不了名字）", dimName);
        }

        try
        {
            // 只用引擎侧 NameIdStore 做判据。`VanillaDimensions::fromString`
            // 在本构建有 std::string ABI 问题，对自定义维度会回读成垃圾值
            // （实测 -1870061440），不可信，故不再用它自检。
            if (auto const engineId = native::engineDimensionId(dimName); !engineId || *engineId != info.id.value())
            {
                hostLogger().error(
                    "维度 '{}'（id {}）在引擎的 DimensionManager 里查不到 —— 传送会失败。引擎侧回读结果：{}",
                    dimName, info.id.value(),
                    engineId ? std::to_string(*engineId) : std::string{"(未注册)"}
                );
            }
            else
            {
                hostLogger().debug(
                    "维度 '{}' 自检通过：id {}，引擎侧 active={}",
                    dimName, info.id.value(), native::isActive(info.id.value())
                );
            }
        }
        catch (...)
        {
            hostLogger().warn("维度 '{}' 自检抛异常（自检失败本身不影响维度）", dimName);
        }

        // 唯一的权威判据：引擎真正建出来的那个 Dimension 自报的 id。
        // 名字->id 表、DimensionMap、我们的台账、配置文件都可能各说各话，
        // 只有这个对象是玩家真正会被传送进去的东西。
        auto* probe = native::getOrCreateByName(dimName);
        if (!probe)
        {
            hostLogger().error("维度 '{}' 注册后建不出实例 —— 判定注册失败", dimName);
            throw std::runtime_error("维度 '" + dimName + "' 无法实例化");
        }

        int const realId = probe->getDimensionId().value();
        if (realId != info.id.value())
        {
            hostLogger().error(
                "维度 '{}' 台账 id 是 {}，但引擎建出来的实例 id 是 {} —— "
                "把玩家传进 {} 会让引擎在区块线程上抛异常直接 abort。判定注册失败。",
                dimName, info.id.value(), realId, info.id.value()
            );
            // 台账已经写脏了，回滚掉，免得 dimensionSelector 还能查到它。
            rememberDimension(dimName, -1);
            throw std::runtime_error("维度 '" + dimName + "' 的 id 与引擎实例不符");
        }

        announceReady(dimName, realId);
        return info.id;
    }
} // namespace pier::dimensions

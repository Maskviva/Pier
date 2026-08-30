#include "pier/dimensions/base/native_dimensions.h"

#include <cstdlib>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "ll/api/service/Bedrock.h"

#include "mc/world/level/DimensionManager.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionDefinitionGroup.h"
#include "mc/world/level/dimension/DimensionType.h"
#include "mc/world/level/dimension/VanillaDimensions.h"

#include "pier/support/log.h"

namespace pier::dimensions
{
    namespace
    {
        using pier::hostLogger;

        /**
         * 「告诉客户端的高度」——**只**影响写进 DimensionDefinition 的那一份，
         * 不影响服务端实际生成和校验用的 Dimension::mHeightRange。
         *
         * 这是一个诊断用的旋钮，不是功能。实测结果是：同一份 -64..320 的定
         * 义，主世界客户端请求子区块 -4..4（正确），自定义维度客户端请求
         * -32..-24（错 28 个子区块）。客户端把这个维度的底部当成了子区块 -32
         * （y=-512）。
         *
         * 现在只有这**一个**数据点，能拟合它的公式不止一条，光靠推理分不出
         * 来。所以把这一份值做成可以从环境变量覆盖的，跑第二次拿第二个数据
         * 点，两点定一条线。
         *
         *   PIER_DIM_DEF_MIN / PIER_DIM_DEF_MAX
         *
         * 安全性：这个高度**不会被持久化** —— dimension_config.json 里存的是种
         * 子和布局，维度定义每次开服都重建。改了再改回来即可，存档里的方块不受
         * 影响。
         */
        std::pair<int, int> advertisedRange(int minY, int maxY)
        {
            static auto const override_ = []() -> std::optional<std::pair<int, int>>
            {
                auto const* lo = std::getenv("PIER_DIM_DEF_MIN");
                auto const* hi = std::getenv("PIER_DIM_DEF_MAX");
                if (!lo || !hi) return std::nullopt;
                try
                {
                    return std::pair<int, int>{std::stoi(lo), std::stoi(hi)};
                }
                catch (...)
                {
                    return std::nullopt;
                }
            }();

            if (!override_) return {minY, maxY};

            hostLogger().warn(
                "诊断开关生效：告诉客户端的高度被覆盖为 {}..{}（服务端实际仍然是 {}..{}）。"
                "这只用于定位子区块索引对不上的问题，排查完请取消这两个环境变量。",
                override_->first, override_->second, minY, maxY
            );
            return *override_;
        }

        DimensionManager* managerOrNull()
        {
            auto level = ll::service::getLevel();
            if (!level) return nullptr;
            return &level->getDimensionManager();
        }

        std::mutex& ledgerMutex()
        {
            static std::mutex m;
            return m;
        }

        std::map<std::string, int>& ledgerByName()
        {
            static std::map<std::string, int> m;
            return m;
        }

        std::map<int, std::string>& ledgerById()
        {
            static std::map<int, std::string> m;
            return m;
        }
    } // namespace

    // ───────────────────────── 台账 ─────────────────────────

    void rememberDimension(std::string const& name, int id)
    {
        std::lock_guard lock{ledgerMutex()};

        // 同一个名字换了 id（不该发生，但如果发生了，旧的反向条目必须清掉，
        // 否则 dimensionNameOf(旧id) 会一直指向一个已经不存在的维度）。
        if (auto it = ledgerByName().find(name); it != ledgerByName().end() && it->second != id)
        {
            ledgerById().erase(it->second);
        }
        ledgerByName()[name] = id;
        ledgerById()[id] = name;
    }

    std::string dimensionNameOf(int id)
    {
        std::lock_guard lock{ledgerMutex()};
        auto it = ledgerById().find(id);
        return it == ledgerById().end() ? std::string{} : it->second;
    }

    int dimensionIdOf(std::string_view name)
    {
        std::lock_guard lock{ledgerMutex()};
        auto it = ledgerByName().find(std::string{name});
        return it == ledgerByName().end() ? -1 : it->second;
    }

    void forEachRegisteredDimension(std::function<void(std::string const&, int)> const& fn)
    {
        std::lock_guard lock{ledgerMutex()};
        for (auto const& [name, id] : ledgerByName()) fn(name, id);
    }

    std::string describeRegisteredDimensions()
    {
        std::string out;
        forEachRegisteredDimension([&](std::string const& name, int id)
        {
            if (!out.empty()) out += ", ";
            out += name + "=" + std::to_string(id);
        });
        return out.empty() ? std::string{"(无)"} : out;
    }

    // ───────────────────────── 原生注册 ─────────────────────────

    namespace native
    {
        bool available() { return managerOrNull() != nullptr; }

        std::optional<int> engineDimensionId(std::string const& name)
        {
            auto* mgr = managerOrNull();
            if (!mgr) return std::nullopt;
            try
            {
                int const v = mgr->getDimensionId(std::string_view{name}).value();

                // 自定义维度一定 >= 3。
                if (v < 3) return std::nullopt;

                // 但 >= 3 不等于「注册过」：未注册的名字拿回来的是
                // Undefined()。早先的版本假设 Undefined() 会被运行时改写成一个
                // 很大的数，所以只判 < 3 就够；改成走引擎原生注册之后我们不再
                // 改写它，它就一直停在 3，于是每一个没注册过的名字都被当成了
                // 「已存在的 id 3」。
                if (v == ::VanillaDimensions::Undefined().value()) return std::nullopt;

                return v;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        bool isActive(int dimId)
        {
            auto* mgr = managerOrNull();
            if (!mgr) return false;
            try
            {
                return mgr->isDimensionTypeActive(DimensionType{dimId});
            }
            catch (...)
            {
                return false;
            }
        }

        std::optional<int>
        registerCustomDimension(std::string const& name, int minY, int maxY, GeneratorType gen)
        {
            auto* mgr = managerOrNull();
            if (!mgr)
            {
                hostLogger().error("原生维度注册：Level 还没开，DimensionManager 拿不到");
                return std::nullopt;
            }

            // 存档里 NameIdStore 恢复出来的 id（如果有）。
            //
            // 注意 **名字->id 表被恢复 != 维度本次会话可用**：NameIdStore 是持
            // 久化的，把维度接进 DimensionRegistry / 工厂那一步不是。所以最早
            // 那版在这里直接 return，等于每次重启后工厂绑定永远不再建立。
            auto const preexisting = engineDimensionId(name);

            // 1) 已知名字：只补工厂绑定，不碰 NameIdStore。成了就走人。
            if (preexisting)
            {
                try
                {
                    mgr->_registerCustomDimensionWithFactory(
                        std::string_view{name}, DimensionType{*preexisting});
                }
                catch (std::exception const& e)
                {
                    hostLogger().warn(
                        "维度 '{}'（id {}）补绑引擎工厂抛异常：{}", name, *preexisting, e.what());
                }
                catch (...)
                {
                    hostLogger().warn("维度 '{}'（id {}）补绑引擎工厂抛未知异常", name, *preexisting);
                }

                // 这一段以前是没有的，注释还写着「不碰 DimensionDefinitionGroup」。
                //
                // 那是个 bug。DimensionDefinitionGroup 不是持久化的 —— 它每次开
                // 服都从空的重建，只有走完下面第 2 步的维度才会往里加一条。而
                // 这条分支是「名字已经在 NameIdStore 里」时走的，也就是**第二次
                // 及以后的每一次开服**。于是：
                //
                //   第一次开服：定义进了组 -> DimensionDataPacket 带上它 -> 客户
                //               端认识这个维度 -> 区块能渲染
                //   之后每次   ：直接从这里 return -> 组里没有这条定义 -> 包里没
                //               有它 -> 客户端收到一个自己没有定义的维度 id 的区
                //               块，只能丢掉 -> 服务端一路 Loaded，玩家看到一片
                //               空白
                //
                // 所以这里必须无条件把定义补回去。已经有了就不动。
                try
                {
                    auto& group = mgr->getDimensionDefinitionGroup();
                    if (!group.getDimensionDefinition(name).has_value())
                    {
                        auto const [advMin, advMax] = advertisedRange(minY, maxY);
                        DimensionDefinitionGroup::DimensionDefinition def{
                            advMin, advMax, gen, DimensionType{*preexisting}
                        };
                        if (group.tryAddDimensionDefinition(name, def))
                        {
                            hostLogger().info(
                                "维度 '{}'（id {}）：本次开服 DimensionDefinitionGroup 里没有它的定义，"
                                "已补上（高度 {}..{}）—— 缺了这条客户端就认不出这个维度，区块会渲染不出来",
                                name, *preexisting, minY, maxY
                            );
                        }
                        else
                        {
                            hostLogger().error(
                                "维度 '{}'（id {}）的定义没能补进 DimensionDefinitionGroup —— "
                                "客户端很可能收不到这个维度的区块",
                                name, *preexisting
                            );
                        }
                    }
                }
                catch (std::exception const& e)
                {
                    hostLogger().error("维度 '{}' 补 DimensionDefinition 时抛异常：{}", name, e.what());
                }
                catch (...)
                {
                    hostLogger().error("维度 '{}' 补 DimensionDefinition 时抛未知异常", name);
                }

                // 不在这里探测。getOrCreateDimension 会真的把维度建出来，而这一
                // 刻 id 还没定稿（调用方的工厂闭包读的是旧的 shared->id）；早先
                // 就是在这里留下了一个 id 用错的实例，后面无论怎么改 id 都盖不
                // 掉它。能不能建得出来，交给调用方在回填 id 之后统一验一次。
                rememberDimension(name, *preexisting);
                return preexisting;
            }

            // 2) serverRegisterCustomDimension 是按名字去 DimensionDefinitionGroup
            //    取几何信息的，所以定义必须先在组里。行为包里的 JSON 维度走的
            //    也是这条路，我们只是手动补一条等价的定义。
            //
            //    这个组会被 DimensionDataPacket 整个发给客户端 —— 这正是原生路
            //    径能工作的原因：客户端由此真正知道有这么一个维度、它多高、用哪
            //    种生成器，于是能接收带真实维度 id 的区块和子区块。
            //
            //    **千万不要再去拦截 DimensionDataPacket。** 老的 FakeDimensionId
            //    方案（已删除）把它无条件丢掉，同时把所有包的维度 id 改写成 0；
            //    那套逻辑和这里是互斥的，两个一起开的症状就是：切维度加载极慢、
            //    加载完客户端闪退、重进后区块全空（服务端有方块，只有
            //    UpdateBlock 这种不带维度字段的包才漏得过去）。
            try
            {
                auto& group = mgr->getDimensionDefinitionGroup();
                if (!group.getDimensionDefinition(name).has_value())
                {
                    // 已经知道 id 就直接填进去，比让引擎回写靠谱；全新维度才用
                    // -1 占位（JSON 加载的维度同样没法预先知道 id）。
                    auto const [advMin, advMax] = advertisedRange(minY, maxY);
                    DimensionDefinitionGroup::DimensionDefinition def{
                        advMin,
                        advMax,
                        gen,
                        DimensionType{preexisting ? *preexisting : -1}
                    };
                    if (!group.tryAddDimensionDefinition(name, def))
                    {
                        hostLogger().warn(
                            "维度 '{}' 的定义没能加进 DimensionDefinitionGroup；"
                            "继续尝试注册，失败的话会退回旧路径",
                            name
                        );
                    }
                }
            }
            catch (std::exception const& e)
            {
                hostLogger().error("维度 '{}' 写入 DimensionDefinitionGroup 时抛异常：{}", name, e.what());
                return std::nullopt;
            }
            catch (...)
            {
                hostLogger().error("维度 '{}' 写入 DimensionDefinitionGroup 时抛未知异常", name);
                return std::nullopt;
            }

            // 3) 正式注册。引擎分配 id、写 NameIdStore、登记工厂。
            std::optional<DimensionType> assigned;
            try
            {
                assigned = mgr->serverRegisterCustomDimension(std::string_view{name});
            }
            catch (std::exception const& e)
            {
                hostLogger().error("serverRegisterCustomDimension('{}') 抛异常：{}", name, e.what());
                return std::nullopt;
            }
            catch (...)
            {
                hostLogger().error("serverRegisterCustomDimension('{}') 抛未知异常", name);
                return std::nullopt;
            }

            if (!assigned)
            {
                // 名字已经在 NameIdStore 里时，引擎很可能就是返回空（「已经注册
                // 过了」）。这种情况下把 id 丢掉是错的 —— 上层会退回手抄分配逻
                // 辑，换一个跟存档对不上的 id，玩家已经建好的东西就全废了。
                if (preexisting)
                {
                    hostLogger().warn(
                        "serverRegisterCustomDimension('{}') 返回空，但 NameIdStore 里已有 id {}，"
                        "沿用它（引擎侧 active={}）",
                        name, *preexisting, isActive(*preexisting)
                    );
                    rememberDimension(name, *preexisting);
                    return preexisting;
                }

                hostLogger().error(
                    "serverRegisterCustomDimension('{}') 返回空 —— 引擎拒绝了这次注册。"
                    "常见原因是调用时机太早（NameIdStore 还没从存档载入）或太晚"
                    "（维度已经全部创建完毕）",
                    name
                );
                return std::nullopt;
            }

            int const id = assigned->value();

            if (preexisting && *preexisting != id)
            {
                // 引擎给了一个跟存档里不一样的 id：存档里所有引用旧 id 的区块和
                // 玩家数据都会失联。这不是能默默咽下去的事。
                hostLogger().error(
                    "维度 '{}' 重新注册后 id 从 {} 变成了 {} —— "
                    "存档里按旧 id 存的区块和玩家位置会全部失联",
                    name, *preexisting, id
                );
            }

            // 4) 回读校验。这一步是防止「注册看起来成功了，实际 id 对不上」，
            //    也就是早先最终表现成「传送失败」的那类静默错位。
            auto const readBack = engineDimensionId(name);
            if (!readBack || *readBack != id)
            {
                hostLogger().error(
                    "维度 '{}' 注册返回 id {}，但 getDimensionId 回读得到 {} —— "
                    "引擎台账不自洽，放弃原生路径",
                    name, id, readBack ? std::to_string(*readBack) : std::string{"(无)"}
                );
                return std::nullopt;
            }

            // 5) 把真实 id 写回 DimensionDefinitionGroup 里那条定义。
            //
            //    第 2 步建定义时 mDimensionType 填的是 -1（占位，等引擎回写）。
            //    引擎在 _registerCustomDimensionWithDimensionDefinitionGroup 里
            //    通常会自己回写，但不能指望 —— 这个组会被 DimensionDataPacket
            //    整个序列化给客户端，一个 -1 的维度类型足以让客户端解析失败。走
            //    原生路径之后这个包是真的会发出去的，所以这一步现在是必需的，不
            //    再只是「让 dump 好看」。
            try
            {
                auto& defs = *mgr->getDimensionDefinitionGroup().mDimensionDefinitions;
                if (auto it = defs.find(name);
                    it != defs.end() && it->second.mDimensionType->value() != id)
                {
                    hostLogger().debug(
                        "维度 '{}' 的 DimensionDefinition 里 mDimensionType 是 {}，改写为 {}",
                        name, it->second.mDimensionType->value(), id
                    );
                    it->second.mDimensionType = DimensionType{id};
                }
            }
            catch (...)
            {
                hostLogger().warn("维度 '{}' 回写 DimensionDefinition 的 id 失败（不影响注册本身）", name);
            }

            hostLogger().debug("维度 '{}' 已由引擎原生注册，id {}（高度 {}..{}），active={}",
                               name, id, minY, maxY, isActive(id));
            rememberDimension(name, id);
            return id;
        }

        Dimension* getOrCreateByName(std::string const& name)
        {
            auto* mgr = managerOrNull();
            if (!mgr)
            {
                hostLogger().error("getOrCreateByName('{}')：Level 还没开", name);
                return nullptr;
            }

            // 失败原因分三种，混在一个 catch(...) 里根本没法排查：
            //   a) 名字在 NameIdStore 里查不到  -> 注册压根没生效
            //   b) 查得到但 active=false        -> 工厂绑定缺失（本次会话没注册）
            //   c) 以上都正常但 lock() 是空的   -> 工厂闭包本身返回了空
            auto const id = engineDimensionId(name);
            if (!id)
            {
                hostLogger().error("getOrCreateByName('{}')：引擎 NameIdStore 里没有这个名字", name);
                return nullptr;
            }
            // active=false 不能当拦路条件：观测下来维度实例还没建出来时它就是
            // false，而 getOrCreateDimension 的职责恰恰是把它建出来。早先的版本
            // 在这里 return，等于自己把唯一一次真正的尝试挡掉了。只记一笔。
            if (!isActive(*id))
            {
                hostLogger().debug(
                    "getOrCreateByName('{}')：id {} 当前 active=false，仍然尝试创建", name, *id);
            }

            try
            {
                auto ref = mgr->getOrCreateDimension(std::string_view{name});
                auto ptr = ref.lock();
                if (!ptr)
                {
                    hostLogger().error(
                        "getOrCreateByName('{}')：id {} 已就绪，但 getOrCreateDimension 拿到的是空引用 —— "
                        "mFactoryMap 里那个闭包返回了空，检查工厂是否在注册之前就位",
                        name, *id
                    );
                    return nullptr;
                }
                return &*ptr;
            }
            catch (std::exception const& e)
            {
                hostLogger().error("getOrCreateByName('{}') 抛异常：{}", name, e.what());
                return nullptr;
            }
            catch (...)
            {
                hostLogger().error("getOrCreateByName('{}') 抛未知异常", name);
                return nullptr;
            }
        }
    } // namespace native
} // namespace pier::dimensions

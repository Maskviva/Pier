/**
 * pier-dimensions/rt/Bridge.cpp —— `spi::DimensionBridge` 的**唯一实现**。
 *
 * api 侧（core/Bridge.cpp）对自定义维度一无所知：它只知道「id >= 3 的东西
 * 得问桥」。为什么必须这么分，两个方向各有一条理由：
 *
 *   - 往下：解析一个自定义维度名要同时看注册台账、引擎的 DimensionMap 和配
 *     置镜像三层数据源，还要知道哪些引擎 API 不能碰（见下面 toString 那
 *     段）。这些知识只在本包里成立，塞进 api 会让「开放接口」的包依赖一个可
 *     选能力。
 *   - 往上：本包不编入时桥缺席，api 侧只认原版三维度并各打一次告警。降级是
 *     可预期的行为，不是崩溃 —— 这正是「可选能力」四个字要求的。
 *
 * # 这个文件承担的那道安全闸
 *
 * `blockSourceOf` **必须**校验引擎实际建出来的那个 Dimension 自报的 id 是否
 * 等于请求的 dim（spi.h §6 明文要求）。台账 id 与引擎 id 一旦漂移：
 *
 *   - 方块写入会静默落进**错的维度**（调用方拿到一个有效的 BlockSource，看
 *     不出任何异常）；
 *   - 把玩家传送进去会让引擎在区块工作线程上抛未捕获异常，整个进程
 *     fastfail(0xC0000409) —— 那不是调用方一句「传送失败」能兜住的。
 *
 * 校验放在这一头，是因为「按名字把维度逼出来、再问它自己的 id」这件事只有本
 * 包做得到。api 侧把「blockSourceOf 返回非空」当作唯一的放行条件。
 */
#include <optional>
#include <set>
#include <string>
#include <string_view>

#include "ll/api/service/Bedrock.h"

#include "mc/util/BidirectionalUnorderedMap.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionType.h"
#include "mc/world/level/dimension/VanillaDimensions.h"

#include "pier/dimensions/base/native_dimensions.h"
#include "pier/dimensions/dim/custom_dimension_config.h"

#include "pier/host/spi.h"
#include "pier/support/log.h"

namespace pier::dimensions::rt
{
    namespace
    {
        /**
         * 名字 → id 的三层数据源，按可信度排列。
         *
         * 1. **注册台账**（`dimensionIdOf`）—— 数据来源是「引擎实际返回的
         *    id」，注册成功那一刻记下来的。最可信，因为它记的就是引擎的答案。
         * 2. **引擎的 DimensionMap** —— BDS 内部各处查的同一张表。
         * 3. **配置镜像** —— 只作兜底，命中时打警告（见下）。
         *
         * ## 为什么不能用 `VanillaDimensions::toString()` 做反查
         *
         * 这一段是血买来的，删掉它下一个人会再买一次。
         *
         * `VanillaDimensions::fromString()` 对未知名字返回
         * `VanillaDimensions::Undefined()`，而 `Undefined()` 会被
         * `CustomDimensionManager::addDimension` **在运行期改写**（它被维持在
         * 比最高自定义 id 大一的位置）。所以它的数值永远看起来像一个完全合理
         * 的维度 id，原样返回会让调用方相信一个不存在的维度存在。这个 bug 真
         * 的发出去过：一个探测未注册地皮维度的调用方拿回了 0（主世界），把整
         * 张地皮网格挂到了玩家的生存世界上。
         *
         * 顺手的护栏 —— 用 `toString()` 往返一次核对 —— **更糟：它会崩服**。
         * `toString()` 交回来的那个对象不符合 MSVC 的 `std::string` 布局（文
         * 本字节落在了 `_Mysize` 该在的位置），于是后面任何一个消费方都会用
         * 一个**从文本里读出来的长度**去 memcpy。线上实测：一个叫 "red" 的维
         * 度产生了 `memcpy(dst, src, 0x646572)` —— 'r','e','d' 被当成了长度
         * —— 服务器死在 VCRUNTIME140 里面。
         *
         * 所以只**直接读 DimensionMap()**：它返回的是一个 const&，指向服务器
         * 自己内存里的 map，没有构造、没有拷贝、没有任何东西跨 ABI 交出来，
         * toString 的那种失败模式因此不适用。它同时也是
         * `VanillaDimensions::fromString` 和每个 BDS 内部消费方查的那张表 ——
         * 这正是关键：私有镜像会漂移，而漂移的时候调用方会被告知一个活着的维
         * 度不存在，那就是「维度 3 没有注册」那类传送失败的来源。
         */
        int resolveIdByName(std::string const& wanted)
        {
            if (wanted.empty()) return -1;
            if (wanted == "overworld") return 0;
            if (wanted == "nether") return 1;
            if (wanted == "the_end") return 2;

            // 1. 注册台账
            if (int const id = dimensionIdOf(wanted); id >= 0) return id;

            // 2. 引擎的 DimensionMap
            {
                auto const& dimMap = ::VanillaDimensions::DimensionMap();
                auto const hit = dimMap.mRight.find(wanted);
                if (hit != dimMap.mRight.end())
                {
                    auto const id = hit->second.value();
                    // Undefined() 在运行期被改写成「比最高已分配 id 大一」，所
                    // 以它永远是一个看着合理的数。解析到它的名字就是没注册的
                    // 名字。
                    if (id >= 0 && id != ::VanillaDimensions::Undefined().value()) return id;
                }
            }

            // 3. 配置镜像（兜底）
            auto const& list = CustomDimensionConfig::getConfig().dimensionList;
            auto const it = list.find(wanted);
            if (it == list.end()) return -1;

            pier::hostLogger().warn(
                "维度 '{}' 是从配置镜像解析出来的（id {}），不是从引擎的维度表 —— "
                "两者已经漂移。传送和写方块可能落到错的地方，建议检查存档里的维度注册。",
                wanted, it->second.dimId
            );
            return it->second.dimId;
        }

        /** id → 名字。台账优先，其次引擎表。查不到给空串。 */
        std::string resolveNameById(int32_t dim)
        {
            if (dim < 0) return {};
            switch (dim)
            {
            case 0:
                return "overworld";
            case 1:
                return "nether";
            case 2:
                return "the_end";
            default:
                break;
            }

            if (auto name = dimensionNameOf(dim); !name.empty()) return name;

            auto const& dimMap = ::VanillaDimensions::DimensionMap();
            auto const hit = dimMap.mLeft.find(DimensionType{dim});
            if (hit != dimMap.mLeft.end()) return hit->second;

            return {};
        }

        /** 每个 id 只抱怨一次，别把一个循环调用刷成一场事故。 */
        bool firstComplaintFor(int32_t dim)
        {
            static std::set<int32_t> seen;
            return seen.insert(dim).second;
        }

        // ── spi::DimensionBridge 的两个面 ────────────────────────────────

        std::string bridgeSelectorNameOf(int32_t dim)
        {
            auto name = resolveNameById(dim);
            if (name.empty() && firstComplaintFor(dim))
            {
                pier::hostLogger().warn(
                    "维度 {} 解析不到名字：注册台账、引擎维度表、配置镜像三处都没有它。"
                    "已注册的是：{}",
                    dim, describeRegisteredDimensions()
                );
            }
            return name;
        }

        ::BlockSource* bridgeBlockSourceOf(int32_t dim)
        {
            auto name = resolveNameById(dim);
            if (name.empty())
            {
                if (firstComplaintFor(dim))
                {
                    pier::hostLogger().error(
                        "维度 {}：解析不到名字，建不出实例。已注册的是：{}",
                        dim, describeRegisteredDimensions()
                    );
                }
                return nullptr;
            }

            // 按名字把维度逼出来。id → 名字的反查在引擎内部走 NameIdStore，
            // 按名字进去可以少一次反查，故障面更小。
            auto* real = native::getOrCreateByName(name);
            if (!real) return nullptr; // getOrCreateByName 自己分三种原因打过日志了

            // ── 安全闸：引擎实例 id 必须等于请求的 dim ──
            //
            // 见文件头。不一致时**不能**把这个 BlockSource 交出去：调用方会拿
            // 它去写方块（静默落进错的维度），或者据此放行一次传送（区块线程
            // 未捕获异常 → 整个进程 fastfail 0xC0000409）。
            int const realId = real->getDimensionId().value();
            if (realId != dim)
            {
                pier::hostLogger().error(
                    "拒绝提供维度 {} 的 BlockSource：台账里它叫 '{}'，但引擎按这个名字建出来的"
                    "实例自报 id 是 {}。台账和引擎已经漂移 —— 继续下去要么把方块写进错的维度，"
                    "要么在传送时让区块线程抛异常直接 abort。",
                    dim, name, realId
                );
                return nullptr;
            }

            return &real->getBlockSourceFromMainChunkSource();
        }

        spi::DimensionBridge const gBridge{
            &bridgeSelectorNameOf,
            &bridgeBlockSourceOf,
        };

        /** Bootstrap：把桥挂上去。在任何 API 调用之前跑（宿主启动路径）。 */
        void bootstrap()
        {
            spi::setDimensionBridge(&gBridge);
            pier::hostLogger().debug("维度桥已挂载（自定义维度可用）");
        }

        // stage 10：桥必须在任何维度注册（stage 20+）之前挂上 —— 注册路径
        // 里的自检会反过来经桥问「引擎实际建出来的 id 是多少」。
        spi::BootstrapReg regBoot{{10, "dimension-bridge", &bootstrap}};
    } // namespace

    /** 给包内其他 TU 用的名字解析（addDimension 的自检路径要）。 */
    int idByName(std::string const& name) { return resolveIdByName(name); }
} // namespace pier::dimensions::rt

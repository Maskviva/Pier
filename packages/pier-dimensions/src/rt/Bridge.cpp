/**
 * pier-dimensions/rt/Bridge.cpp —— spi::DimensionBridge 的唯一实现。
 *
 * api 侧（core/Bridge.cpp）对自定义维度一无所知，只知道「id >= 3 的东西得问桥」。
 * 往下：解析一个自定义维度名要同时看注册台账、引擎的 DimensionMap 和配置镜像三层
 * 数据源，还要知道哪些引擎 API 不能碰（见 nameToId 上的注释），这些知识只在本包里
 * 成立。往上：本包不编入时桥缺席，api 侧只认原版三维度并各打一次告警，降级是可预
 * 期的行为。
 *
 * blockSourceOf 承担一道安全闸：必须校验引擎实际建出的 Dimension 自报的 id 等于请
 * 求的 dim（spi.h §6 明文要求）。台账 id 与引擎 id 一旦漂移，方块写入会静默落进错
 * 的维度（调用方拿到一个有效的 BlockSource，看不出异常），把玩家传送进去则会让引
 * 擎在区块工作线程上抛未捕获异常，整个进程 fastfail(0xC0000409)。校验放在这一头，
 * 因为「按名字把维度逼出来、再问它自己的 id」只有本包做得到；api 侧把
 * blockSourceOf 返回非空当作唯一放行条件。
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
         * 名字 → id 的三层数据源，按可信度排列：注册台账（dimensionIdOf，记的是引擎
         * 注册成功时实际返回的 id）、引擎的 DimensionMap、配置镜像（兜底，命中打警
         * 告）。
         *
         * 不许用 VanillaDimensions::toString() 做反查或往返核对。fromString() 对未知
         * 名字返回 Undefined()，而它会被 addDimension 在运行期改写（维持在比最高自定
         * 义 id 大一的位置），数值永远看起来像合理的维度 id，原样返回会让调用方相信
         * 一个不存在的维度存在。toString() 交回来的对象则不符合 MSVC 的 std::string
         * 布局，文本字节落在 _Mysize 的位置：一个叫 "red" 的维度会让消费方执行
         * memcpy(dst, src, 0x646572)，死在 VCRUNTIME140 里。只直接读 DimensionMap()，
         * 它返回 const&、不构造不拷贝不跨 ABI，也正是每个 BDS 内部消费方查的同一张
         * 表。
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

        //  spi::DimensionBridge 的两个面

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

            //  安全闸：引擎实例 id 必须等于请求的 dim
            //
            // 见文件头。不一致时不能把这个 BlockSource 交出去：调用方会拿
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

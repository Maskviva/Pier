#pragma once
// pier-api 包内共享的解析助手。**私有** include（契约 §一：能力包之间
// 互不 include）—— 这里的一切只服务本包各域的 TU。
//
// 收进来的判据：至少两个域要用，且属于「把 ABI 侧的引用解析成引擎对象」
// 这一件事。只有一个域用的助手留在那个域自己的 TU 里。

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "sdk/abi.h"

class Actor;
class Block;
class BlockChangeContext;
class BlockSource;
class CompoundTag;
class CompoundTagVariant;
class Container;
class ItemStack;
class Level;
class Player;

namespace pier::bridge
{
    /** Level 就绪则返回指针，否则 nullptr。服务端取 getLevel()，
     *  客户端构建取 getMultiPlayerLevel() —— 唯一一处目标分支。 */
    [[nodiscard]] Level* levelReady();

    /** 维度 id → BlockSource。已建维度直取；自定义维度（id ≥ 3）经
     *  spi::dimensionBridge 强制建出（能力包缺席时降级为只认原版并
     *  warn 一次）。失败 nullptr，调用方**必须**按失败处理（§5.1）。 */
    [[nodiscard]] BlockSource* blockSourceOf(int32_t dimId);

    /** 玩家选择器（kind: 0=账号名 1=xuid 2=uuid；账号名落空再按显示名
     *  过一遍 —— 改名牌插件改的是 NameTag）。找不到 nullptr。 */
    [[nodiscard]] Player* resolvePlayer(PierPlayerSel sel);

    /** ActorUniqueID → Actor（不含已移除的）。找不到 nullptr。 */
    [[nodiscard]] Actor* resolveActor(PierActorId id);

    /** 容器引用 → Container。which: 0=背包 1=末影箱 2=盔甲 3=手
     *  4=方块容器(dim,x,y,z)。盔甲/手是货真价实的 Container（经
     *  ActorEquipment 拿 SimpleContainer），不是快照 NBT。 */
    [[nodiscard]] Container* resolveContainer(PierContainerRef ref);

    /** 任何已注册维度 id → `/execute in` 认的名字。未知返回**空串**，
     *  调用方必须失败 —— 绝不回退主世界（回退曾把别的维度的写操作全部
     *  砸进生存主世界）。 */
    [[nodiscard]] std::string dimensionSelector(int32_t dim);

    /** 原版三维度的名字（0/1/2 之外一律按 overworld 报）。只给「肯定是
     *  原版」的展示场景用；解析用 dimensionSelector。 */
    [[nodiscard]] char const* dimensionName(int dim);

    /** 以服务器身份（Owner 权限）执行一条控制台命令。客户端构建恒 false。 */
    bool runConsoleCommand(std::string const& cmd);

    /** 玩家身份 + 位置一行：{name,xuid,uuid,dim,x,y,z}。 */
    [[nodiscard]] std::string playerSummarySnbt(Player& p);

    /** ItemStack ↔ SNBT。fromSnbt 对畸形输入不抛（W12：输入最终来自
     *  客户端），失败 nullopt 并留日志。 */
    [[nodiscard]] std::string itemToSnbt(ItemStack const& item);
    [[nodiscard]] std::optional<ItemStack> itemFromSnbt(std::string_view snbt);

    /** NBT 数值 → double；非数值给 def。 */
    [[nodiscard]] double nbtToDouble(CompoundTagVariant const& val, double def);

    /** 方块规格解析。World 与 Edit 两个 TU 共用 —— 两处解析方块的规则必须
     *  是同一套，否则 `//set` 和 `setblock` 会对同一个名字给出不同结果。
     *  定义在 world/BlockResolve.cpp（双目标；旧版把它们放在 server-only 的
     *  Edit.cpp 里、却被双目标的 World.cpp 引用 —— 客户端目标链接必断，这
     *  次把归属摆正）。 */

    /** 序列化 NBT（{name,states,version}）→ Block；跑引擎版本升级表。 */
    [[nodiscard]] Block const* blockFromTag(CompoundTag const& tag);
    /** SNBT 文本 → Block（内部走 blockFromTag）。失败 nullptr。 */
    [[nodiscard]] Block const* blockFromSnbt(std::string_view snbt);
    /** 方块名 → 默认状态。**找不到时 nullptr**，绝不给占位方块。 */
    [[nodiscard]] Block const* defaultBlockNamed(std::string_view name);
    /** 写方块用的变更来源（= commandsChange，理由见 BlockResolve.cpp）。 */
    [[nodiscard]] BlockChangeContext blockEditContext();

    /** 事件载荷富化：把 LL serializeRefObj 发出的反射指针桩
     *  `{_type_,_pointer_}` 解成消费方读得懂的字段（_player/dim/
     *  _identifier/…）。定义与理由见 Enrich.cpp 文件头。 */
    [[nodiscard]] std::string enrichEventData(CompoundTag const& data);
} // namespace pier::bridge

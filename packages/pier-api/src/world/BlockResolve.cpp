/** world/BlockResolve.cpp —— 方块规格解析与写方块的变更来源。
 *
 * 单独成 TU 的理由：World.cpp（双目标）和 Edit.cpp（服务端专属）都要用同
 * 一套解析规则。旧版把定义放在 Edit.cpp 里，World 的客户端目标一链接就缺
 * 符号 —— 归属摆正到这里，双目标编入。
 */
#include "pier/api/bridge.h"

#include <string>
#include <string_view>

#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockChangeContext.h"
#include "mc/world/level/block/block_serialization_utils/BlockSerializationUtils.h"
#include "mc/world/level/block/registry/BlockTypeRegistry.h"

namespace pier::bridge
{
    namespace
    {
        /** 补上 minecraft: 前缀。注册表里的键是带命名空间的。 */
        std::string qualify(std::string_view name)
        {
            std::string s{name};
            if (s.find(':') == std::string::npos) s = "minecraft:" + s;
            return s;
        }
    } // namespace

    Block const* blockFromTag(CompoundTag const& tag)
    {
        // tryGetBlockFromNBT 会顺带跑引擎的版本升级表：老存档里的
        // {name:"minecraft:wool",states:{color:…}} 能被正确升级成新方块。
        // 这正是我们要的 —— 手工解析 states 做不到这一步。
        auto pair = BlockSerializationUtils::tryGetBlockFromNBT(tag, nullptr);
        return pair.second;
    }

    Block const* blockFromSnbt(std::string_view snbt)
    {
        auto parsed = CompoundTag::fromSnbt(snbt);
        if (!parsed) return nullptr;
        return blockFromTag(*parsed);
    }

    /**
     * 方块名 → 默认状态。**找不到时返回 nullptr**，不返回「未知方块」。
     *
     * `getDefaultBlockState` 对不认识的名字会给一个占位方块而不是报错，
     * 于是 `//set 拼错的名字` 会安静地把整片地区填成那个占位方块。
     * 这里比对一次 type_name 把它挡住 —— 调用方拿到 false 可以回落到
     * 命令路径去拿一句真正的错误信息。
     */
    Block const* defaultBlockNamed(std::string_view name)
    {
        std::string full = qualify(name);
        auto const& block = BlockTypeRegistry::get().getDefaultBlockState(HashedString{full}, false);
        if (block.getTypeName() != full) return nullptr;
        return &block;
    }

    /**
     * 写方块时用哪个「变更来源」。
     *
     * 用 `commandsChange()` 而不是 `structureChange()`：这条路取代的正是
     * `/setblock`，保持同一个来源意味着**别的插件挂在方块变更上的钩子看到
     * 的东西不变**。换成 structure 会让一部分保护插件突然不再拦截 ——
     * 那种「换了个实现，别人的插件失效了」的坑，值得用一段注释钉住。
     */
    BlockChangeContext blockEditContext() { return BlockChangeContext::commandsChange(); }
} // namespace pier::bridge

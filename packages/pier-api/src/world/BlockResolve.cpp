/** world/BlockResolve.cpp: block specification parsing and the change source used
 *  when writing blocks.
 *
 * It is a TU of its own because World.cpp, compiled into both targets, and Edit.cpp,
 * which is server only, need the same parsing rules. Defining them in Edit.cpp would
 * leave the client target of World.cpp with an unresolved symbol at link time, so
 * they live here and are compiled into both targets.
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
        /** Adds the minecraft: prefix. Registry keys carry a namespace. */
        std::string qualify(std::string_view name)
        {
            std::string s{name};
            if (s.find(':') == std::string::npos) s = "minecraft:" + s;
            return s;
        }
    } // namespace

    Block const* blockFromTag(CompoundTag const& tag)
    {
        // tryGetBlockFromNBT also runs the engine upgrade table, so an old save
        // holding {name:"minecraft:wool",states:{color:...}} is upgraded to the
        // current block correctly. That is what is wanted here and parsing states by
        // hand cannot do it.
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
     * Block name to default state. Returns nullptr when the name is not found, rather
     * than an unknown block.
     *
     * `getDefaultBlockState` answers an unrecognized name with a placeholder block
     * instead of an error, so a set operation with a misspelled name would quietly
     * fill an entire region with that placeholder. Comparing type_name once blocks
     * that, and a caller receiving false can fall back to the command path for a real
     * error message.
     */
    Block const* defaultBlockNamed(std::string_view name)
    {
        std::string full = qualify(name);
        auto const& block = BlockTypeRegistry::get().getDefaultBlockState(HashedString{full}, false);
        if (block.getTypeName() != full) return nullptr;
        return &block;
    }

    /**
     * Which change source a block write uses.
     *
     * `commandsChange()` and not `structureChange()`. This path replaces `/setblock`,
     * and keeping the same source means a hook another plugin installed on block
     * changes sees exactly what it saw before. Switching to structure would make some
     * protection plugins stop intercepting, which is the kind of breakage that is
     * invisible until someone else's plugin fails.
     */
    BlockChangeContext blockEditContext() { return BlockChangeContext::commandsChange(); }
} // namespace pier::bridge

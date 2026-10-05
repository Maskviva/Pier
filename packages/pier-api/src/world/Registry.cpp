/** world/Registry.cpp: the engine's block, item and entity registries, listed entry by entry.
 *
 * A mod that wants "a random block" or "a random rare item" decides by rules over what the
 * running game contains, and this is where it reads that content. Each entry goes out as one
 * JSON object through the sink. A TypedStorage member holding a scalar, an enum or a pointer
 * is that value itself and is read without `.get()`; the rule is in tools/typed-storage.py. */
#include <cstdint>
#include <string>
#include <unordered_set>

#include "ll/api/service/Bedrock.h"

#include "mc/common/Globals.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/actor/ActorInfo.h"
#include "mc/world/actor/ActorInfoRegistry.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ItemCommandVisibility.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/registry/ItemRegistryManager.h"
#include "mc/world/item/registry/ItemRegistryRef.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockType.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/registry/BlockTypeRegistry.h"

#include "sdk/abi.h"

#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        std::string flag(char const* key, bool on) { return std::string{",\""} + key + "\":" + (on ? "true" : "false"); }

        std::string num(char const* key, double v)
        {
            // A non-finite number is not JSON. The engine has none of these here, and a
            // corrupt one becomes 0 rather than ruining the entry.
            if (!(v == v) || v > 1e300 || v < -1e300) v = 0.0;
            return std::string{",\""} + key + "\":" + std::to_string(v);
        }

        std::string whole(char const* key, std::int64_t v) { return std::string{",\""} + key + "\":" + std::to_string(v); }

        std::string text(char const* key, std::string const& v)
        {
            return std::string{",\""} + key + "\":\"" + pier::snbtEscape(v) + "\"";
        }

        void listBlocks(void* ctx, PierStrSink sink)
        {
            auto& registry = BlockTypeRegistry::mBlockTypeRegistry().mValue;
            for (auto const& [key, owned] : registry.mBlockLookupMap.get())
            {
                if (!owned) continue;
                BlockType const& type = *owned;
                Block const* block = type.mDefaultState;
                if (!block) continue;
                std::string line = "{\"name\":\"" + pier::snbtEscape(type.getTypeName()) + "\"";
                line += whole("category", static_cast<int>(type.mCreativeCategory));
                line += flag("solid", type.mSolid);
                line += flag("vanilla", type.mIsVanilla);
                line += flag("container", type.isContainerBlock());
                line += flag("signal", type.isSignalSource());
                line += flag("fence", type.isFenceBlock());
                line += flag("rail", type.isRailBlock());
                line += flag("slab", type.isSlabBlock());
                line += flag("wall", type.isWallBlock());
                line += flag("crop", block->isCropBlock());
                line += flag("entity", type.mBlockEntityType != BlockActorType::Undefined);
                // The overload that survives asks how fast a given item breaks the block;
                // an empty stack is the bare hand, as block_get_num reads it.
                line += num("destroy", static_cast<double>(block->getDestroySpeed(ItemStack{})));
                line += num("resistance", static_cast<double>(type.getExplosionResistance()));
                line += whole("light", static_cast<int>(type.getLightEmission(*block).mValue));
                line += text("description", block->getDescriptionId());
                line += "}";
                sink(ctx, ps(line));
            }
        }

        void listItems(void* ctx, PierStrSink sink)
        {
            auto registry = ItemRegistryManager::getItemRegistry();
            // The name map also holds the aliases an item answers to; each item is listed
            // once, under its own full name.
            std::unordered_set<std::string> seen;
            for (auto const& [alias, weak] : registry.getNameToItemMap())
            {
                Item const* item = weak.get();
                if (!item) continue;
                // getFullItemName is inlined away; mFullName is the string it returned.
                std::string const& name = item->mFullName->getString();
                if (name.empty() || !seen.insert(name).second) continue;
                std::string line = "{\"name\":\"" + pier::snbtEscape(name) + "\"";
                line += whole("rarity", static_cast<int>(item->getBaseRarity()));
                line += whole("stack", static_cast<int>(item->mMaxStackSize));
                line += whole("category", static_cast<int>(item->mCreativeCategory));
                line += flag("hidden", item->mIsHiddenInCommands == ItemCommandVisibility::Hidden);
                line += "}";
                sink(ctx, ps(line));
            }
        }

        /** The engine's actor type number. A vanilla identifier may resolve only without its
         *  namespace, so both spellings are tried; 0 is what an unknown name gives. */
        std::int64_t actorTypeOf(std::string const& id)
        {
            auto t = static_cast<std::int64_t>(EntityTypeFromString(id));
            if (t != 0) return t;
            auto const colon = id.find(':');
            if (colon == std::string::npos) return 0;
            return static_cast<std::int64_t>(EntityTypeFromString(id.substr(colon + 1)));
        }

        bool listEntities(void* ctx, PierStrSink sink)
        {
            auto level = ll::service::getLevel();
            if (!level) return false;
            ActorInfoRegistry* registry = level->getActorInfoRegistry();
            if (!registry) return false;
            for (auto const& info : registry->getActorInfoList())
            {
                std::string const& id = info.mIdentifier->mCanonicalName->getString();
                if (id.empty()) continue;
                std::string line = "{\"name\":\"" + pier::snbtEscape(id) + "\"";
                line += flag("spawn_egg", info.mHasSpawnEgg);
                line += flag("summonable", info.mIsSummonable);
                line += flag("experimental", info.mExperimentIndex->has_value());
                line += whole("type", actorTypeOf(id));
                line += "}";
                sink(ctx, ps(line));
            }
            return true;
        }

        bool api_registry_list(int32_t kind, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return false;
                switch (kind)
                {
                case PIER_REGISTRY_BLOCKS:
                    listBlocks(ctx, sink);
                    return true;
                case PIER_REGISTRY_ITEMS:
                    listItems(ctx, sink);
                    return true;
                case PIER_REGISTRY_ENTITIES:
                    return listEntities(ctx, sink);
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        void fill(PierApi& api) { api.registry_list = &api_registry_list; }

        spi::SlotPackReg reg{{"registry", &fill}};
    } // namespace
} // namespace pier::api_impl

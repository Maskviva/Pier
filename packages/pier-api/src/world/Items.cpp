/** world/Items.cpp: items as value objects.
 *
 * An item crosses the boundary as the SNBT form of ItemStack::save. Every call
 * rebuilds a temporary ItemStack through ItemStack::fromTag, queries or modifies it,
 * and a transforming operation serializes it back the same way. No ownership crosses
 * the boundary. */
#include <string>
#include <vector>

#include "mc/deps/core/math/Color.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/safety/RedactableString.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ItemDescriptor.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/BlockType.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        bool api_item_get_num(PierStr itemSnbt, int32_t prop, double* out)
        {
            PIER_API_GUARD_BEGIN
                auto item = bridge::itemFromSnbt(sv(itemSnbt));
                if (!item || !out) return false;
                switch (prop)
                {
                case PIER_IPROP_COUNT:
                    *out = static_cast<double>(item->mCount);
                    return true;
                case PIER_IPROP_MAX_STACK_SIZE:
                    *out = static_cast<double>(item->getMaxStackSize());
                    return true;
                case PIER_IPROP_AUX_VALUE:
                    *out = static_cast<double>(item->getAuxValue());
                    return true;
                case PIER_IPROP_ID:
                    *out = static_cast<double>(item->getId());
                    return true;
                case PIER_IPROP_DAMAGE:
                    *out = static_cast<double>(item->getDamageValue());
                    return true;
                case PIER_IPROP_IS_NULL:
                    *out = item->isNull() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_BLOCK:
                    // isBlock asked whether the stack resolved to a block, which is the
                    // pointer it kept.
                    *out = item->mBlock != nullptr ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_ENCHANTED:
                    *out = item->isEnchanted() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_ARMOR:
                    *out = item->isArmorItem() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_DAMAGEABLE:
                    *out = item->isDamageableItem() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_DAMAGED:
                    *out = item->isDamaged() ? 1.0 : 0.0;
                    return true;
                /*  Appended: item gap fills  */
                case PIER_IPROP_MAX_DAMAGE:
                    // The accessors below moved off ItemStackBase and are on Item, which
                    // mItem points at. A stack whose item is gone answers nothing rather
                    // than a zero that reads like a real durability.
                    if (!item->mItem) return false;
                    *out = static_cast<double>(item->mItem->getMaxDamage());
                    return true;
                case PIER_IPROP_IS_UNBREAKABLE:
                    // Inlined away. It read a tag out of the user data rather than a
                    // field, and reproducing that read means guessing the key.
                    return false;
                case PIER_IPROP_HAS_DURABILITY:
                    // hasDurability is inlined away with no field left to read. It asked
                    // whether the item wears out at all, which mMaxDamage alone does not
                    // answer: a stack of an item with a max damage is still not damageable
                    // when it is not an equipment item.
                    return false;
                case PIER_IPROP_IS_POTION:
                    *out = item->isPotionItem() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_THROWABLE:
                    // isThrowable() sits behind #ifdef LL_PLAT_C and is unavailable on
                    // the server.
                    return false;
                case PIER_IPROP_IS_FIRE_RESISTANT:
                    // Inlined away; the FireResistantItemComponent it consulted is not
                    // reachable from here.
                    return false;
                case PIER_IPROP_ATTACK_DAMAGE:
                    if (!item->mItem) return false;
                    *out = static_cast<double>(item->mItem->getAttackDamage());
                    return true;
                case PIER_IPROP_REPAIR_COST:
                    *out = static_cast<double>(item->getBaseRepairCost());
                    return true;
                case PIER_IPROP_ENCHANT_VALUE:
                    if (!item->mItem) return false;
                    *out = static_cast<double>(item->mItem->getEnchantValue());
                    return true;
                case PIER_IPROP_IS_STACKABLE:
                    *out = item->isStackable() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_MUSIC_DISC:
                    // Inlined away with nothing left to read.
                    return false;
                case PIER_IPROP_IS_OFFHAND:
                    // Inlined away with nothing left to read.
                    return false;
                case PIER_IPROP_USE_DURATION:
                    if (!item->mItem) return false;
                    *out = static_cast<double>(item->mItem->getMaxUseDuration(nullptr));
                    return true;
                case PIER_IPROP_IS_GLINT:
                    if (!item->mItem) return false;
                    *out = item->mItem->isGlint(*item) ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_BUNDLE:
                    // isBundle() sits behind #ifdef LL_PLAT_C and is unavailable on the
                    // server.
                    return false;
                case PIER_IPROP_HAS_USER_DATA:
                    // hasUserData read exactly this pointer.
                    *out = item->mUserData ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_HAS_CUSTOM_NAME:
                    if (!item->mItem) return false;
                    *out = item->mItem->hasCustomHoverName(*item) ? 1.0 : 0.0;
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_item_get_str(PierStr itemSnbt, int32_t prop, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto item = bridge::itemFromSnbt(sv(itemSnbt));
                if (!item || !sink) return false;
                switch (prop)
                {
                case PIER_ISTR_TYPE_NAME:
                    sink(ctx, ps(item->getTypeName()));
                    return true;
                case PIER_ISTR_NAME:
                    sink(ctx, ps(item->getName()));
                    return true;
                case PIER_ISTR_CUSTOM_NAME:
                    sink(ctx, ps(item->getCustomName()));
                    return true;
                case PIER_ISTR_RAW_NAME_ID:
                    sink(ctx, ps(item->getRawNameId()));
                    return true;
                /*  Appended  */
                case PIER_ISTR_LORE:
                {
                    auto const& lore = item->getCustomLore();
                    std::string out = "[";
                    for (size_t i = 0; i < lore.size(); ++i)
                    {
                        if (i > 0) out += ",";
                        out += "\"" + snbtEscape(lore[i]) + "\"";
                    }
                    out += "]";
                    sink(ctx, ps(out));
                    return true;
                }
                case PIER_ISTR_CAN_DESTROY:
                {
                    // getCanDestroy is inlined away; mCanDestroy is the vector it returned.
                    auto const& list = item->mCanDestroy;
                    std::string out = "[";
                    for (size_t i = 0; i < list.size(); ++i)
                    {
                        if (!list[i]) continue;
                        if (out.size() > 1) out += ",";
                        // BlockType::getRawNameId is inlined away; mNameInfo.mRawName is
                        // the HashedString it returned.
                        out += "\"" + snbtEscape(list[i]->mNameInfo->mRawName->getString()) + "\"";
                    }
                    out += "]";
                    sink(ctx, ps(out));
                    return true;
                }
                case PIER_ISTR_CAN_PLACE_ON:
                {
                    // getCanPlaceOn is inlined away; mCanPlaceOn is the vector it returned.
                    auto const& list = item->mCanPlaceOn;
                    std::string out = "[";
                    for (size_t i = 0; i < list.size(); ++i)
                    {
                        if (!list[i]) continue;
                        if (out.size() > 1) out += ",";
                        // BlockType::getRawNameId is inlined away; mNameInfo.mRawName is
                        // the HashedString it returned.
                        out += "\"" + snbtEscape(list[i]->mNameInfo->mRawName->getString()) + "\"";
                    }
                    out += "]";
                    sink(ctx, ps(out));
                    return true;
                }
                case PIER_ISTR_USER_DATA:
                {
                    // getUserData is inlined away; mUserData is the pointer it handed back.
                    auto* ud = item->mUserData.get();
                    if (!ud)
                    {
                        sink(ctx, ps(std::string_view{"{}"}));
                        return true;
                    }
                    sink(ctx, ps(ud->toSnbt(SnbtFormat::Minimize)));
                    return true;
                }
                case PIER_ISTR_HOVER_NAME:
                    // getHoverName() sits behind #ifdef LL_PLAT_C, so getName() is
                    // used. On the server it returns the same display string, since a
                    // custom name takes precedence inside getName().
                    sink(ctx, ps(item->getName()));
                    return true;
                case PIER_ISTR_EFFECT_NAME:
                    // Inlined away. It built the potion effect line out of the item's
                    // components, which are not reachable from a stack here.
                    return false;
                case PIER_ISTR_COLOR:
                {
                    // ItemStackBase::getColor is inlined away. The Item override survives
                    // and takes the two things the stack version passed it.
                    if (!item->mItem) return false;
                    auto const desc = item->getDescriptor();
                    auto color = item->mItem->getColor(item->mUserData.get(), desc);
                    std::string snbt = "{r:" + snbtDouble(color.r);
                    snbt += ",g:" + snbtDouble(color.g);
                    snbt += ",b:" + snbtDouble(color.b) + "}";
                    sink(ctx, ps(snbt));
                    return true;
                }
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_item_transform(
            PierStr itemSnbt, int32_t op, PierStr sarg, double narg, void* ctx, PierStrSink out)
        {
            PIER_API_GUARD_BEGIN
                auto item = bridge::itemFromSnbt(sv(itemSnbt));
                if (!item || !out) return false;
                switch (op)
                {
                case PIER_IOP_SET_CUSTOM_NAME:
                    item->setCustomName(
                        // The two-argument constructor is inlined away. The one that
                        // survives takes the unredacted string, which is what a null
                        // redacted string meant.
                        ::Bedrock::Safety::RedactableString{toString(sarg)});
                    break;
                case PIER_IOP_SET_DAMAGE:
                    item->setDamageValue(static_cast<short>(narg));
                    break;
                case PIER_IOP_SET_COUNT:
                {
                    int count = static_cast<int>(narg);
                    if (count < 0 || count > 255) return false;
                    item->mCount = static_cast<unsigned char>(count);
                    break;
                }
                case PIER_IOP_SET_LORE:
                {
                    // sarg is SNBT wrapped one level for easier parsing:
                    // {lore:["l1","l2"]}.
                    auto tag = CompoundTag::fromSnbt(sv(sarg));
                    if (!tag || !tag->contains("lore") || !tag->at("lore").is_array()) return false;
                    std::vector<std::string> lore;
                    for (auto const& p : tag->at("lore").get<ListTag>())
                    {
                        if (!p || p->getId() != Tag::Type::String) continue;
                        lore.emplace_back(
                            static_cast<std::string const&>(static_cast<StringTag const&>(*p)));
                    }
                    item->setCustomLore(lore);
                    break;
                }
                /*  Appended  */
                case PIER_IOP_SET_UNBREAKABLE:
                    item->setUnbreakable(narg != 0.0);
                    break;
                case PIER_IOP_HURT_AND_BREAK:
                    item->hurtAndBreak(static_cast<int>(narg), nullptr);
                    break;
                case PIER_IOP_SET_REPAIR_COST:
                    item->setRepairCost(static_cast<int>(narg));
                    break;
                case PIER_IOP_ADD_ENCHANT:
                {
                    // sarg is "enchant_name:level". A full implementation needs
                    // EnchantUtils::applyEnchant. The stub reports failure honestly
                    // rather than returning the item unchanged and claiming success
                    // (contract §5.1).
                    return false;
                }
                case PIER_IOP_REMOVE_ENCHANTS:
                    (void)item->removeEnchants();
                    break;
                case PIER_IOP_CLEAR_LORE:
                    item->setCustomLore({});
                    break;
                case PIER_IOP_RESET_NAME:
                    item->resetHoverName();
                    break;
                case PIER_IOP_SET_CAN_DESTROY:
                {
                    // sarg is an SNBT list wrapped as {v:["minecraft:stone",...]},
                    // the same pattern SET_LORE uses. CompoundTag::fromSnbt parses
                    // compound tags only, so a bare [..] list cannot be fed to it.
                    auto tag = CompoundTag::fromSnbt(sv(sarg));
                    if (!tag || !tag->contains("v") || !tag->at("v").is_array()) return false;
                    std::vector<std::string> list;
                    for (auto const& p : tag->at("v").get<ListTag>())
                    {
                        if (!p || p->getId() != Tag::Type::String) continue;
                        list.emplace_back(
                            static_cast<std::string const&>(static_cast<StringTag const&>(*p)));
                    }
                    // setCanDestroy is inlined away. mCanDestroy holds BlockType
                    // pointers and a hash beside it, so writing it means resolving every
                    // id and reproducing the hash the engine compares against; a wrong
                    // hash silently stops the restriction from applying.
                    (void)list;
                    return false;
                }
                case PIER_IOP_SET_CAN_PLACE_ON:
                {
                    auto tag = CompoundTag::fromSnbt(sv(sarg));
                    if (!tag || !tag->contains("v") || !tag->at("v").is_array()) return false;
                    std::vector<std::string> list;
                    for (auto const& p : tag->at("v").get<ListTag>())
                    {
                        if (!p || p->getId() != Tag::Type::String) continue;
                        list.emplace_back(
                            static_cast<std::string const&>(static_cast<StringTag const&>(*p)));
                    }
                    // Same shape as set_can_destroy above.
                    (void)list;
                    return false;
                }
                default:
                    return false;
                }
                out(ctx, ps(bridge::itemToSnbt(*item)));
                return true;
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.item_get_num = &api_item_get_num;
            api.item_get_str = &api_item_get_str;
            api.item_transform = &api_item_transform;
        }

        spi::SlotPackReg reg{{"items", &fill}};
    } // namespace
} // namespace pier::api_impl

/** world/Items.cpp —— 物品值对象。
 *
 * 物品以 ItemStack::save 的 SNBT 形式过边界。每次调用重建一个临时
 * ItemStack（ItemStack::fromTag）、查询或改动它，变换类操作再原样序列化
 * 回去。零跨边界所有权。 */
#include <string>
#include <vector>

#include "mc/deps/core/math/Color.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/safety/RedactableString.h"
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
                    *out = item->isBlock() ? 1.0 : 0.0;
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
                /* ── 追加：物品补漏 ── */
                case PIER_IPROP_MAX_DAMAGE:
                    *out = static_cast<double>(item->getMaxDamage());
                    return true;
                case PIER_IPROP_IS_UNBREAKABLE:
                    *out = item->isUnbreakable() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_HAS_DURABILITY:
                    *out = item->hasDurability() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_POTION:
                    *out = item->isPotionItem() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_THROWABLE:
                    // isThrowable() 在 #ifdef LL_PLAT_C 后面 —— 服务端拿不到。
                    return false;
                case PIER_IPROP_IS_FIRE_RESISTANT:
                    *out = item->isFireResistant() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_ATTACK_DAMAGE:
                    *out = static_cast<double>(item->getAttackDamage());
                    return true;
                case PIER_IPROP_REPAIR_COST:
                    *out = static_cast<double>(item->getBaseRepairCost());
                    return true;
                case PIER_IPROP_ENCHANT_VALUE:
                    *out = static_cast<double>(item->getEnchantValue());
                    return true;
                case PIER_IPROP_IS_STACKABLE:
                    *out = item->isStackable() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_MUSIC_DISC:
                    *out = item->isMusicDiscItem() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_OFFHAND:
                    *out = item->isOffhandItem() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_USE_DURATION:
                    *out = static_cast<double>(item->getMaxUseDuration());
                    return true;
                case PIER_IPROP_IS_GLINT:
                    *out = item->isGlint() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_IS_BUNDLE:
                    // isBundle() 在 #ifdef LL_PLAT_C 后面 —— 服务端拿不到。
                    return false;
                case PIER_IPROP_HAS_USER_DATA:
                    *out = item->hasUserData() ? 1.0 : 0.0;
                    return true;
                case PIER_IPROP_HAS_CUSTOM_NAME:
                    *out = item->hasCustomHoverName() ? 1.0 : 0.0;
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
                /* ── 追加 ── */
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
                    auto const& list = item->getCanDestroy();
                    std::string out = "[";
                    for (size_t i = 0; i < list.size(); ++i)
                    {
                        if (!list[i]) continue;
                        if (out.size() > 1) out += ",";
                        out += "\"" + snbtEscape(list[i]->getRawNameId()) + "\"";
                    }
                    out += "]";
                    sink(ctx, ps(out));
                    return true;
                }
                case PIER_ISTR_CAN_PLACE_ON:
                {
                    auto const& list = item->getCanPlaceOn();
                    std::string out = "[";
                    for (size_t i = 0; i < list.size(); ++i)
                    {
                        if (!list[i]) continue;
                        if (out.size() > 1) out += ",";
                        out += "\"" + snbtEscape(list[i]->getRawNameId()) + "\"";
                    }
                    out += "]";
                    sink(ctx, ps(out));
                    return true;
                }
                case PIER_ISTR_USER_DATA:
                {
                    auto* ud = item->getUserData();
                    if (!ud)
                    {
                        sink(ctx, ps(std::string_view{"{}"}));
                        return true;
                    }
                    sink(ctx, ps(ud->toSnbt(SnbtFormat::Minimize)));
                    return true;
                }
                case PIER_ISTR_HOVER_NAME:
                    // getHoverName() 在 #ifdef LL_PLAT_C 后面 —— 用 getName()，
                    // 它在服务端返回同一个展示串（自定义名在 getName() 内部优
                    // 先）。
                    sink(ctx, ps(item->getName()));
                    return true;
                case PIER_ISTR_EFFECT_NAME:
                    sink(ctx, ps(item->getEffectName(false)));
                    return true;
                case PIER_ISTR_COLOR:
                {
                    auto color = item->getColor();
                    std::string snbt = "{r:" + snbtNum(color.r);
                    snbt += ",g:" + snbtNum(color.g);
                    snbt += ",b:" + snbtNum(color.b) + "}";
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
                        ::Bedrock::Safety::RedactableString{toString(sarg), std::nullopt});
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
                    // sarg 是包了一层方便解析的 SNBT：{lore:["l1","l2"]}。
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
                /* ── 追加 ── */
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
                    // sarg = "enchant_name:level" —— 完整实现需要
                    // EnchantUtils::applyEnchant；目前是桩，原样返回物品。
                    break;
                }
                case PIER_IOP_REMOVE_ENCHANTS:
                    item->removeEnchants();
                    break;
                case PIER_IOP_CLEAR_LORE:
                    item->setCustomLore({});
                    break;
                case PIER_IOP_RESET_NAME:
                    item->resetHoverName();
                    break;
                case PIER_IOP_SET_CAN_DESTROY:
                {
                    // sarg = 包成 {v:["minecraft:stone", …]} 的 SNBT 列表（与
                    // SET_LORE 同一个模式）—— CompoundTag::fromSnbt 只解析复合标
                    // 签，裸 [..] 列表没法直接喂。
                    auto tag = CompoundTag::fromSnbt(sv(sarg));
                    if (!tag || !tag->contains("v") || !tag->at("v").is_array()) return false;
                    std::vector<std::string> list;
                    for (auto const& p : tag->at("v").get<ListTag>())
                    {
                        if (!p || p->getId() != Tag::Type::String) continue;
                        list.emplace_back(
                            static_cast<std::string const&>(static_cast<StringTag const&>(*p)));
                    }
                    item->setCanDestroy(list);
                    break;
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
                    item->setCanPlaceOn(list);
                    break;
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

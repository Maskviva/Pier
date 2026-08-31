/** core/Bridge.cpp —— 把 ABI 侧的引用解析成引擎对象。声明见 bridge.h。 */
#include <mutex>
#include <string>
#include <unordered_set>

#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/service/TargetedBedrock.h"
#include "ll/api/utils/ErrorUtils.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/platform/UUID.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/ServerCommandOrigin.h"
#include "mc/world/Container.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Inventory.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/actor/provider/ActorEquipment.h"
#include "mc/world/inventory/EnderChestContainer.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/SaveContext.h"
#include "mc/world/item/SaveContextFactory.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/dimension/Dimension.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::bridge
{
    namespace
    {
        /**
         * 同一个 id 只抱怨一次。这些错误一旦发生就会每 tick 复现，
         * 不去重的话日志会被冲垮，反而看不见第一条。
         */
        bool firstComplaintFor(int32_t dimId)
        {
            static std::mutex mtx;
            static std::unordered_set<int32_t> seen;
            std::lock_guard lock{mtx};
            return seen.insert(dimId).second;
        }
    } // namespace

    Level* levelReady()
    {
#ifdef PIER_BUILD_CLIENT
        auto level = ll::service::getMultiPlayerLevel();
#else
        auto level = ll::service::getLevel();
#endif
        return level ? &*level : nullptr;
    }

    BlockSource* blockSourceOf(int32_t dimId)
    {
        auto* level = levelReady();
        if (!level) return nullptr;

        // 已经建好的维度：直接拿。
        if (auto dim = level->getDimension(DimensionType{dimId}).lock())
        {
            return &dim->getBlockSourceFromMainChunkSource();
        }

        // 没人进过的自定义维度只存在于注册表里，Dimension 对象还没被创建。
        // 要往里写方块（或者传送人进去）就得先把它逼出来 —— 怎么逼、按什么
        // 名字逼，只有 dimensions 能力包知道，所以走维度桥（spi §6）。
        if (dimId >= 3)
        {
            if (auto const* db = spi::dimensionBridge())
            {
                return db->blockSourceOf(dimId); // 失败的诊断由桥的实现方打
            }
            if (firstComplaintFor(dimId))
            {
                hostLogger().warn(
                    "维度 {}：这是自定义维度 id，但 dimensions 能力包没有编进本宿主 —— "
                    "只认原版三个维度",
                    dimId
                );
            }
            return nullptr;
        }

        // 原版维度。
        if (dimensionSelector(dimId).empty()) return nullptr;
        auto dim = level->getOrCreateDimension(DimensionType{dimId}).lock();
        if (!dim) return nullptr;
        return &dim->getBlockSourceFromMainChunkSource();
    }

    Player* resolvePlayer(PierPlayerSel sel)
    {
        auto* level = levelReady();
        if (!level || sel.value.len == 0) return nullptr;
        std::string_view wanted = sv(sel.value);

        Player* found = nullptr;
        level->forEachPlayer([&](Player& p)
        {
            bool hit = false;
            switch (sel.kind)
            {
            case 0: // 账号名
                hit = (p.getRealName() == wanted);
                break;
            case 1: // xuid
                hit = (p.getXuid() == wanted);
                break;
            case 2: // uuid
                hit = (p.getUuid().asString() == wanted);
                break;
            default:
                break;
            }
            if (hit)
            {
                found = &p;
                return false;
            }
            return true;
        });
        if (!found && sel.kind == 0)
        {
            // 第二遍按显示名找：改名牌插件改的是 NameTag，不是账号名。
            level->forEachPlayer([&](Player& p)
            {
                if (std::string_view{p.getNameTag()} == wanted)
                {
                    found = &p;
                    return false;
                }
                return true;
            });
        }
        return found;
    }

    Actor* resolveActor(PierActorId id)
    {
        auto* level = levelReady();
        if (!level || id == 0) return nullptr;
        ActorUniqueID uid{};
        uid.rawID = id;
        return level->fetchEntity(uid, /*getRemoved*/ false);
    }

    Container* resolveContainer(PierContainerRef ref)
    {
        if (ref.which == 4)
        {
            // 方块容器（箱子 / 漏斗 /…）在 (dim, pos)。
            auto* bs = blockSourceOf(ref.dim);
            if (!bs) return nullptr;
            auto* be = bs->getBlockEntity(BlockPos{ref.x, ref.y, ref.z});
            if (!be) return nullptr;
            return be->getContainer();
        }
        Player* p = resolvePlayer(ref.player);
        if (!p) return nullptr;
        switch (ref.which)
        {
        case 0: // 主背包
            return &p->getInventory();
        case 1:
        {
            // 末影箱
            auto ec = p->getEnderChestContainer();
            return ec ? ec.as_ptr() : nullptr;
        }
        // 盔甲和手是货真价实的 Container，只是不从 Player 走：
        // ActorEquipment::getArmorContainer(EntityContext&) 与 getHandContainer(...)
        // 递回 SimpleContainer，它派生自 Container，既有的物品读写路径原样可用。
        // 不要改用 actor 快照 NBT，那是存档表示，落后现实的时长等于上次落盘到现在。
        case 2: // 盔甲：头、胸、腿、脚
            return &ActorEquipment::getArmorContainer(p->getEntityContext());
        case 3: // 手：槽 0 = 主手，槽 1 = 副手
            return &ActorEquipment::getHandContainer(p->getEntityContext());
        default:
            return nullptr;
        }
    }

    std::string dimensionSelector(int32_t dim)
    {
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
        if (dim < 0) return {};

        // 自定义维度的名字只有 dimensions 能力包知道（它的注册台账、引擎
        // DimensionMap、配置镜像的三层数据源和漂移告警都在桥的那一头 ——
        // 连「为什么不能碰 VanillaDimensions::toString」的血泪史也在那边）。
        if (auto const* db = spi::dimensionBridge())
        {
            return db->selectorNameOf(dim);
        }
        if (firstComplaintFor(dim))
        {
            hostLogger().warn(
                "维度 {}：自定义维度 id，但 dimensions 能力包没有编进本宿主 —— 无法解析名字",
                dim
            );
        }
        return {};
    }

    char const* dimensionName(int dim)
    {
        switch (dim)
        {
        case 1:
            return "nether";
        case 2:
            return "the_end";
        default:
            return "overworld";
        }
    }

    bool runConsoleCommand(std::string const& cmd)
    {
#ifdef PIER_BUILD_CLIENT
        // 客户端构建没有服务器控制台 —— 命令是服务端专属。
        (void)cmd;
        return false;
#else
        auto* level = levelReady();
        if (!level) return false;
        ServerCommandOrigin origin{
            "Server",
            static_cast<ServerLevel&>(*level),
            CommandPermissionLevel::Owner,
            0
        };
        auto output = ll::command::CommandRegistrar::getServerInstance().executeCommand(cmd, origin);
        return output.mSuccessCount > 0;
#endif
    }

    std::string playerSummarySnbt(Player& p)
    {
        auto pos = p.getPosition();
        std::string out = "{name:\"" + snbtEscape(p.getRealName())
            + "\",xuid:\"" + snbtEscape(p.getXuid())
            + "\",uuid:\"" + snbtEscape(p.getUuid().asString())
            + "\",dim:" + snbtNum(static_cast<long long>(static_cast<int>(p.getDimensionId())))
            + ",x:" + snbtDouble(pos.x)
            + ",y:" + snbtDouble(pos.y)
            + ",z:" + snbtDouble(pos.z) + "}";
        return out;
    }

    std::string itemToSnbt(ItemStack const& item)
    {
        auto ctx = SaveContextFactory::createCloneSaveContext();
        auto tag = item.save(*ctx);
        if (!tag) return "{}";
        return tag->toSnbt(SnbtFormat::Minimize);
    }

    std::optional<ItemStack> itemFromSnbt(std::string_view snbt)
    {
        // fromSnbt / fromTag 对畸形输入会抛；这里的输入最终来自客户端。
        try
        {
            auto tag = CompoundTag::fromSnbt(snbt);
            if (!tag) return std::nullopt;
            return ItemStack::fromTag(*tag);
        }
        catch (...)
        {
            ll::error_utils::printCurrentException(hostLogger());
            return std::nullopt;
        }
    }

    double nbtToDouble(CompoundTagVariant const& val, double def)
    {
        if (val.is_number_float()) return static_cast<double>(val);
        if (val.is_number_integer()) return static_cast<double>(static_cast<int64_t>(val));
        return def;
    }
} // namespace pier::bridge

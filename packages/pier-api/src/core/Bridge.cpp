/** core/Bridge.cpp: resolves ABI-side references into engine objects. The
 *  declarations are in bridge.h. */
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
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
#include "mc/legacy/ActorUniqueID.h"
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
         * Complains once per id. These errors repeat every tick once they start, and
         * without deduplication the log is flooded and the first line is lost.
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

        // An already built dimension is taken directly.
        if (auto dim = level->getDimension(DimensionType{dimId}).lock())
        {
            return &dim->getBlockSourceFromMainChunkSource();
        }

        // A custom dimension nobody has entered exists only in the registry and its
        // Dimension object has not been created yet. Writing a block into it, or
        // teleporting someone there, has to force it into existence first. Only the
        // dimensions package knows how and under which name, so this goes through the
        // dimension bridge (spi §6).
        if (dimId >= 3)
        {
            if (auto const* db = spi::dimensionBridge())
            {
                return db->blockSourceOf(dimId); // The bridge logs its own diagnostics
            }
            if (firstComplaintFor(dimId))
            {
                hostLogger().warn(
                    "[api] dimension {} is a custom dimension id, but the dimensions "
                    "package is not compiled into this host; only the three vanilla "
                    "dimensions are recognized",
                    dimId
                );
            }
            return nullptr;
        }

        // A vanilla dimension.
        if (dimensionSelector(dimId).empty()) return nullptr;
        auto dim = level->getOrCreateDimension(DimensionType{dimId}).lock();
        if (!dim) return nullptr;
        return &dim->getBlockSourceFromMainChunkSource();
    }

    namespace
    {
        struct StringHash
        {
            using is_transparent = void;
            size_t operator()(std::string_view s) const noexcept { return std::hash<std::string_view>{}(s); }
        };
        using SelectorCache = std::unordered_map<std::string, int64_t, StringHash, std::equal_to<>>;

        /**
         * Selector to ActorUniqueID, one table per selector kind, server thread only.
         *
         * A forEachPlayer scan is what a selector costs without this table, and
         * Player::getRealName() and getXuid() return std::string by value, so one scan over
         * N players is N allocations, on every property read of every player handle. A
         * repeat lookup is one fetchEntity plus one identity check instead. A hit is always
         * verified against the live player, so a stale entry after a leave and rejoin, or an
         * id reused by a bot, is dropped and the scan runs.
         */
        SelectorCache& cacheFor(int32_t kind)
        {
            static SelectorCache byName, byXuid, byUuid;
            switch (kind)
            {
            case 1:
                return byXuid;
            case 2:
                return byUuid;
            default:
                return byName;
            }
        }

        bool selectorMatches(Player const& p, int32_t kind, std::string_view wanted)
        {
            switch (kind)
            {
            case 0:
                return p.getRealName() == wanted;
            case 1:
                return p.getXuid() == wanted;
            case 2:
                return p.getUuid().asString() == wanted;
            default:
                return false;
            }
        }
    } // namespace

    Player* resolvePlayer(PierPlayerSel sel)
    {
        auto* level = levelReady();
        if (!level || sel.value.len == 0) return nullptr;
        std::string_view wanted = sv(sel.value);
        if (sel.kind < 0 || sel.kind > 2) return nullptr;

        auto& cache = cacheFor(sel.kind);
        if (auto hit = cache.find(wanted); hit != cache.end())
        {
            ActorUniqueID uid{};
            uid.rawID = hit->second;
            Actor* a = level->fetchEntity(uid, /*getRemoved*/ false);
            if (a && a->isPlayer())
            {
                auto* p = static_cast<Player*>(a);
                if (selectorMatches(*p, sel.kind, wanted)) return p;
            }
            cache.erase(hit); // Gone, or the id now belongs to someone else
        }

        Player* found = nullptr;
        level->forEachPlayer([&](Player& p)
        {
            bool hit = false;
            switch (sel.kind)
            {
            case 0: // Account name
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
        if (found)
        {
            // Only an exact match of the selector kind is cached. The NameTag fallback
            // below is deliberately not, because a NameTag is a display string another
            // plugin may change at any time.
            if (cache.size() > 4096) cache.clear(); // Bounded; bots come and go
            cache.emplace(std::string{wanted}, found->getOrCreateUniqueID().rawID);
        }
        if (!found && sel.kind == 0)
        {
            // A second pass matches the display name, because a name-tag plugin
            // changes the NameTag and not the account name.
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
            // A block container such as a chest or hopper lives at (dim, pos).
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
        case 0: // Main inventory
            return &p->getInventory();
        case 1:
        {
            // Ender chest
            auto ec = p->getEnderChestContainer();
            return ec ? ec.as_ptr() : nullptr;
        }
        // Armor and hands are real Containers, they simply do not come from Player.
        // ActorEquipment::getArmorContainer(EntityContext&) and getHandContainer(...)
        // hand back a SimpleContainer, which derives from Container, so the existing
        // item read and write paths work unchanged. Actor snapshot NBT is not a
        // substitute: it is the save representation and lags reality by however long
        // ago the last write to disk was.
        case 2: // Armor: head, chest, legs, feet
            return &ActorEquipment::getArmorContainer(p->getEntityContext());
        case 3: // Hands: slot 0 is the main hand, slot 1 the offhand
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

        // Only the dimensions package knows the name of a custom dimension. Its
        // registration ledger, the engine DimensionMap, the config mirror and the
        // drift warnings across those three sources all live on the far side of the
        // bridge, together with the reason VanillaDimensions::toString must not be
        // used.
        if (auto const* db = spi::dimensionBridge())
        {
            return db->selectorNameOf(dim);
        }
        if (firstComplaintFor(dim))
        {
            hostLogger().warn(
                "[api] dimension {} is a custom dimension id, but the dimensions "
                "package is not compiled into this host; the name cannot be resolved",
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
        // A client build has no server console, so commands are server only.
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
        // fromSnbt and fromTag throw on malformed input, and this input ultimately
        // comes from a client.
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

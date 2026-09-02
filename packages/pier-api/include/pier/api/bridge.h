#pragma once
// Resolution helpers shared inside the pier-api package. A private include, since
// capability packages do not include each other (contract §1), so everything here
// serves only the TUs of this package's own domains.
//
// The bar for admission: at least two domains need it, and it belongs to the single
// job of resolving an ABI-side reference into an engine object. A helper used by one
// domain stays in that domain's own TU.

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
    /** The Level pointer once it is ready, otherwise nullptr. The server build uses
     *  getLevel() and the client build uses getMultiPlayerLevel(), which is the only
     *  target branch here. */
    [[nodiscard]] Level* levelReady();

    /** Dimension id to BlockSource. An already built dimension is taken directly. A
     *  custom dimension with id 3 or above is forced into existence through
     *  spi::dimensionBridge, and when that package is absent this degrades to vanilla
     *  dimensions only and warns once. Returns nullptr on failure, which the caller
     *  must treat as a failure (§5.1). */
    [[nodiscard]] BlockSource* blockSourceOf(int32_t dimId);

    /** Player selector, where kind 0 is the account name, 1 is the xuid and 2 is the
     *  uuid. When the account name finds nothing, a second pass matches the display
     *  name, because a name-tag plugin changes the NameTag. nullptr when not found. */
    [[nodiscard]] Player* resolvePlayer(PierPlayerSel sel);

    /** ActorUniqueID to Actor, excluding removed ones. nullptr when not found. */
    [[nodiscard]] Actor* resolveActor(PierActorId id);

    /** Container reference to Container. which is 0 for the inventory, 1 for the
     *  ender chest, 2 for armor, 3 for hands and 4 for a block container at
     *  (dim,x,y,z). Armor and hands are real Containers, obtained as SimpleContainer
     *  through ActorEquipment, and not snapshot NBT. */
    [[nodiscard]] Container* resolveContainer(PierContainerRef ref);

    /** Any registered dimension id to the name `/execute in` accepts. An unknown id
     *  returns an empty string and the caller must fail. Falling back to the overworld
     *  is never correct, because it lands every write meant for another dimension in
     *  the survival overworld. */
    [[nodiscard]] std::string dimensionSelector(int32_t dim);

    /** Name of one of the three vanilla dimensions. Anything outside 0, 1 and 2 is
     *  reported as overworld. For display where the dimension is certainly vanilla.
     *  Resolution uses dimensionSelector. */
    [[nodiscard]] char const* dimensionName(int dim);

    /** Runs one console command as the server, at Owner permission. Always false in
     *  the client build. */
    bool runConsoleCommand(std::string const& cmd);

    /** Player identity and position on one line: {name,xuid,uuid,dim,x,y,z}. */
    [[nodiscard]] std::string playerSummarySnbt(Player& p);

    /** ItemStack to and from SNBT. fromSnbt does not throw on malformed input, since
     *  that input ultimately comes from a client. It returns nullopt on failure and
     *  leaves a log line. */
    [[nodiscard]] std::string itemToSnbt(ItemStack const& item);
    [[nodiscard]] std::optional<ItemStack> itemFromSnbt(std::string_view snbt);

    /** NBT number to double. A non-numeric value yields def. */
    [[nodiscard]] double nbtToDouble(CompoundTagVariant const& val, double def);

    /** Block specification parsing, shared by the World and Edit TUs. Both must parse
     *  a block by the same rules, otherwise a fill and a setblock would resolve the
     *  same name differently. Defined in world/BlockResolve.cpp, which is compiled
     *  into both targets. Defining them in the server-only Edit.cpp while the
     *  both-target World.cpp references them breaks the client link. */

    /** Serialized NBT of the form {name,states,version} to Block, running the engine
     *  version upgrade table. */
    [[nodiscard]] Block const* blockFromTag(CompoundTag const& tag);
    /** SNBT text to Block, internally through blockFromTag. nullptr on failure. */
    [[nodiscard]] Block const* blockFromSnbt(std::string_view snbt);
    /** Block name to default state. nullptr when not found, never a placeholder
     *  block. */
    [[nodiscard]] Block const* defaultBlockNamed(std::string_view name);
    /** The change source a block write uses, which is commandsChange. The reason is
     *  in BlockResolve.cpp. */
    [[nodiscard]] BlockChangeContext blockEditContext();

    /** Event payload enrichment. Resolves the reflection pointer stubs
     *  `{_type_,_pointer_}` emitted by LL serializeRefObj into fields a consumer can
     *  read, such as _player, dim and _identifier. The definition and the reasoning
     *  are in the Enrich.cpp file header. */
    [[nodiscard]] std::string enrichEventData(CompoundTag const& data);
} // namespace pier::bridge

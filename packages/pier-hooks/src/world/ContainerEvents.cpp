/** hooks/world/ContainerEvents.cpp: the synthetic, cancellable
 * "PlayerOpenContainerEvent".
 * The permission model has an open_container action, and the host had no container hook
 * to feed it: a visitor who breaks no block can walk into someone's claim and empty their
 * chests.
 * The hook point is VanillaServerGameplayEventListener::onEvent, which returns an
 * EventResult where StopProcessing aborts the open. A synthetic event in this package
 * observes only by default (see hook_events.h), so this uses
 * dispatchHookEventCancellable rather than dispatchHookEvent, whose write-back sink is a
 * no-op by design. Any subscriber answering with the cancel flag refuses the open. */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/server/module/VanillaServerGameplayEventListener.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/events/EventResult.h"
#include "mc/deps/shared_types/legacy/ContainerType.h"
#include "mc/world/events/PlayerOpenContainerEvent.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& openContainerDef(); // Forward declaration

        /** The block at `pos` by type name, or an empty string when it cannot be read.
         *
         *  A subscriber uses this to tell a chest from a dropper. Empty rather than a
         *  placeholder on failure: a name invented here would be indistinguishable from a
         *  real one, and a rule keyed on it would fire on the wrong block. */
        std::string blockNameAt(Player& p, BlockPos const& pos)
        {
            auto& region = p.getDimensionBlockSource();
            return std::string{region.getBlock(pos).getTypeName()};
        }

        /** `ContainerType` as a lowercase word, for a permission node that reads
         *  `minecraft.container.dropper` rather than `minecraft.container.7`.
         *
         *  A table and not a reflection call: the numbers are a network enum and are
         *  stable, while a name derived from the engine would change a server's
         *  permission nodes under it whenever Mojang renamed one. An unlisted value is
         *  empty, and the subscriber falls back to its unqualified node. */
        std::string containerName(::SharedTypes::Legacy::ContainerType t)
        {
            using Ct = ::SharedTypes::Legacy::ContainerType;
            switch (t)
            {
            case Ct::Container: return "chest";
            case Ct::Workbench: return "workbench";
            case Ct::Furnace: return "furnace";
            case Ct::Enchantment: return "enchanting_table";
            case Ct::BrewingStand: return "brewing_stand";
            case Ct::Anvil: return "anvil";
            case Ct::Dispenser: return "dispenser";
            case Ct::Dropper: return "dropper";
            case Ct::Hopper: return "hopper";
            case Ct::MinecartChest: return "minecart_chest";
            case Ct::MinecartHopper: return "minecart_hopper";
            case Ct::Horse: return "horse";
            case Ct::Beacon: return "beacon";
            case Ct::StructureEditor: return "structure_editor";
            case Ct::Trade: return "trade";
            case Ct::CommandBlock: return "command_block";
            case Ct::Jukebox: return "jukebox";
            case Ct::CompoundCreator: return "compound_creator";
            case Ct::ElementConstructor: return "element_constructor";
            case Ct::MaterialReducer: return "material_reducer";
            case Ct::LabTable: return "lab_table";
            case Ct::Loom: return "loom";
            case Ct::Lectern: return "lectern";
            case Ct::Grindstone: return "grindstone";
            case Ct::BlastFurnace: return "blast_furnace";
            case Ct::Smoker: return "smoker";
            case Ct::Stonecutter: return "stonecutter";
            case Ct::Cartography: return "cartography_table";
            case Ct::JigsawEditor: return "jigsaw_editor";
            case Ct::SmithingTable: return "smithing_table";
            case Ct::ChestBoat: return "chest_boat";
            case Ct::DecoratedPot: return "decorated_pot";
            case Ct::Crafter: return "crafter";
            default: return "";
            }
        }

        LL_TYPE_INSTANCE_HOOK(
            PlayerOpenContainerHook,
            ll::memory::HookPriority::Normal,
            VanillaServerGameplayEventListener,
            &VanillaServerGameplayEventListener::$onEvent,
            ::EventResult,
            ::PlayerOpenContainerEvent const& ev)
        {
            auto& def = openContainerDef();
            if (!def.live())
            {
                return origin(ev);
            }

            // mPlayer is a WeakEntityRef and may already be dead when read, so it goes
            // through tryUnwrap and the action passes on failure.
            Actor* actor = nullptr;
            auto opt = ev.mPlayer->tryUnwrap<Actor>();
            actor = opt ? &*opt : nullptr;
            if (!actor || !actor->isType(::ActorType::Player))
            {
                return origin(ev);
            }
            auto& p = *static_cast<Player*>(actor);

            auto const& pos = ev.mBlockPos.get();
            std::string snbt = "{\"eventId\":\"PlayerOpenContainerEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(actor->getDimensionId()))
                + ",\"containerType\":" + snbtNum(static_cast<int>(ev.mContainerType))
                + ",\"container\":" + snbtStr(containerName(ev.mContainerType))
                + ",\"block\":" + snbtStr(blockNameAt(p, pos))
                + "," + playerRefSnbt(p) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return ::EventResult::StopProcessing;
            }
            return origin(ev);
        }

        HookEventDef gDef{"PlayerOpenContainerEvent", [] { return PlayerOpenContainerHook::hook() == 0; }};
        HookEventDef& openContainerDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

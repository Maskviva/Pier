/** protect/NpcCommandEvent.cpp: the synthetic, cancellable "NpcCommandEvent".
 * A button on an NPC dialogue runs commands for the player who pressed it, and which commands
 * is the NPC author's choice and not the player's. On a server with plots that makes an NPC a
 * way to run commands in someone else's ground. NpcComponent::executeCommandAction is the
 * engine's single place where a command action is carried out, so the hook asks right before.
 * Returning without calling origin means the button does nothing.
 * Payload {eventId, x, y, z, dim, npc, npcId, actionIndex, scene, command, _player}: the NPC,
 * the commands of the action joined by semicolons as the author wrote them, and the player who
 * pressed the button. An action that is not a command action is not asked about, and `partial`
 * is 1 when the commands could not be read. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>
#include <variant>

#include "ll/api/memory/Hook.h"

#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/entity/components_json_legacy/NpcComponent.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/npc/ActionContainer.h"
#include "mc/world/actor/npc/CommandAction.h"
#include "mc/world/actor/npc/StoredCommand.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& npcCommandDef(); // Forward declaration

        LL_TYPE_STATIC_HOOK(
            NpcCommandHook,
            ll::memory::HookPriority::Normal,
            NpcComponent,
            &NpcComponent::executeCommandAction,
            void,
            ::Actor& owner,
            ::Player& sourcePlayer,
            int actionIndex,
            ::std::string const& sceneName)
        {
            auto& def = npcCommandDef();
            if (!def.live()) return origin(owner, sourcePlayer, actionIndex, sceneName);

            std::string command;
            std::string npcName;
            int64_t npcId = 0;
            bool isCommand = false;
            bool readFailed = false;
            try
            {
                npcName = owner.getTypeName();
                npcId = owner.getOrCreateUniqueID().rawID;
                auto component = owner.getEntityContext().tryGetComponent<NpcComponent>();
                if (component)
                {
                    auto& actions = component->mActionsContainer->mActions.get();
                    if (actionIndex >= 0 && static_cast<std::size_t>(actionIndex) < actions.size())
                    {
                        if (auto const* action = std::get_if<::npc::CommandAction>(&actions[actionIndex]))
                        {
                            isCommand = true;
                            for (auto const& stored : action->commands.get())
                            {
                                if (!command.empty()) command += ";";
                                command += stored.rawCommand.get();
                            }
                        }
                    }
                }
                else
                {
                    readFailed = true;
                }
            }
            catch (...)
            {
                command.clear();
                readFailed = true;
            }
            if (!isCommand && !readFailed) return origin(owner, sourcePlayer, actionIndex, sceneName);

            auto const& at = owner.getPosition();
            std::string const snbt = "{\"eventId\":\"NpcCommandEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(at.x))
                + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(owner.getDimensionId()))
                + ",\"npc\":\"" + snbtEscape(npcName) + "\""
                + ",\"npcId\":" + snbtNum(npcId) + "L"
                + ",\"actionIndex\":" + snbtNum(actionIndex)
                + ",\"scene\":\"" + snbtEscape(sceneName) + "\""
                + ",\"command\":" + snbtStr(command)
                + (readFailed ? ",\"partial\":1" : "")
                + "," + playerRefSnbt(sourcePlayer) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin(owner, sourcePlayer, actionIndex, sceneName);
        }

        HookEventDef gDef{"NpcCommandEvent", [] { return NpcCommandHook::hook() == 0; }};
        HookEventDef& npcCommandDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT

/** hooks/world/ScoreChangedEvent.cpp: the synthetic "ScoreChangedEvent", observation only.
 * A scoreboard is where plenty of servers keep what a player has: a balance, a rank, a
 * playtime. ServerScoreboard::onScoreChanged is the engine's notification that a score now has
 * a new value, whoever changed it, a command, a command block or another mod, so a mod can
 * react without polling. It runs after the score holds the new value, so it cannot be cancelled.
 * Payload {eventId, objective, displayName, score, kind, name, entityId, _player}: `kind` is
 * "player", "entity" or "fake" for a name that belongs to no one. `entityId` is the player's or
 * the entity's id and `name` is a fake player's name, and `_player` appears for a player who is
 * online. A score of an invalid identity is not reported, and `partial` is 1 when a field could
 * not be read. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

// cl rejects EventCoordinatorPimpl::_processEvent, an inline member template marked MCAPI
// (dllimport), with C2491; clang-cl, which the host builds with, accepts it. Emptying
// MCAPI for this one header keeps the file building under cl as well, and #pragma once
// stops the headers below from reading it again. A file that takes the address of
// _processEvent cannot do this: the address would be this local copy, not the engine's.
#include "mc/_HeaderOutputPredefine.h"
#include "mc/world/events/EventResult.h"
#pragma push_macro("MCAPI")
#undef MCAPI
#define MCAPI
#include "mc/world/events/EventCoordinatorPimpl.h"
#pragma pop_macro("MCAPI")

#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"

#include "mc/legacy/ActorUniqueID.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include "mc/world/scores/IdentityDefinition.h"
#include "mc/world/scores/Objective.h"
#include "mc/world/scores/ScoreboardId.h"
#include "mc/world/scores/ServerScoreboard.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& scoreDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            ScoreChangedHook,
            ll::memory::HookPriority::Normal,
            ServerScoreboard,
            &ServerScoreboard::$onScoreChanged,
            void,
            ::ScoreboardId const& id,
            ::Objective const& obj)
        {
            auto& def = scoreDef();
            if (!def.live()) return origin(id, obj);

            std::string kind;
            std::string name;
            int64_t entityId = 0;
            std::string playerPart;
            int score = 0;
            std::string objective;
            std::string displayName;
            bool readFailed = false;
            try
            {
                auto* identity = id.mIdentityDef;
                if (identity != nullptr)
                {
                    switch (identity->mIdentityType)
                    {
                    case ::IdentityDefinition::Type::Player:
                    {
                        kind = "player";
                        entityId = identity->mPlayerId->mActorUniqueId;
                        if (auto level = ll::service::getLevel())
                        {
                            if (::Player* online = level->getPlayer(::ActorUniqueID{entityId}))
                            {
                                playerPart = "," + playerRefSnbt(*online);
                            }
                        }
                        break;
                    }
                    case ::IdentityDefinition::Type::Entity:
                        kind = "entity";
                        entityId = identity->mEntityId->rawID;
                        break;
                    case ::IdentityDefinition::Type::FakePlayer:
                        kind = "fake";
                        name = identity->mPlayerName.get();
                        break;
                    default:
                        break;
                    }
                }
                score = obj.getPlayerScore(id).mValue;
                objective = obj.mName.get();
                displayName = obj.mDisplayName.get();
            }
            catch (...)
            {
                readFailed = true;
            }
            if (kind.empty()) return origin(id, obj);

            std::string const snbt = "{\"eventId\":\"ScoreChangedEvent\""
                ",\"objective\":\"" + snbtEscape(objective) + "\""
                + ",\"displayName\":\"" + snbtEscape(displayName) + "\""
                + ",\"score\":" + snbtNum(score)
                + ",\"kind\":\"" + kind + "\""
                + ",\"name\":\"" + snbtEscape(name) + "\""
                + ",\"entityId\":" + snbtNum(entityId) + "L"
                + (readFailed ? ",\"partial\":1" : "") + playerPart + "}";

            dispatchHookEvent(def, snbt);
            origin(id, obj);
        }

        HookEventDef gDef{"ScoreChangedEvent", [] { return ScoreChangedHook::hook() == 0; }};
        HookEventDef& scoreDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT

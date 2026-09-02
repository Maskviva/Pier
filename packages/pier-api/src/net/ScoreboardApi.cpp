/** net/ScoreboardApi.cpp: scoreboard operations.
 *
 * Reads and writes both go through the native Scoreboard from Level::getScoreboard.
 * A scoring identity uses a fake player name, in the same namespace vanilla
 * /scoreboard uses, so results line up with in-game state. */
#ifndef PIER_BUILD_CLIENT

#include <limits>
#include <string>

#include "mc/world/level/Level.h"
#include "mc/world/scores/Objective.h"
#include "mc/world/scores/ObjectiveCriteria.h"
#include "mc/world/scores/PlayerScoreSetFunction.h"
#include "mc/world/scores/ScoreInfo.h"
#include "mc/world/scores/Scoreboard.h"
#include "mc/world/scores/ScoreboardId.h"
#include "mc/world/scores/ScoreboardOperationResult.h"

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
        /** Gets or creates the ScoreboardId for a fake player name. */
        ScoreboardId const& idFor(Scoreboard& board, std::string const& name)
        {
            auto const& existing = board.getScoreboardId(name);
            if (existing.mRawID != ScoreboardId::INVALID().mRawID) return existing;
            return board.createScoreboardId(name);
        }

        bool modifyScore(Scoreboard& board, std::string const& objName, std::string const& who,
                         int value, PlayerScoreSetFunction fn, int* newValue)
        {
            auto* obj = board.getObjective(objName);
            if (!obj) return false;
            auto const& id = idFor(board, who);
            ScoreboardOperationResult result{};
            int v = board.modifyPlayerScore(result, id, *obj, value, fn);
            if (newValue) *newValue = v;
            return result == ScoreboardOperationResult::Success;
        }

        bool api_scoreboard_op(int32_t op, PierStr a, PierStr b, int64_t n, void* ctx, PierStrSink out)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return false;
                auto& board = level->getScoreboard();
                std::string sa = toString(a);
                std::string sb = toString(b);
                // A score is an int, and silent truncation would turn 2^32 into 0.
                if (n < std::numeric_limits<int>::min() || n > std::numeric_limits<int>::max()) return false;

                switch (op)
                {
                case PIER_SB_ADD_OBJECTIVE:
                {
                    auto* criteria = board.getCriteria(Scoreboard::DEFAULT_CRITERIA());
                    if (!criteria) return false;
                    auto* obj = board.addObjective(sa, sb.empty() ? sa : sb, *criteria);
                    return obj != nullptr;
                }
                case PIER_SB_REMOVE_OBJECTIVE:
                {
                    auto* obj = board.getObjective(sa);
                    if (!obj) return false;
                    return board.removeObjective(obj);
                }
                case PIER_SB_LIST_OBJECTIVES:
                {
                    if (!out) return false;
                    std::string list = "[";
                    for (auto const* obj : board.getObjectives())
                    {
                        if (!obj) continue;
                        list += "{name:\"" + snbtEscape(obj->mName.get())
                            + "\",display:\"" + snbtEscape(obj->mDisplayName.get()) + "\"},";
                    }
                    if (list.back() == ',') list.pop_back();
                    list += "]";
                    out(ctx, ps(list));
                    return true;
                }
                case PIER_SB_GET_SCORE:
                {
                    auto* obj = board.getObjective(sa);
                    if (!obj || !out) return false;
                    auto const& id = board.getScoreboardId(sb);
                    if (id.mRawID == ScoreboardId::INVALID().mRawID) return false;
                    auto info = obj->getPlayerScore(id);
                    if (!info.mValid) return false;
                    out(ctx, ps(snbtNum(info.mValue)));
                    return true;
                }
                case PIER_SB_SET_SCORE:
                {
                    int nv = 0;
                    if (!modifyScore(board, sa, sb, static_cast<int>(n),
                                     PlayerScoreSetFunction::Set, &nv))
                        return false;
                    if (out) out(ctx, ps(snbtNum(nv)));
                    return true;
                }
                case PIER_SB_ADD_SCORE:
                {
                    int nv = 0;
                    if (!modifyScore(board, sa, sb, static_cast<int>(n),
                                     PlayerScoreSetFunction::Add, &nv))
                        return false;
                    if (out) out(ctx, ps(snbtNum(nv)));
                    return true;
                }
                case PIER_SB_REDUCE_SCORE:
                {
                    int nv = 0;
                    if (!modifyScore(board, sa, sb, static_cast<int>(n),
                                     PlayerScoreSetFunction::Subtract, &nv))
                        return false;
                    if (out) out(ctx, ps(snbtNum(nv)));
                    return true;
                }
                case PIER_SB_RESET_SCORE:
                {
                    auto* obj = board.getObjective(sa);
                    if (!obj) return false;
                    auto const& id = board.getScoreboardId(sb);
                    if (id.mRawID == ScoreboardId::INVALID().mRawID) return false;
                    return board.resetPlayerScore(id, *obj);
                }
                case PIER_SB_SET_DISPLAY:
                {
                    // Native. setDisplayObjective already takes an ObjectiveSortOrder,
                    // and passing Ascending explicitly is the default the command
                    // applies when no sortOrder is given, so routing through
                    // /scoreboard would buy nothing and would lose the failure reason,
                    // such as a missing objective or a misspelled slot name.
                    //
                    // sa is the slot name, one of "sidebar", "list" or "belowname",
                    // and sb is the objective name.
                    auto* obj = board.getObjective(sb);
                    if (!obj) return false;
                    return board.setDisplayObjective(sa, *obj, ObjectiveSortOrder::Ascending) != nullptr;
                }
                case PIER_SB_CLEAR_DISPLAY:
                    // Returns the objective that was cleared, or nullptr when no
                    // objective was displayed, which also counts as success because
                    // the operation is idempotent. The return value is not read.
                    board.clearDisplayObjective(sa);
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        void fill(PierApi& api) { api.scoreboard_op = &api_scoreboard_op; }

        spi::SlotPackReg reg{{"scoreboard", &fill}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT

/** net/ScoreboardApi.cpp —— 计分板操作。
 *
 * 读写都走原生 Scoreboard（Level::getScoreboard）。计分身份用「假玩家」名
 * —— 和原版 /scoreboard 同一个命名空间，结果与游戏内状态对得上。 */
#ifndef PIER_BUILD_CLIENT

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
        /** 取或建一个假玩家名的 ScoreboardId。 */
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
                    // 原生。早先走 /scoreboard 的理由是「让排序默认值留在引擎
                    // 侧」，但 setDisplayObjective 本来就收 ObjectiveSortOrder，
                    // 显式传 Ascending 就是命令不带 sortOrder 时的默认值 —— 绕
                    // 命令并没有换来什么，反而丢了失败原因（目标不存在？槽位名
                    // 拼错？）。
                    //
                    // sa = 槽位名（"sidebar"/"list"/"belowname"），sb = 目标名。
                    auto* obj = board.getObjective(sb);
                    if (!obj) return false;
                    return board.setDisplayObjective(sa, *obj, ObjectiveSortOrder::Ascending) != nullptr;
                }
                case PIER_SB_CLEAR_DISPLAY:
                    // 返回被清掉的那个目标；本来就没有显示目标时返回 nullptr，
                    // 那也算成功（幂等），所以不看返回值。
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

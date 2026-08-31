/** runtime/Server.cpp —— 时钟、天气、难度、种子、游戏规则、服务器信息。
 *
 * 读取一律直连原生；对版本敏感的写入走原版命令（设计决策），BDS 升版不用
 * 跟着改。世界级写操作是服务端能力 —— 客户端构建整文件不编，槽位留 NULL。
  * 双目标编入（与旧构建矩阵一致）。写路径里 runConsoleCommand 在客户端恒 false，各槽位按各自的 level 判空降级。
 */
#include <string>
#include <variant>

#include "mc/common/Common.h"
#include "mc/common/SharedConstants.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/LevelSeed64.h"
#include "mc/world/level/storage/GameRule.h"
#include "mc/world/level/storage/GameRuleId.h"
#include "mc/world/level/storage/GameRules.h"

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
        bool api_get_time(int64_t* out)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !out) return false;
                *out = static_cast<int64_t>(level->getTime());
                return true;
            PIER_API_GUARD_END
        }

        bool api_set_time(int64_t t)
        {
            PIER_API_GUARD_BEGIN
                // 原生。读取侧本来就是 level->getTime()，写入侧再绕一圈命令的
                // 话 —— 同一个属性两条路，失败方式还不一样。
                auto* level = bridge::levelReady();
                if (!level) return false;
                level->setTime(static_cast<int>(t));
                return true;
            PIER_API_GUARD_END
        }

        bool api_set_weather(int32_t weather)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return false;
                // updateWeather(雨强度, 雨持续, 雷强度, 雷持续)。持续时间给 0
                // 表示由引擎自己按默认规则续 —— 和 /weather 不带秒数时一致。
                switch (weather)
                {
                case 1:
                    level->updateWeather(1.0f, 0, 0.0f, 0);
                    return true;
                case 2:
                    level->updateWeather(1.0f, 0, 1.0f, 0);
                    return true;
                case 0:
                    level->updateWeather(0.0f, 0, 0.0f, 0);
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_get_difficulty(int32_t* out)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !out) return false;
                *out = static_cast<int32_t>(level->getDifficulty());
                return true;
            PIER_API_GUARD_END
        }

        bool api_set_difficulty(int32_t d)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return false;
                if (d < 0 || d > 3) return false;
                level->setDifficulty(static_cast<::SharedTypes::Legacy::Difficulty>(d));
                return true;
            PIER_API_GUARD_END
        }

        bool api_get_seed(int64_t* out)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !out) return false;
                *out = static_cast<int64_t>(level->getLevelSeed64().mValue);
                return true;
            PIER_API_GUARD_END
        }

        bool api_game_rule_get(PierStr name, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !sink) return false;
                auto& rules = level->getGameRules();
                GameRuleId id = rules.nameToGameRuleIndex(toString(name));
                // NewType<int>：裸下标；越界 = 不认识的规则。
                int idx = id.mValue;
                auto const& list = rules.mGameRules.get();
                if (idx < 0 || static_cast<size_t>(idx) >= list.size()) return false;
                auto const& rule = list[static_cast<size_t>(idx)];

                std::string out;
                switch (rule.mType)
                {
                case GameRule::Type::Bool:
                    out = std::string{"{type:\"bool\",value:"} + (rule.getBool() ? "1b" : "0b") + "}";
                    break;
                case GameRule::Type::Int:
                    out = "{type:\"int\",value:" + snbtNum(rule.getInt()) + "}";
                    break;
                case GameRule::Type::Float:
                {
                    // 这个 LL 版本没有 getFloat() 访问器；读公开的 variant。
                    auto const& var = rule.mValue.get();
                    float f = std::holds_alternative<float>(var) ? std::get<float>(var) : 0.0f;
                    out = "{type:\"float\",value:" + snbtNum(f) + "f}";
                    break;
                }
                default:
                    return false;
                }
                sink(ctx, ps(out));
                return true;
            PIER_API_GUARD_END
        }

        bool api_game_rule_set(PierStr name, PierStr value)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return false;
                // 这一条**故意保留命令路径**。
                //
                // GameRules 只暴露了 getBool/getInt/getFloat 和
                // nameToGameRuleIndex，写入侧公开的只有带下划线的 _setGameRule
                // —— 那是内部接口，签名跨版本不稳，而且绕过它会漏掉
                // GameRulesChangedPacket 的广播（客户端不会知道规则变了）。
                // `/gamerule` 会把这些都做对。
                //
                // 先用 nameToGameRuleIndex 验一次名字，这样至少「规则名拼错」
                // 能和「命令执行失败」区分开 —— 那正是命令路径最难查的地方。
                std::string const rule = toString(name);
                if (!level->getGameRules().hasRule(level->getGameRules().nameToGameRuleIndex(rule)))
                {
                    return false;
                }
                // V-25：value 会拼进控制台命令 —— 只接受 true/false 或（可带负号的）
                // 整数，其余一律拒绝，别把调用方的任意文本送进命令解析器。
                std::string const val = toString(value);
                bool const isBool = (val == "true" || val == "false");
                bool isInt = !val.empty() && val.size() <= 11;
                for (size_t i = 0; i < val.size() && isInt; ++i)
                {
                    char const c = val[i];
                    if (!((c >= '0' && c <= '9') || (i == 0 && c == '-' && val.size() > 1))) isInt = false;
                }
                if (!isBool && !isInt) return false;
                return bridge::runConsoleCommand("gamerule " + rule + " " + val);
            PIER_API_GUARD_END
        }

        bool api_server_info_str(int32_t prop, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return false;
                switch (prop)
                {
                case PIER_SRV_BDS_VERSION:
                    sink(ctx, ps(Common::getGameVersionString()));
                    return true;
                case PIER_SRV_PROTOCOL_VERSION:
                    sink(ctx, ps(snbtNum(SharedConstants::NetworkProtocolVersion())));
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.get_time = &api_get_time;
            api.set_time = &api_set_time;
            api.set_weather = &api_set_weather;
            api.get_difficulty = &api_get_difficulty;
            api.set_difficulty = &api_set_difficulty;
            api.get_seed = &api_get_seed;
            api.game_rule_get = &api_game_rule_get;
            api.game_rule_set = &api_game_rule_set;
            api.server_info_str = &api_server_info_str;
        }

        spi::SlotPackReg reg{{"server", &fill}};
    } // namespace
} // namespace pier::api_impl

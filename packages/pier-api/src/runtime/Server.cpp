/** runtime/Server.cpp: clock, weather, difficulty, seed, game rules and server info.
 *
 * Every read goes straight to native. Version-sensitive writes go through vanilla
 * commands by design, so a BDS upgrade does not force a change here.
 *
 * Compiled into both targets. On the client runConsoleCommand is always false, and
 * each slot degrades on its own null level check.
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
                // Native. The read side is level->getTime() already, and routing the
                // write through a command would give one property two paths that fail
                // in different ways.
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
                // updateWeather(rain level, rain time, lightning level, lightning
                // time). A duration of 0 lets the engine extend it by its own default
                // rule, which matches /weather without a seconds argument.
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
                // NewType<int> is a bare index. Out of range means an unknown rule.
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
                    // This LL version has no getFloat() accessor, so the public
                    // variant is read instead.
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
                // This one deliberately keeps the command path. GameRules exposes
                // only getBool, getInt, getFloat and nameToGameRuleIndex. The only
                // public write is the internal _setGameRule, whose signature is
                // unstable across versions, and bypassing /gamerule would skip the
                // GameRulesChangedPacket broadcast so clients would never learn the
                // rule changed. nameToGameRuleIndex validates the name first, which
                // separates a misspelled rule from a failed command.
                std::string const rule = toString(name);
                if (!level->getGameRules().hasRule(level->getGameRules().nameToGameRuleIndex(rule)))
                {
                    return false;
                }
                // value is concatenated into a console command, so only true, false
                // or an optionally negative integer is accepted. Everything else is
                // refused rather than feeding caller text to the command parser.
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

/** runtime/Server.cpp: clock, weather, difficulty, seed, game rules and server info.
 *
 * Every read goes straight to native. Version-sensitive writes go through vanilla
 * commands by design, so a BDS upgrade does not force a change here.
 *
 * Compiled into both targets. On the client runConsoleCommand is always false, and
 * each slot degrades on its own null level check.
 */
#include <cctype>
#include <string>
#include <string_view>
#include <variant>

#include "mc/common/Common.h"
#include "mc/common/SharedConstants.h"
// SharedConstants.h only forward-declares SemVersionConstant, so
// CurrentGameSemVersion() returns a reference to an incomplete type and no field on it
// can be read. The definition, and SemVersionBase underneath it that actually carries
// mMajor / mMinor / mPatch / mPreRelease, come from here.
#include "mc/deps/core/sem_ver/SemVersionConstant.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/storage/LevelData.h"
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

        /**
         * The rule of that name, or null. A scan over mGameRules is the only route left:
         * GameRules exposes no name lookup and no membership test in 26.32, so the name a
         * caller gives is matched against GameRule::mName.
         *
         * The match folds case. GameRule::mName is spelled the way GameRulesIndex is,
         * `showCoordinates` rather than `showcoordinates`, while /gamerule and every
         * caller that predates this write it in lower case. An exact comparison found
         * `pvp` and nothing else, which is the shape this ASCII fold fixes. Rule names
         * are ASCII, so no locale enters into it.
         */
        bool sameRuleName(std::string_view a, std::string_view b)
        {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
            {
                auto const l = static_cast<unsigned char>(a[i]);
                auto const r = static_cast<unsigned char>(b[i]);
                if (std::tolower(l) != std::tolower(r)) return false;
            }
            return true;
        }

        ::GameRule const* findGameRule(::GameRules const& rules, std::string const& name)
        {
            for (auto const& rule : rules.mGameRules.get())
            {
                if (sameRuleName(rule.mName.get(), name)) return &rule;
            }
            return nullptr;
        }

        bool api_game_rule_get(PierStr name, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !sink) return false;
                auto const* rule = findGameRule(level->getGameRules(), toString(name));
                if (!rule) return false;

                // The value is read out of the public variant for every type. The
                // getBool and getInt accessors are compiled only on the client
                // platform, and this TU is built for both.
                auto const& var = rule->mValue.get();
                std::string out;
                switch (rule->mType)
                {
                case GameRule::Type::Bool:
                    if (!std::holds_alternative<bool>(var)) return false;
                    out = std::string{"{type:\"bool\",value:"} + (std::get<bool>(var) ? "1b" : "0b") + "}";
                    break;
                case GameRule::Type::Int:
                    if (!std::holds_alternative<int>(var)) return false;
                    out = "{type:\"int\",value:" + snbtNum(std::get<int>(var)) + "}";
                    break;
                case GameRule::Type::Float:
                    if (!std::holds_alternative<float>(var)) return false;
                    out = "{type:\"float\",value:" + snbtNum(std::get<float>(var)) + "f}";
                    break;
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
                // This one deliberately keeps the command path. The only public write
                // is the internal _setGameRule, whose signature is unstable across
                // versions, and bypassing /gamerule would skip the
                // GameRulesChangedPacket broadcast so clients would never learn the
                // rule changed. The name is validated against the rule table first,
                // which separates a misspelled rule from a failed command.
                std::string const rule = toString(name);
                if (!findGameRule(level->getGameRules(), rule)) return false;
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

        /** major.minor.patch of a BDS build to the network protocol it speaks.
         *
         * Matched on the exact triple: Bedrock bumps the protocol on patch releases, so
         * major.minor would answer from the wrong end of a release line.
         *
         * An unlisted version fails the slot. Do not interpolate and do not take the
         * nearest row: a nearly right protocol is a client that nearly connects, and
         * that surfaces as a handshake error with nothing pointing back here.
         * PIER_SRV_GAME_SEM_VERSION lets a mod carry its own table instead of waiting
         * on this one.
         */
        bool protocolForGameVersion(int major, int minor, int patch, int32_t* out)
        {
            struct Row
            {
                int major;
                int minor;
                int patch;
                int32_t protocol;
            };
            // Only versions actually confirmed against a running server. A row nobody
            // checked is worth less than no row at all, because no row fails loudly.
            static constexpr Row kKnown[] = {
                {1, 21, 93, 819},
                {1, 26, 40, 2169}, // retail; the beta of the same triple is 2168
            };
            for (auto const& row : kKnown)
            {
                if (row.major == major && row.minor == minor && row.patch == patch)
                {
                    if (out) *out = row.protocol;
                    return true;
                }
            }
            return false;
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
                {
                    // The protocol the running server speaks, not the one the level was
                    // written by. NetworkProtocolVersion is under #ifdef LL_PLAT_C and
                    // unreadable here; CurrentGameSemVersion is not gated. An unknown
                    // version fails rather than guesses: a caller given nothing says so,
                    // a caller given a plausible number acts on it.
                    auto const& sem = SharedConstants::CurrentGameSemVersion();
                    // A version that did not parse has zeroed fields, and 0.0.0 would
                    // miss every row anyway; refusing here says why instead.
                    if (!sem.mValidVersion) return false;
                    // A pre-release shares its triple with the retail build but not its
                    // protocol, so it is refused rather than answered from a retail row.
                    if (!sem.mPreRelease.empty()) return false;
                    int32_t protocol = 0;
                    if (!protocolForGameVersion(sem.mMajor, sem.mMinor, sem.mPatch, &protocol))
                    {
                        return false;
                    }
                    sink(ctx, ps(snbtNum(protocol)));
                    return true;
                }
                case PIER_SRV_LEVEL_PROTOCOL_VERSION:
                {
                    // LevelData::mNetworkVersion, under the name it actually deserves.
                    // Kept because it answers a real question — how old is this save —
                    // which is exactly what it was accidentally answering before.
                    auto* level = bridge::levelReady();
                    if (!level) return false;
                    sink(ctx, ps(snbtNum(level->getLevelData().mNetworkVersion)));
                    return true;
                }
                case PIER_SRV_GAME_SEM_VERSION:
                {
                    // major.minor.patch of the running build, so a mod can carry its own
                    // table and stop depending on this one being current. A Pier release
                    // should not be on the critical path for a mod supporting a BDS that
                    // shipped yesterday.
                    auto const& sem = SharedConstants::CurrentGameSemVersion();
                    if (!sem.mValidVersion) return false;
                    // The temporary string lives to the end of this full expression, so
                    // the view ps() takes is still valid while sink runs.
                    sink(ctx, ps(std::to_string(static_cast<int>(sem.mMajor)) + "."
                                 + std::to_string(static_cast<int>(sem.mMinor)) + "."
                                 + std::to_string(static_cast<int>(sem.mPatch))));
                    return true;
                }
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

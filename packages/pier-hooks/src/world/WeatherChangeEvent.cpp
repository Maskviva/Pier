/** world/WeatherChangeEvent.cpp —— 天气被改了。
 *
 * 只观察，不可取消：`updateWeather` 是引擎自己的天气状态机在推进，中途拦下
 * 会让下雨计时器和实际天气不一致（表现是「一直下雨但雨停了」）。要控制天气
 * 用 `level_update_weather` 覆盖，或者拿 gamerule 关掉天气循环。
 *
 * 载荷是四个原始参数：降雨强度/时长、闪电强度/时长。强度 0 表示停。
 */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/Level.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& weatherDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            LevelUpdateWeatherHook,
            ll::memory::HookPriority::Normal,
            Level,
            &Level::$updateWeather,
            void,
            float rainLevel,
            int rainTime,
            float lightningLevel,
            int lightningTime)
        {
            auto& def = weatherDef();
            if (!def.live()) return origin(rainLevel, rainTime, lightningLevel, lightningTime);

            std::string snbt = "{\"eventId\":\"WeatherChangeEvent\""
                ",\"rainLevel\":" + snbtDouble(rainLevel)
                + ",\"rainTime\":" + snbtNum(rainTime)
                + ",\"lightningLevel\":" + snbtDouble(lightningLevel)
                + ",\"lightningTime\":" + snbtNum(lightningTime) + "}";
            dispatchHookEvent(def, snbt); // 只观察

            origin(rainLevel, rainTime, lightningLevel, lightningTime);
        }

        HookEventDef gDef{
            "WeatherChangeEvent",
            []
            {
                int const r = LevelUpdateWeatherHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[WeatherChangeEvent] Level::$updateWeather 的 detour 安装失败（code={}）。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& weatherDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT

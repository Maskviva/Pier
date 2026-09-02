/** world/WeatherChangeEvent.cpp: the weather changed.
 * Observation only, not cancellable: `updateWeather` is the engine's own weather state
 * machine advancing, and stopping it midway leaves the rain timer disagreeing with the
 * actual weather, which shows up as raining forever while the rain has stopped.
 * Controlling the weather means overriding it through `level_update_weather` or turning
 * the weather cycle off with a gamerule.
 * The payload is the four raw parameters: rain level and duration, lightning level and
 * duration. A level of 0 means stopped. */
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
        HookEventDef& weatherDef(); // Forward declaration

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
            dispatchHookEvent(def, snbt); // Observation only

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
                        "[hooks/WeatherChangeEvent] the Level::$updateWeather detour failed to install with code={}", r);
                }
                return r == 0;
            }
        };
        HookEventDef& weatherDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT

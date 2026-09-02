/** core/GameStats.cpp: read-only game state slots for gaming_status, tick, player
 *  count and pause. */
#include "ll/api/service/GamingStatus.h"

#include "mc/world/level/Level.h"
#include "mc/world/level/Tick.h"
#include "mc/world/level/TickDeltaTimeManager.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"

namespace pier::api_impl
{
    namespace
    {
        int32_t api_gaming_status()
        {
            PIER_API_GUARD_BEGIN
                return static_cast<int32_t>(ll::getGamingStatus());
            PIER_API_GUARD_END
        }

        uint64_t api_get_current_tick()
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return 0;
                return level->getCurrentTick().tickID;
            PIER_API_GUARD_END
        }

        double api_get_tick_delta_time()
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return -1.0;
                return level->getTickDeltaTimeManager()->mTickDeltaTime;
            PIER_API_GUARD_END_VAL(-1.0)
        }

        int32_t api_get_player_count()
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return 0;
                return static_cast<int32_t>(level->getActivePlayerCount());
            PIER_API_GUARD_END
        }

        bool api_get_sim_paused()
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return true; // Safe default: unknown counts as paused
                return level->getSimPaused();
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.gaming_status = &api_gaming_status;
            api.get_current_tick = &api_get_current_tick;
            api.get_tick_delta_time = &api_get_tick_delta_time;
            api.get_player_count = &api_get_player_count;
            api.get_sim_paused = &api_get_sim_paused;
        }

        spi::SlotPackReg reg{{"game-stats", &fill}};
    } // namespace
} // namespace pier::api_impl

/** core/Log.cpp —— 模组日志槽。一个槽一个 TU 显得小题大做，但 log 是唯一
 *  「必须在任何别的槽可用之前就能用」的槽 —— 它不许染上任何域的依赖。 */
#include "ll/api/io/LogLevel.h"
#include "ll/api/io/Logger.h"

#include "sdk/abi.h"

#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        void api_log(PierModHandle mod, int32_t level, PierStr msg)
        {
            PIER_API_GUARD_BEGIN
                if (!mod) return;
                auto& logger = asMod(mod)->getLogger();
                switch (static_cast<ll::io::LogLevel>(level))
                {
                case ll::io::LogLevel::Fatal:
                    logger.fatal("{}", sv(msg));
                    break;
                case ll::io::LogLevel::Error:
                    logger.error("{}", sv(msg));
                    break;
                case ll::io::LogLevel::Warn:
                    logger.warn("{}", sv(msg));
                    break;
                case ll::io::LogLevel::Debug:
                    logger.debug("{}", sv(msg));
                    break;
                case ll::io::LogLevel::Trace:
                    logger.trace("{}", sv(msg));
                    break;
                case ll::io::LogLevel::Off:
                    break;
                case ll::io::LogLevel::Info:
                default:
                    logger.info("{}", sv(msg));
                    break;
                }
            PIER_API_GUARD_END_VOID
        }

        void fill(PierApi& api) { api.log = &api_log; }

        spi::SlotPackReg reg{{"log", &fill}};
    } // namespace
} // namespace pier::api_impl

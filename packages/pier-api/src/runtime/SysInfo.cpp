/** runtime/SysInfo.cpp —— 系统信息与环境变量。纯 OS 调用，双目标均编入。 */
#include <string>

#include "ll/api/utils/SystemUtils.h"

#include "sdk/abi.h"

#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        bool api_sys_info_str(int32_t prop, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return false;
                switch (prop)
                {
                case PIER_SYS_OS_NAME:
                    sink(ctx, ps(ll::sys_utils::getSystemName()));
                    return true;
                case PIER_SYS_OS_VERSION:
                    sink(ctx, ps(ll::sys_utils::getSystemVersion().to_string()));
                    return true;
                case PIER_SYS_LOCALE:
                    sink(ctx, ps(ll::sys_utils::getSystemLocaleCode()));
                    return true;
                case PIER_SYS_LOCAL_TIME:
                {
                    auto t = ll::sys_utils::getLocalTime();
                    // std::tm 字段 + ms，规整成人读的数值。
                    std::string out = "{year:" + snbtNum(t.tm_year + 1900)
                        + ",month:" + snbtNum(t.tm_mon + 1)
                        + ",day:" + snbtNum(t.tm_mday)
                        + ",hour:" + snbtNum(t.tm_hour)
                        + ",minute:" + snbtNum(t.tm_min)
                        + ",second:" + snbtNum(t.tm_sec)
                        + ",ms:" + snbtNum(t.ms) + "}";
                    sink(ctx, ps(out));
                    return true;
                }
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_sys_get_env(PierStr name, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return false;
                auto value = ll::sys_utils::getEnvironmentVariable(sv(name));
                sink(ctx, ps(value));
                return true;
            PIER_API_GUARD_END
        }

        bool api_sys_set_env(PierStr name, PierStr value)
        {
            PIER_API_GUARD_BEGIN
                return ll::sys_utils::setEnvironmentVariable(sv(name), sv(value));
            PIER_API_GUARD_END
        }

        bool api_sys_is_wine()
        {
            PIER_API_GUARD_BEGIN
                return ll::sys_utils::isWine();
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.sys_info_str = &api_sys_info_str;
            api.sys_get_env = &api_sys_get_env;
            api.sys_set_env = &api_sys_set_env;
            api.sys_is_wine = &api_sys_is_wine;
        }

        spi::SlotPackReg reg{{"sys-info", &fill}};
    } // namespace
} // namespace pier::api_impl

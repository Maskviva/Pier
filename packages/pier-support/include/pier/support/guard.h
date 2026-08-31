#pragma once
// 每个 `api_*` 入口都是别的语言一条 `extern "C"` 帧的下面一层。
// C++ 异常穿过语言边界是 UB（实际表现是无日志 abort）。
// 用法：函数体首行 PIER_API_GUARD_BEGIN，末行按返回值选一个 END。
//
// `return {};` 对 bool / 整数 / 指针 / 句柄 / 按值结构体都是「零值」——
// 恰好是绝大多数入口的失败值。失败值不是零的入口必须用 _VAL：
// PIER_SERVICE_OK / PIER_LANE_OK 都是 0，对它们 `return {}` 等于把异常
// 报成「调用成功」，那正是这道屏障要防的反面；同理 -1 / -1.0 是
// cooldown、chunk、tick-delta 这几族约定的失败值。
#include "ll/api/utils/ErrorUtils.h"

#include "pier/support/log.h"

#define PIER_API_GUARD_BEGIN try {

#define PIER_API_GUARD_END                                                                         \
    }                                                                                              \
    catch (...)                                                                                    \
    {                                                                                              \
        ll::error_utils::printCurrentException(::pier::hostLogger());                              \
        return {};                                                                                 \
    }

#define PIER_API_GUARD_END_VAL(failure)                                                            \
    }                                                                                              \
    catch (...)                                                                                    \
    {                                                                                              \
        ll::error_utils::printCurrentException(::pier::hostLogger());                              \
        return (failure);                                                                          \
    }

#define PIER_API_GUARD_END_VOID                                                                    \
    }                                                                                              \
    catch (...)                                                                                    \
    {                                                                                              \
        ll::error_utils::printCurrentException(::pier::hostLogger());                              \
    }

#pragma once
// Every `api_*` entry point sits one frame below an `extern "C"` frame of another
// language. A C++ exception crossing that boundary is undefined behavior, and in
// practice aborts the process without a log line.
// Usage: PIER_API_GUARD_BEGIN on the first line of the body, one of the END forms
// on the last, chosen by return type.
//
// `return {};` yields the zero value for bool, integers, pointers, handles and
// by-value structs, which is the failure value of almost every entry point. An
// entry point whose failure value is not zero must use _VAL. PIER_SERVICE_OK and
// PIER_LANE_OK are both 0, so `return {}` there reports an exception as a
// successful call, which is exactly what this barrier exists to prevent. The same
// holds for -1 and -1.0, the agreed failure values of the cooldown, chunk and
// tick-delta families.
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

#pragma once
// 宿主自己的 Logger（不是某个被托管模组的）。
// 单独成包级入口的原因：guard.h 和所有能力包都要打日志，而它们都
// 不许 include pier-host（契约 §一）—— 日志必须住在比 host 更低的层。
#include "ll/api/io/Logger.h"

namespace pier
{
    ll::io::Logger& hostLogger();
} // namespace pier

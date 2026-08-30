#include "pier/host/api_table.h"

namespace pier
{
    namespace
    {
        // 值初始化：所有函数指针起始为 NULL。这不是巧合而是契约 ——
        // 「能力缺席 = 槽位 NULL」（§2.1）的第一半就在这一行；
        // 第二半是缺席的包根本不会注册槽位包来覆盖它。
        PierApi gApi{};
    } // namespace

    PierApi& mutableApi() noexcept { return gApi; }
    PierApi const* bridgeApi() noexcept { return &gApi; }
} // namespace pier

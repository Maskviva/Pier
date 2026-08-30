#include "pier/support/log.h"

#include "ll/api/mod/NativeMod.h"

namespace pier
{
    ll::io::Logger& hostLogger()
    {
        // NativeMod::current() 按调用方所在映像解析 —— 所有包都编进同一个
        // pier.dll，所以这里拿到的恒是加载器自己的 mod。
        return ll::mod::NativeMod::current()->getLogger();
    }
} // namespace pier

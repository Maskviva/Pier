#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "ll/api/event/ListenerBase.h"
#include "ll/api/mod/Manifest.h"
#include "ll/api/mod/Mod.h"
#include "ll/api/utils/SystemUtils.h"

#include "sdk/abi.h"

namespace pier
{
    /** manifest 里 `"type"` 的取值。用户可见字符串同受语言名禁令
     *（契约 §七）：这里写 "pier"，不写任何一门语言的名字 —— 这个
     *  管理器装的是「说 Pier ABI 的 cdylib」，什么语言编出来的它不知道。 */
    inline constexpr std::string_view ModHostName = "pier";

    /**
     * 一个 `"type": "pier"` 的模组：一个普通 cdylib，经 PierApi 函数表
     * 契约装载，而不是走 C++ 原生 mod 的 ABI。
     */
    class HostedMod : public ll::mod::Mod, public std::enable_shared_from_this<HostedMod>
    {
    public:
        explicit HostedMod(ll::mod::Manifest manifest) : Mod(std::move(manifest)) {}

        ll::sys_utils::DynamicLibrary lib;
        PierModVTable vtable{};

        /**
         * 保持 DynamicListener 存活；卸载时清空。
         *
         * 用进程内单调 id 索引，**不是**监听器的地址。地址是最直觉的句柄，也
         * 是不安全的那个：退订会释放监听器，下一次订阅完全可能落在同一块内存
         * 上，于是一个 mod 忘了丢弃的过期句柄会匹配上另一条订阅 —— 悄无声息
         * 地退掉别人的监听器。id 永不复用，过期句柄只会匹配失败。
         */
        struct ListenerSlot
        {
            std::uint64_t id;
            std::shared_ptr<ll::event::ListenerBase> listener;
        };

        std::vector<ListenerSlot> listeners;

        /** 禁用后置真：已注册的命令回调全部变 no-op（命令没法真注销）。 */
        bool commandsMuted = false;

        /**
         * 正在执行本模组回调的栈帧数（跨线程累计）。
         *
         * 用途：ModHost::unload 在它非零时拒绝卸载 —— 否则 FreeLibrary 会发生在
         * 模组自己的栈帧之下（回调里 execute_command("pier unload <self>")），
         * 或另一线程正在派发它的总线/数据包回调时把代码段抽走。
         * 各派发点用 CallbackScope 维护它；忘记包的派发点只是失去这层保护，
         * 不会引入新错误。
         */
        std::atomic<int> inCallback{0};
    };

    /** 派发模组回调时的 RAII 计数：构造 +1，析构 -1。mod 为空时什么都不做。 */
    class CallbackScope
    {
    public:
        explicit CallbackScope(HostedMod* mod) noexcept : mMod(mod)
        {
            if (mMod) mMod->inCallback.fetch_add(1, std::memory_order_acq_rel);
        }
        ~CallbackScope()
        {
            if (mMod) mMod->inCallback.fetch_sub(1, std::memory_order_acq_rel);
        }
        CallbackScope(CallbackScope const&) = delete;
        CallbackScope& operator=(CallbackScope const&) = delete;

    private:
        HostedMod* mMod;
    };

    /** PierModHandle ↔ HostedMod*。句柄就是指针本身 —— 生命周期由
     *  ModHost 的表保证：卸载先走全部 Teardown 再 erase（见 mod_host.cpp）。 */
    [[nodiscard]] inline HostedMod* asMod(PierModHandle h) noexcept
    {
        return static_cast<HostedMod*>(h);
    }
} // namespace pier

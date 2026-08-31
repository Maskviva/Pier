#include "pier/host/mod_host.h"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ll/api/event/EventBus.h"
#include "ll/api/mod/ModManagerRegistry.h"
#include "ll/api/utils/StringUtils.h"

#include "pier/host/api_table.h"
#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/log.h"

namespace pier
{
    using ll::mod::Manifest;

    namespace
    {
        ModHost* gInstance = nullptr;
    } // namespace

    ModHost::ModHost() : ModManager(ModHostName) { gInstance = this; }

    ModHost::~ModHost() { gInstance = nullptr; }

    ModHost* ModHost::instance() { return gInstance; }

    ll::Expected<> ModHost::load(Manifest manifest)
    {
        auto mod = std::make_shared<HostedMod>(std::move(manifest));

        std::error_code ec;
        auto modDir = ll::mod::getModsRoot() / ll::string_utils::sv2u8sv(mod->getName());
        if (auto c = std::filesystem::canonical(modDir, ec); ec.value() == 0)
        {
            modDir = c;
        }
        else
        {
            modDir = modDir.lexically_normal();
        }
        auto entry = modDir / ll::string_utils::sv2u8sv(mod->getManifest().entry);

        if (auto e = mod->lib.load(entry); e)
        {
            return ll::makeExceptionError(std::make_exception_ptr(*e));
        }

        // 唯一的入口符号。找不到就明确拒绝 —— 不做任何历史别名回退
        //（契约 §2.4：回退意味着两个名字永远都不能改）。
        auto main = mod->lib.getAddress<PierMainFn>(PIER_MAIN_SYMBOL);
        if (!main)
        {
            // pier_main 尚未被调用：模组一行代码都没跑，没有任何登记可拆。
            (void)mod->lib.free();
            return ll::makeStringError(
                "'" + mod->getName() + "' 没有导出 " PIER_MAIN_SYMBOL
                " —— 它是一个 Pier 模组吗？（SDK 的注册宏会替你导出这个符号）"
            );
        }

        mod->vtable = PierModVTable{};

        /* ── 拒绝路径的统一出口（V-05） ──────────────────────────────────
         * pier_main 一旦被调用，模组就可能已经订阅了事件、注册了总线/服务/
         * 数据包钩子、排了任务。此后任何一条拒绝路径都必须先把这些登记全部
         * 拆掉再 FreeLibrary —— 否则 EventBus 和各注册表里留着指向已 unmap
         * 代码段的回调，下一次事件触发就是 use-after-free。
         * SDK 侧（pier-rs）虽然在 on_load 之前自检版本/flag，但 ABI 面向任意
         * 语言，宿主不能依赖对方的自觉；而且 on_load 部分注册后返回失败是
         * 契约 §5.3 自己讨论过的正常场景。 */
        auto abandon = [&](std::string why) -> ll::Expected<>
        {
            auto& bus = ll::event::EventBus::getInstance();
            for (auto& slot : mod->listeners)
            {
                if (slot.listener && !bus.removeListener(slot.listener))
                {
                    mod->getLogger().error(
                        "'{}' 装载失败后退订监听器 #{} 失败 —— 这条监听器可能残留在 EventBus 里",
                        mod->getName(), slot.id
                    );
                }
            }
            mod->listeners.clear();
            spi::runTeardown(mod.get());
            (void)mod->lib.free();
            return ll::makeStringError(std::move(why));
        };

        if (!main(bridgeApi(), static_cast<PierModHandle>(mod.get()), &mod->vtable))
        {
            return abandon("'" + mod->getName() + "' 的 " PIER_MAIN_SYMBOL " 返回 false");
        }

        /* ── v1 握手：先看长度，再看版本，再看目标 ──────────────────────
         * vtable 自带 struct_size（契约 §2.3），宿主只读模组声明长度以内
         * 的字段。顺序有讲究：长度不够时连 abi_version 都不可信，所以
         * 长度检查必须最先。 */
        auto const& vt = mod->vtable;
        if (vt.struct_size < sizeof(PierModVTable))
        {
            return abandon(
                "'" + mod->getName() + "' 填回的 vtable 只有 " + std::to_string(vt.struct_size)
                + " 字节，宿主需要至少 " + std::to_string(sizeof(PierModVTable))
                + " —— 它的 SDK 没有填 struct_size，或早于 ABI v1"
            );
        }

        // 兼容是一个区间，不是相等（契约 §2.2）。只追加的演进意味着新宿主
        // 能跑旧模组：旧模组调用的是我们表的一个逐字节相同的前缀，永远够
        // 不到它不认识的尾部槽位。
        //
        //   太新（mod_abi > 宿主）→ 模组可能调用我们没有的槽。拒绝；升级宿主。
        //   太旧（mod_abi < 下限）→ 早于一次非追加断裂，我们的表已不是它期待
        //                            的前缀。拒绝；重编模组。
        //   区间内 → 安全，装。
        //
        // 反向偏斜（旧宿主 + 新模组）由模组侧逐槽比对 struct_size 兜住
        //（require_slot!），宿主不用管。
        if (vt.abi_version > PIER_ABI_VERSION)
        {
            return abandon(
                "'" + mod->getName() + "' 按 Pier ABI v" + std::to_string(vt.abi_version)
                + " 编译，而本宿主最高只到 v" + std::to_string(PIER_ABI_VERSION) + " —— 升级 pier 宿主"
            );
        }
        if (vt.abi_version < PIER_ABI_MIN_SUPPORTED)
        {
            return abandon(
                "'" + mod->getName() + "' 按 Pier ABI v" + std::to_string(vt.abi_version)
                + " 编译，低于本宿主支持的下限 v" + std::to_string(PIER_ABI_MIN_SUPPORTED)
                + " —— 用当前的 pier SDK 重新编译该模组"
            );
        }

        // 目标匹配走 flags 的 bit 0（契约 §2.3）。布局在所有目标下相同，
        // 所以错配**不会**造成槽位错位 —— 这个检查防的是语义层面的荒唐：
        // 服务端宿主里跑一个只会调 client_* 空槽的模组，每一步都「安全地」
        // 失败，不如装载时就把话说清楚。
        uint32_t const hostFlags = bridgeApi()->host_flags;
        if ((vt.mod_flags ^ hostFlags) & PIER_FLAG_CLIENT)
        {
            bool const modIsClient = (vt.mod_flags & PIER_FLAG_CLIENT) != 0;
            return abandon(
                "'" + mod->getName() + "' 是按" + (modIsClient ? "客户端" : "服务端")
                + "目标编译的，而这个宿主是" + (modIsClient ? "服务端" : "客户端")
                + "构建 —— 用匹配的目标重新编译该模组"
            );
        }
        // 未知位必须为零：这是「保留」二字的全部含义。现在不严，将来这些
        // 位就没法再用 —— 老模组里会躺着随机脏值。
        if ((vt.mod_flags & ~PIER_FLAG_CLIENT) != 0 || vt._reserved0 != 0)
        {
            return abandon(
                "'" + mod->getName() + "' 的 vtable 里保留位非零（mod_flags=0x"
                + ll::string_utils::intToHexStr(vt.mod_flags) + "）—— SDK 有 bug，或按未来的 ABI 编译"
            );
        }
        if (vt.abi_version != PIER_ABI_VERSION)
        {
            // 收下，但把偏斜记下来：野外的「版本不匹配」报告要一眼能核对。
            // 模组跑在它当年那张表的严格超集上。
            hostLogger().info(
                "'{}' 按 ABI v{} 编译；宿主提供 v{}（追加超集）—— 装载",
                mod->getName(),
                vt.abi_version,
                PIER_ABI_VERSION
            );
        }

        // 把 Mod 的生命周期回调接到模组的 vtable 上。ModManager 默认的
        // enable()/disable() 会调它们（见 ll/api/mod/ModManager.cpp）。
        mod->onEnable([](ll::mod::Mod& self)
        {
            auto& hosted = static_cast<HostedMod&>(self);
            hosted.commandsMuted = false;
            auto* fn = hosted.vtable.on_enable;
            return fn ? fn(hosted.vtable.instance) : true;
        });
        mod->onDisable([](ll::mod::Mod& self)
        {
            auto& hosted = static_cast<HostedMod&>(self);
            bool const ok =
                hosted.vtable.on_disable ? hosted.vtable.on_disable(hosted.vtable.instance) : true;
            hosted.commandsMuted = true;
            return ok;
        });

        addMod(mod->getName(), mod);
        return {};
    }

    ll::Expected<> ModHost::unload(std::string_view name)
    {
        auto const mod = std::static_pointer_cast<HostedMod>(getMod(name));
        if (!mod)
        {
            return ll::makeStringError("没有名为 '" + std::string(name) + "' 的 pier 模组");
        }

        // 否决在 on_unload **之前**问，而不是之后：否决的意义是「现在根本
        // 不能卸」，那就不该先让模组跑完自己的收尾逻辑再告诉它卸不掉。
        // 典型否决方是 lane：有个栈帧正停在这个模组提供的车道表项里 ——
        // 十有八九就是当前这层调用链自己（提供方的表项触发了命令派发，那
        // 条命令要卸载提供方）。这时 FreeLibrary 会把仍在执行的代码段
        // unmap 掉。
        if (auto veto = spi::askUnloadVetoes(mod.get()))
        {
            return ll::makeStringError(
                "'" + std::string(name) + "' 现在不能卸载 —— " + std::string(veto->who)
                + " 否决：" + veto->reason
            );
        }

        // 通用否决（V-06/V-28）：这个模组的某个回调正在执行 —— 要么是当前调用
        // 链自己（回调里 execute_command("pier unload <self>")），要么是另一
        // 线程正在派发它的总线/数据包回调。两种情况下 FreeLibrary 都会把正在
        // 执行的代码段 unmap 掉。lane 的 busy 只覆盖车道；这个计数覆盖宿主的
        // 全部派发点。
        if (int const depth = mod->inCallback.load(std::memory_order_acquire); depth > 0)
        {
            return ll::makeStringError(
                "'" + std::string(name) + "' 现在不能卸载 —— 它有 " + std::to_string(depth)
                + " 个回调正在执行（回调里卸载自己，或另一线程正在派发它的回调）。"
                  "请在回调返回后重试。"
            );
        }

        if (mod->vtable.on_unload && !mod->vtable.on_unload(mod->vtable.instance))
        {
            return ll::makeStringError("'" + std::string(name) + "' 拒绝卸载（on_unload 返回 false）");
        }
        mod->commandsMuted = true;

        // W-EV1：`listeners.clear()` 只丢掉宿主自己那份 shared_ptr。
        // EventBus 的 EventStorage 存的是**强引用**（OrderedSet<ListenerPtr>），
        // 所以清空这个 vector 一个监听器都没摘下来 —— 它们连同指向即将被
        // unmap 的 dylib 的回调，继续挂在总线上。必须显式 removeListener。
        for (auto const& l : mod->listeners)
        {
            if (!ll::event::EventBus::getInstance().removeListener(l.listener))
            {
                mod->getLogger().error(
                    "卸载 '{}' 时摘不下监听器 {}：它可能仍挂在事件总线上，"
                    "而它的回调指向马上要被卸载的 dylib。",
                    name,
                    l.id
                );
            }
        }
        mod->listeners.clear();

        // 各能力包按 stage 升序清掉自己名下属于这个模组的一切
        //（契约 §一 规则二；顺序背后的不变量写在 spi.h）。
        spi::runTeardown(mod.get());

        if (auto const e = mod->lib.free(); e)
        {
            return ll::makeExceptionError(std::make_exception_ptr(*e));
        }
        eraseMod(name);
        return {};
    }

    /* ───────────────────── 运行期模组控制 ───────────────────── */

    ll::Expected<> ModHost::controlLoad(Manifest manifest)
    {
        std::string const name = manifest.name;
        if (auto e = load(std::move(manifest)); !e)
        {
            return e;
        }
        // LeviLamina 自己的流程是 load → enable；ModManager::load 只把
        // dylib 拉起来并跑 pier_main。少了这步，模组装着但禁用：命令静音、
        // on_enable 永远不来。
        if (auto e = enable(name); !e)
        {
            // 回滚而不是留一个半活的模组：装载了却从未启用的模组仍然
            // 占着它的 dylib 和监听器。
            (void)unload(name);
            return e;
        }
        return {};
    }

    ll::Expected<> ModHost::controlUnload(std::string_view name)
    {
        auto const mod = std::static_pointer_cast<HostedMod>(getMod(name));
        if (!mod)
        {
            return ll::makeStringError("'" + std::string(name) + "' 没有装载");
        }
        // 先 disable() 让 on_disable 真的跑到；直接 unload() 只有 on_unload，
        // 模组永远见不到自己的禁用阶段。
        if (mod->isEnabled())
        {
            if (auto e = disable(name); !e)
            {
                return e;
            }
        }
        return unload(name);
    }

    void const* ModHost::moduleBase(std::string_view name) const
    {
        auto const mod = std::static_pointer_cast<HostedMod>(getMod(name));
        if (!mod) return nullptr;
        return mod->lib.handle(); // HandleT 就是 void*，只差限定符转换
    }

    std::vector<std::string> ModHost::loadedNames() const
    {
        std::vector<std::string> out;
        for (auto& mod : mods())
        {
            out.push_back(mod.getName());
        }
        return out;
    }

    std::vector<std::shared_ptr<HostedMod>> ModHost::hostedMods() const
    {
        std::vector<std::shared_ptr<HostedMod>> out;
        for (auto const& name : loadedNames())
        {
            if (auto m = std::static_pointer_cast<HostedMod>(getMod(name))) out.push_back(std::move(m));
        }
        return out;
    }
} // namespace pier

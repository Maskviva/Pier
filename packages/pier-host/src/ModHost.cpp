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

        // The only entry symbol. A miss is refused outright, with no fallback to a
        // legacy alias (contract §2.4, a fallback means neither name can ever
        // change).
        auto main = mod->lib.getAddress<PierMainFn>(PIER_MAIN_SYMBOL);
        if (!main)
        {
            // pier_main has not been called, so the mod ran no code and there is
            // nothing registered to tear down.
            (void)mod->lib.free();
            return ll::makeStringError(
                "'" + mod->getName() + "' does not export " PIER_MAIN_SYMBOL
                "; the entry symbol is exported by the SDK registration macro"
            );
        }

        mod->vtable = PierModVTable{};

        /*  Single exit for every rejection path
         * Once pier_main has been called the mod may already have subscribed to
         * events, registered bus, service and packet hooks, and scheduled tasks.
         * Every rejection path from here on must tear all of that down before
         * FreeLibrary, otherwise EventBus and the registries keep callbacks pointing
         * into an unmapped code section and the next event is a use-after-free.
         * The SDK side in pier-rs does check version and flags before on_load, but
         * the ABI faces any language and the host cannot rely on that. Returning
         * failure from on_load after partial registration is a normal case that
         * contract §5.3 discusses. */
        auto abandon = [&](std::string why) -> ll::Expected<>
        {
            auto& bus = ll::event::EventBus::getInstance();
            for (auto& slot : mod->listeners)
            {
                if (slot.listener && !bus.removeListener(slot.listener))
                {
                    mod->getLogger().error(
                        "[host] load rollback for '{}' could not remove listener {}; "
                        "it may remain on the EventBus with a callback into an image "
                        "about to be unmapped",
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
            return abandon("'" + mod->getName() + "': " PIER_MAIN_SYMBOL " returned false");
        }

        /*  Handshake: size first, then version, then target
         * The vtable carries its own struct_size (contract §2.3) and the host reads
         * only fields within the length the mod declares. The order matters, because
         * when the length is too small even abi_version is untrustworthy, so the
         * length check comes first. */
        auto const& vt = mod->vtable;
        if (vt.struct_size < sizeof(PierModVTable))
        {
            return abandon(
                "'" + mod->getName() + "' filled in a vtable of " + std::to_string(vt.struct_size)
                + " bytes, the host requires at least " + std::to_string(sizeof(PierModVTable))
                + "; its SDK does not set struct_size, or predates ABI v1"
            );
        }

        // Compatibility is a range and not an equality (contract §2.2). Append-only
        // evolution lets a new host run an old mod, which calls a byte-identical
        // prefix of the host table and cannot reach slots it does not know. A mod_abi
        // above the host may call a slot the host lacks, so it is refused with a
        // prompt to upgrade the host. A mod_abi below the floor predates a
        // non-additive break, so the host table is no longer the prefix it expects and
        // it is refused with a prompt to rebuild the mod. The reverse skew is covered
        // on the mod side by per-slot struct_size checks.
        if (vt.abi_version > PIER_ABI_VERSION)
        {
            return abandon(
                "'" + mod->getName() + "' was built against Pier ABI v" + std::to_string(vt.abi_version)
                + ", this host supports up to v" + std::to_string(PIER_ABI_VERSION)
                + "; upgrade the pier host"
            );
        }
        if (vt.abi_version < PIER_ABI_MIN_SUPPORTED)
        {
            return abandon(
                "'" + mod->getName() + "' was built against Pier ABI v" + std::to_string(vt.abi_version)
                + ", below the minimum v" + std::to_string(PIER_ABI_MIN_SUPPORTED)
                + " this host supports; rebuild the mod against the current pier SDK"
            );
        }

        // Target matching uses bit 0 of flags (contract §2.3). The layout is the same
        // on every target, so a mismatch cannot misalign slots. This check guards
        // against a semantic absurdity instead. A mod that only ever calls empty
        // client_* slots would run in a server host and fail safely at every step,
        // which is worse than saying so at load time.
        uint32_t const hostFlags = bridgeApi()->host_flags;
        if ((vt.mod_flags ^ hostFlags) & PIER_FLAG_CLIENT)
        {
            bool const modIsClient = (vt.mod_flags & PIER_FLAG_CLIENT) != 0;
            return abandon(
                "'" + mod->getName() + "' was built for the " + (modIsClient ? "client" : "server")
                + " target, this host is a " + (modIsClient ? "server" : "client")
                + " build; rebuild the mod for the matching target"
            );
        }
        // Unknown bits must be zero, which is the entire meaning of "reserved". Being
        // lax now would make those bits unusable later, because older mods would carry
        // arbitrary values in them.
        if ((vt.mod_flags & ~PIER_FLAG_CLIENT) != 0 || vt._reserved0 != 0)
        {
            return abandon(
                "'" + mod->getName() + "' has non-zero reserved bits in its vtable (mod_flags=0x"
                + ll::string_utils::intToHexStr(vt.mod_flags) + "); its SDK is faulty, or it was built against a future ABI"
            );
        }
        if (vt.abi_version != PIER_ABI_VERSION)
        {
            // Accepted, with the skew recorded so that a version mismatch report from
            // the field can be checked at a glance. The mod runs on a strict superset
            // of the table it was built against.
            hostLogger().info(
                "[host] loading '{}': built against ABI v{}, host provides v{} (additive superset)",
                mod->getName(),
                vt.abi_version,
                PIER_ABI_VERSION
            );
        }

        // Wire the Mod lifecycle callbacks onto the mod's vtable. The default
        // enable() and disable() of ModManager call them (see
        // ll/api/mod/ModManager.cpp).
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
            return ll::makeStringError("no pier mod named '" + std::string(name) + "' is loaded");
        }

        // Vetoes are asked before on_unload and not after. A veto means the mod
        // cannot be unloaded at all right now, so the mod should not first run its
        // own teardown only to be told the unload is refused. The typical veto comes
        // from lane, when a stack frame is sitting inside a lane entry this mod
        // provides, most often the current call chain itself, where the provider's
        // entry triggered a command dispatch and that command unloads the provider.
        // FreeLibrary would then unmap a code section that is still executing.
        if (auto veto = spi::askUnloadVetoes(mod.get()))
        {
            return ll::makeStringError(
                "'" + std::string(name) + "' cannot be unloaded now, vetoed by "
                + std::string(veto->who) + ": " + veto->reason
            );
        }

        // The general veto. A callback of this mod is executing, either on the
        // current call chain, where a callback issued
        // execute_command("pier unload <self>"), or on another thread dispatching a
        // bus or packet callback of it. FreeLibrary would unmap the executing code
        // section in both cases. The busy flag in lane covers lanes only, while this
        // counter covers every dispatch site in the host.
        if (int const depth = mod->inCallback.load(std::memory_order_acquire); depth > 0)
        {
            return ll::makeStringError(
                "'" + std::string(name) + "' cannot be unloaded now, " + std::to_string(depth)
                + " of its callbacks are executing, either unloading itself from inside "
                  "a callback or another thread dispatching one; retry once they return"
            );
        }

        if (mod->vtable.on_unload && !mod->vtable.on_unload(mod->vtable.instance))
        {
            return ll::makeStringError("'" + std::string(name) + "' refused to unload, on_unload returned false");
        }
        mod->commandsMuted = true;

        // `listeners.clear()` drops only the host's own shared_ptr. The EventStorage
        // of EventBus holds a strong reference in an OrderedSet<ListenerPtr>, so
        // clearing this vector removes no listener at all. They stay on the bus along
        // with callbacks into a dylib that is about to be unmapped, which is why
        // removeListener is called explicitly.
        for (auto const& l : mod->listeners)
        {
            if (!ll::event::EventBus::getInstance().removeListener(l.listener))
            {
                mod->getLogger().error(
                    "[host] unloading '{}' could not remove listener {}; it may remain "
                    "on the event bus with a callback into a dylib about to be unloaded",
                    name,
                    l.id
                );
            }
        }
        mod->listeners.clear();

        // Each capability package clears everything it holds for this mod, in
        // ascending stage order (contract §1 rule 2; the invariant behind the order
        // is documented in spi.h).
        spi::runTeardown(mod.get());

        if (auto const e = mod->lib.free(); e)
        {
            return ll::makeExceptionError(std::make_exception_ptr(*e));
        }
        eraseMod(name);
        return {};
    }

    /*  Runtime mod control  */

    ll::Expected<> ModHost::controlLoad(Manifest manifest)
    {
        std::string const name = manifest.name;
        if (auto e = load(std::move(manifest)); !e)
        {
            return e;
        }
        // The LeviLamina flow is load then enable, and ModManager::load only brings
        // the dylib up and runs pier_main. Without this step the mod stays loaded but
        // disabled, with its commands muted and on_enable never delivered.
        if (auto e = enable(name); !e)
        {
            // Roll back instead of leaving a half-live mod behind. One that is
            // loaded but never enabled still holds its dylib and its listeners.
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
            return ll::makeStringError("'" + std::string(name) + "' is not loaded");
        }
        // disable() runs first so that on_disable is actually delivered. A direct
        // unload() would fire on_unload only and the mod would never see its own
        // disable phase.
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
        return mod->lib.handle(); // HandleT is void*, only a qualifier conversion
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

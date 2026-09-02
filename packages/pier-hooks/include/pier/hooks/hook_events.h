/** pier/hooks/hook_events.h: the synthetic event registry. Event ids backed by native detours and
 * subscribed by name through the ordinary subscribe_event ABI. One concern per TU, each self-
 * registering, so adding a synthetic event touches neither this header, nor Events.cpp, nor any
 * table. The shared lifetime rules every hook file follows: - A detour installs lazily, on the
 * first subscriber or the first control call, and is never unpatched, because an unsubscribe can
 * arrive from inside the hooked function where unpatching is unsafe. An idle hook routes back to
 * origin on one subs-empty or not-armed test. - Everything runs on the server thread, so the
 * registry needs no lock. This package registers spi::EventProvider{name="hooks",
 * covers_registry=false}. The false says these are purely synthetic events, so an id with the
 * same suffix appearing in the registry means upstream introduced a real event whose name
 * collides and resolution must warn. A claim goes through spi::idMatches, an exact name or a
 * unique separated suffix, and never a substring: find(name) != npos would also match
 * "xxPlayerAttackEventxx". / */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "sdk/abi.h"

class Player;

namespace pier
{
    class HostedMod;
}

namespace pier::hooks
{
    /**
     * Assembles the player identity into the _player subobject consumers expect, of the
     * form "_player":{"name":...,"xuid":...,"uuid":...}, without a leading comma.
     *
     * The single source, which forces the same shape on every synthetic event. A copy
     * per TU would drift: once one of them renames a field or drops an escape, only that
     * one event fails to resolve a player and the symptom shows nothing about the cause.
     */
    [[nodiscard]] std::string playerRefSnbt(::Player const& p);

    struct HookSub
    {
        HostedMod* mod;
        PierEventCb cb;
        void* user;
        /** Shares the single id source spi::nextListenerId() with the host's dynamic
         *  event path. A subscription cannot use an address as its identity, because an
         *  entry is freed and reallocated and a recycled address turns a stale ticket
         *  into a key that unsubscribes someone else. One shared source means the two
         *  paths never issue the same handle. */
        std::uint64_t id;
        /** The lower value dispatches first, in the same order as the LL priority
         *  mapping on the host side. */
        int32_t priority;
    };

    struct HookEventDef
    {
        std::string_view name;
        /** Installs the native detours, called once when the first subscriber appears.
         *  Returns whether every primary hook point installed. On failure the
         *  subscription is refused rather than answered with a handle that never
         *  fires. */
        bool (*install)();
        bool installed = false;
        std::vector<std::unique_ptr<HookSub>> subs;

        /** The fast-routing test inside a hook body. */
        bool live() const { return !subs.empty(); }
    };

    /**
     * Self-registration. Each hook TU keeps its HookEventDef as a static and records it
     * through a file-level registrar object. It is consumed only at runtime, when a
     * subscription happens, so static initialization order across TUs does not matter.
     */
    struct HookEventRegistrar
    {
        explicit HookEventRegistrar(HookEventDef& def);
    };

    /**
     * Delivers one SNBT payload to every subscriber of def.
     *
     * Snapshot safe: a callback may subscribe or unsubscribe mid-dispatch, a callback
     * that unsubscribes itself still receives the current event, and one that joins
     * mid-dispatch starts from the next. A synthetic event only observes, so the
     * write-back sink is a no-op. An exception from a callback is caught and printed on
     * the spot and never unwinds back into the hooked engine function, and a silent
     * catch would make the bug invisible forever.
     */
    void dispatchHookEvent(HookEventDef& def, std::string const& snbt);

    /**
     * As dispatchHookEvent, with a live write-back sink: it returns true as soon as any
     * subscriber answers with SNBT carrying a true cancelled field.
     *
     * Only for hook points where origin really can be skipped. It must not be used where
     * skipping would leave the engine half updated. Remaining subscribers are still
     * called after a cancel, so the behavior does not depend on registration order.
     */
    bool dispatchHookEventCancellable(HookEventDef& def, std::string const& snbt);
} // namespace pier::hooks

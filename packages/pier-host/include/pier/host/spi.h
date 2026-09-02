#pragma once
// The host SPI, the only collaboration surface between capability packages and the
// host (contract §1 rule 2).
//
// The direction is always the same. A capability package registers into the host and
// the host calls back at the right moment. The host includes and links no symbol of
// any capability package, and the packages do not know each other. When a package is
// absent its registration never happens and the matching slots stay NULL, which is
// the whole implementation of "optional".
//
// Registration happens in the constructors of file-level static objects, which are
// guaranteed to exist because every package uses set_kind("object") (contract §1
// rule 4). The registries themselves are Meyers singletons, so static initialization
// order does not matter. Every callback fires on the server thread.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "sdk/abi.h"

namespace ll::io
{
    class Logger;
}

namespace pier
{
    class HostedMod;
}

class BlockSource;

namespace pier::spi
{
    /*  1. Slot packs, which fill function pointers into PierApi at load time
     * Each capability package registers one or more fill functions and the host runs
     * them together inside load(), before any mod is loaded. A fill function may
     * write only the slots of its own domain. The four header scalars are filled by
     * the host. */
    struct SlotPack
    {
        std::string_view name;      // Appears in the debug line listing slot packs.
        void (*fill)(PierApi& api);
    };
    void addSlotPack(SlotPack pack);

    /** For static registration: `static spi::SlotPackReg reg{{"core", &fillCore}};` */
    struct SlotPackReg
    {
        explicit SlotPackReg(SlotPack p) { addSlotPack(p); }
    };

    /*  2. Bootstrap steps, run in ascending stage order after the table is filled
     *     and before any mod is loaded
     * For packages that must do work as soon as the host is up, such as dimensions
     * reading its config and installing hooks, or hooks warming up the engine. They
     * live in the SPI and not in package static constructors because the work needs
     * the LL environment to be ready, including the logger and the config directory,
     * and static construction is too early for that. */
    struct Bootstrap
    {
        int stage;                  // Ascending execution order. Ties are unordered.
        std::string_view name;
        void (*run)();
    };
    void addBootstrap(Bootstrap step);

    struct BootstrapReg
    {
        explicit BootstrapReg(Bootstrap b) { addBootstrap(b); }
    };

    /*  3. Unload vetoes, asked one by one before an unload
     * Returning nullptr allows the unload. Returning a reason string of static
     * storage duration vetoes it. lane uses this to block an unload while another
     * mod still holds a lane of this provider. A future package with a similar
     * invariant registers here and the host needs no change. */
    struct UnloadVeto
    {
        std::string_view name;
        char const* (*why)(HostedMod* mod);
    };
    void addUnloadVeto(UnloadVeto veto);

    struct UnloadVetoReg
    {
        explicit UnloadVetoReg(UnloadVeto v) { addUnloadVeto(v); }
    };

    /*  4. Teardown steps, releasing resources in ascending stage order on mod death
     * A mod dies on an explicit unload, on rollback from a failed load and on server
     * shutdown. All three paths run these steps, so a package holding mod resources
     * writes its cleanup once.
     *
     * Stage assignment, spaced to leave room for later insertions:
     *   10 scheduled tasks  20 bus  30 services  40 lanes  50 command callbacks
     *   60 form tickets  70 KvDb  80 synthetic events  90 packet hooks
     *  100 economy callbacks  110 client resources
     * The invariant behind the order is to first stop what can re-enter mod code,
     * meaning tasks and events, then withdraw registrations others may still hold,
     * meaning services and lanes, and clear plain data last. */
    struct Teardown
    {
        int stage;
        std::string_view name;
        void (*run)(HostedMod* mod);
    };
    void addTeardown(Teardown step);

    struct TeardownReg
    {
        explicit TeardownReg(Teardown t) { addTeardown(t); }
    };

    /*  5. Event providers, splicing synthetic events into subscribe_event resolution
     * Neither hooks, which are synthesized by native detours, nor command events
     * appear in the LL dynamic event registry. They attach as providers and Events in
     * pier-api resolves them in the order given by contract §6. A claim decision goes
     * through idMatches, which is the only matcher. */
    struct EventProvider
    {
        std::string_view name;

        /** Whether the id of this provider corresponds to an entry of the same name
         *  in the registry.
         *
         *  true covers cases such as command events. The registry holds an emitter
         *  entry for the same event, but LL dispatches only to typed listeners and
         *  the dynamic path never receives it. A provider replacing the registry path
         *  repairs that rather than shadowing it, so resolution does not warn.
         *  false covers purely synthetic events such as hooks. An id with the same
         *  suffix appearing in the registry means upstream introduced a real event
         *  whose name collides, and that must be warned about (contract §6, shadowing
         *  is always visible). */
        bool covers_registry;

        /** Whether `wanted` belongs to this provider, by exact name or by a suffix
         *  preceded by a separator. */
        bool (*claims)(std::string_view wanted);

        /** Subscribes after claiming. Returns NULL on failure. The caller, Events,
         *  reports the error and does not fall through to another resolution path.
         *  Claiming means owning the outcome (contract §6). */
        PierListenerHandle (*subscribe)(
            HostedMod* mod, std::string_view wanted, int32_t priority, PierEventCb cb, void* user);

        /** True when the handle was found and unsubscribed. False when the handle
         *  does not belong to this provider, so the next one may try. */
        bool (*unsubscribe)(HostedMod* mod, PierListenerHandle handle);

        /** Mod death. Drops every subscription held under that mod. This is the
         *  provider side of removing listeners the host cannot reach. */
        void (*dropMod)(HostedMod* mod);

        /** Feeds every event id of this provider to the sink one at a time. Used by
         *  /pier events and by error hints. */
        void (*list)(void* ctx, PierStrSink sink);
    };
    void addEventProvider(EventProvider provider);

    struct EventProviderReg
    {
        explicit EventProviderReg(EventProvider p) { addEventProvider(p); }
    };

    /*  6. Dimension bridge, the single-slot extension point of the dimensions package
     * The world functions in api need two things only dimensions knows, the name of a
     * custom id and how to force a custom dimension that has not been built yet. It
     * is a single slot and not a list. Two dimension ledgers existing at once is
     * itself an error. It is empty when the package is absent, and api then degrades
     * to recognizing vanilla dimensions only and warns once per function. Diagnostic
     * detail such as the registration ledger and config drift is logged by the
     * implementation on its own failure paths, which is the only side that knows what
     * the ledger looks like. */
    struct DimensionBridge
    {
        /** The dimension name accepted by `/execute in`. An empty string when the
         *  dimension is not registered, with a diagnostic logged by this side. */
        std::string (*selectorNameOf)(int32_t dim);
        /** Forces a custom dimension into existence and takes its BlockSource.
         *  Returns nullptr on failure, with a diagnostic logged by this side.
         *
         *  The implementation must check that the engine instance it built carries
         *  the requested dim as its id, and must report an error and return nullptr
         *  when they differ. Once the ledger id and the engine id drift apart, block
         *  writes land silently in the wrong dimension, and teleporting a player into
         *  dim makes the engine throw an uncaught exception on a chunk worker thread
         *  and fastfail the process with 0xC0000409, which no failure return from the
         *  caller can contain. The knowledge that check needs exists only in the
         *  dimensions package, so the gate belongs on the implementation side. The
         *  api side treats a non-null blockSourceOf as its only condition to go
         *  ahead. */
        ::BlockSource* (*blockSourceOf)(int32_t dim);
    };
    void setDimensionBridge(DimensionBridge const* bridge);
    [[nodiscard]] DimensionBridge const* dimensionBridge() noexcept;

    /*  Host consumption surface, used by pier-host and by Events in pier-api */

    /** Fills the table header and runs every slot pack in registration order, and
     *  logs the pack names at debug level. Called once from load(), after which the
     *  table is frozen. */
    void buildApi(PierApi& api, ll::io::Logger& log);

    /** Runs every teardown step in ascending stage order. */
    void runTeardown(HostedMod* mod);

    /** Asks each veto in turn. Returns {package, reason} for the first refusal and
     *  an empty optional when all of them allow the unload. */
    struct VetoAnswer
    {
        std::string_view who;
        char const* reason;
    };
    std::optional<VetoAnswer> askUnloadVetoes(HostedMod* mod);

    /** Runs every bootstrap step in ascending stage order, from load() after
     *  buildApi. */
    void runBootstrap(ll::io::Logger& log);

    /** Iterates event providers. Returning true from visit means the entry was
     *  handled and iteration stops. */
    bool forEachEventProvider(bool (*visit)(EventProvider const&, void* ctx), void* ctx);

    /*  Id matching (contract §6, the only such decision in the repository) */

    /** True when wanted equals canonical exactly, or ends with a separator followed
     *  by canonical, where the separator is one of "::", ":" or ".". Never a
     *  substring match. A substring match would let a future upstream event sharing
     *  the same stem silently hijack the subscription. */
    [[nodiscard]] bool idMatches(std::string_view wanted, std::string_view canonical) noexcept;

    /*  Id space for listener handles, process-wide monotonic and never reused
     * It lives in the SPI because three consumers issue handles, Events, the hooks
     * provider and the command event provider, and none of them includes another.
     * There must be exactly one id space, otherwise two of them issuing the same
     * "unique" id is only a matter of time. */
    [[nodiscard]] std::uint64_t nextListenerId() noexcept;

    [[nodiscard]] inline PierListenerHandle handleOf(std::uint64_t id) noexcept
    {
        return reinterpret_cast<PierListenerHandle>(static_cast<std::uintptr_t>(id));
    }

    [[nodiscard]] inline std::uint64_t idOf(PierListenerHandle h) noexcept
    {
        return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(h));
    }
} // namespace pier::spi

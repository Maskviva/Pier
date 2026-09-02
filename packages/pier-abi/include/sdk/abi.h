/**
 * Pier ABI — sdk/abi.h (ABI v1)
 *
 * This header is the product: the sole contract between the C++ host
 * (pier-host plus the capability packages) and an SDK written in any language.
 * The reference mirror is packages/pier-sys-rs/src/api.rs, hand-written with no
 * bindgen, which doubles as readable annotation for this file.
 *
 * This file must parse as C. Consumers are "any language", so it uses C11 only:
 * no std::string_view, no enum class, no nested types. The C++ convenience
 * wrappers (PierStr to string_view and back) live in pier-support, not here; a
 * language-specific type in the contract forces every other language to guess
 * that type's layout. CI compiles this file once as C11 and once as C++20.
 *
 * Rules for changing this file. These are the only versioning rules anywhere.
 *   1. Append at the end of PierApi only. Never reorder, remove, or change the
 *      signature of an existing slot. Appending does NOT bump PIER_ABI_VERSION.
 *   2. After appending, update every SDK mirror slot for slot; the
 *      sys-mirrors-abi check enforces the ordering.
 *   3. Only a non-append change (reorder, removal, signature change) advances
 *      PIER_ABI_VERSION and PIER_ABI_MIN_SUPPORTED, both to the same number.
 *
 * Appending does not bump the version because the version answers "which
 * already-compiled mods still load". An appended slot invalidates no old mod:
 * the old table is a byte-identical prefix of the new one, and an old mod can
 * never reach the new slot. Bumping for it would announce an incompatibility
 * that does not exist. Each direction has its own gate instead: a new host with
 * an old mod is covered by the version range (see PIER_ABI_MIN_SUPPORTED); an
 * old host with a new mod is covered by the mod comparing struct_size slot by
 * slot, reporting "host lacks this capability" for the one call that overruns.
 *
 * The layout is identical across all build targets. PierApi carries no
 * conditional compilation: slots for client-only and dimension capabilities are
 * always present in the layout and are simply NULL when that package was not
 * built into the host. "Capability present" means "slot is non-NULL", and the
 * SDK reports "host does not provide X" from that. This buys three things:
 * mirrors need no conditional compilation, a cross-target mismatch cannot call
 * the wrong slot, and the struct has exactly one append point, the end.
 *
 * Conventions for the whole file; per-slot comments record only the exceptions.
 *   - Strings are UTF-8 (ptr, len) views and are NOT guaranteed NUL-terminated.
 *   - A string passed into a callback is owned by the caller and valid only for
 *     that call; copy it to keep it.
 *   - A mod hands strings out through a sink callback within the current call
 *     frame. Ownership never crosses the boundary: this ABI has no "returns a
 *     pointer the other side must free".
 *   - Threading: unless a slot says otherwise, call only on the server thread.
 *     log, gaming_status, schedule and schedule_after are thread-safe. Every
 *     callback (event, command, scheduled task) fires on the server thread.
 */
#ifndef PIER_SDK_ABI_H
#define PIER_SDK_ABI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** See "Rules for changing this file" in the file header. Appending a slot does
 *  not touch this. */
#define PIER_ABI_VERSION 1u

/** Oldest mod ABI the host accepts. Moves only on a non-append change, and then
 *  to the same number as PIER_ABI_VERSION. It is the switch for "a table older
 *  than this is no longer a prefix of mine". */
#define PIER_ABI_MIN_SUPPORTED 1u

/** The only entry symbol a mod must export. The host looks for this name alone
 *  and refuses to load with an explicit error if it is missing; there is no
 *  fallback and no historical alias. */
#define PIER_MAIN_SYMBOL "pier_main"

/** Bits for PierApi.host_flags and PierModVTable.mod_flags. Bit 0 must match on
 *  both sides or the host refuses to load and says why: a server host cannot
 *  load a client-built mod, and vice versa. All other bits are reserved and must
 *  currently be 0. */
#define PIER_FLAG_CLIENT 0x1u

/**
 * UTF-8 string view. An explicit {pointer, length} struct, not an alias for any
 * language's string type: the layout is defined by this declaration alone and
 * does not depend on either side's standard library. The zero-copy conversion
 * to and from std::string_view lives in pier-support.
 */
typedef struct PierStr
{
    const char* ptr;
    size_t len;
} PierStr;

/** Opaque handle to the HostedMod instance managed by the loader. */
typedef void* PierModHandle;
/** Opaque handle to an event listener. */
typedef void* PierListenerHandle;

/** Generic "run this" callback. */
typedef void (*PierTaskCb)(void* user);

/** Generic string sink: receives a string within the current call frame. */
typedef void (*PierStrSink)(void* ctx, PierStr s);

/**
 * Event callback.
 *   event_id : the full event id this listener fired for.
 *   snbt     : event data serialized as SNBT (CompoundTag). For cancellable
 *              events it contains a `cancelled` byte field.
 *   write_ctx / write_back : to mutate the event (e.g. cancel it, edit the
 *              chat message), call write_back(write_ctx, new_snbt) with the
 *              modified SNBT before returning. The loader deserializes it
 *              back into the event. Calling it zero times leaves the event
 *              untouched; the last call wins.
 */
typedef void (*PierEventCb)(
    void* user,
    PierStr event_id,
    PierStr snbt,
    void* write_ctx,
    PierStrSink write_back
);

/**
 * Custom command callback.
 *   args        : raw text following the command name (may be empty).
 *   origin_name : display name of the command origin (player name / "Server").
 *   out_success / out_error : call any number of times to emit output lines.
 */
typedef void (*PierCommandCb)(
    void* user,
    PierStr args,
    PierStr origin_name,
    void* out_ctx,
    PierStrSink out_success,
    PierStrSink out_error
);

/** Output sink for execute_command: full command output + success flag. */
typedef void (*PierCmdOutputSink)(void* ctx, bool success, PierStr output);

/*  World reads (scan)  */

/** A player's feet position + dimension. `found` is false if no such player. */
typedef struct PierPlayerPos
{
    double x;
    double y;
    double z;
    int32_t dimension;
    bool found;
} PierPlayerPos;

/**
 * Block sink: invoked once per cell during scan_region.
 *   x, y, z : the cell's world coordinates.
 *   name    : block type name, e.g. "minecraft:redstone_wire".
 *   snbt    : full block serialization (name + states + version) as SNBT.
 */
typedef void (*PierBlockSink)(void* ctx, int32_t x, int32_t y, int32_t z, PierStr name, PierStr snbt);

/**
 * Entity sink: invoked once per entity whose position falls inside the region.
 *   x, y, z : the block cell that contains the entity (floor of its position).
 *   type    : entity type name, e.g. "minecraft:creeper".
 *   snbt    : the entity's serialized NBT (Actor::save) as SNBT.
 */
typedef void (*PierEntitySink)(void* ctx, int32_t x, int32_t y, int32_t z, PierStr type, PierStr snbt);


/*  Per-domain payload types  */

/**
 * Player selector — the identifier half of the "handles are identifiers,
 * not pointers" rule. Resolved against the live player list on every call.
 *   kind: 0 = name (getRealName, falling back to getNameTag),
 *         1 = xuid, 2 = uuid (canonical string form).
 */
typedef struct PierPlayerSel
{
    int32_t kind;
    PierStr value;
} PierPlayerSel;

/** ActorUniqueID raw value. 0 / negative-invalid never resolves. */
typedef int64_t PierActorId;

/**
 * Container reference — "owner + which container".
 *   which: 0=inventory 1=ender_chest 2=armor 3=offhand 4=block container.
 *   player: valid for which 0..3.   dim/x/y/z: valid for which == 4.
 */
typedef struct PierContainerRef
{
    int32_t which;
    PierPlayerSel player;
    int32_t dim;
    int32_t x;
    int32_t y;
    int32_t z;
} PierContainerRef;

/** Raw byte sink (binary NBT). Bytes valid only within the call frame. */
typedef void (*PierBytesSink)(void* ctx, uint8_t const* data, size_t len);
/** Key/value sink (kvdb_iter). Views valid only within the call frame. */
typedef void (*PierKvSink)(void* ctx, PierStr key, PierStr value);
/** Actor sink (list_actors). */
typedef void (*PierActorSink)(void* ctx, PierActorId id, PierStr type_name);
/**
 * Form result callback. Invoked ONCE on the server thread when the player
 * responds (or the form is cancelled). result_snbt:
 *   cancelled       : {cancelled:1b, reason:N}
 *   SimpleForm      : {button:N}
 *   CustomForm      : {values:{<name>: string|double|int64…}}
 *   ModalForm       : {button:"upper"|"lower"}
 * Muted (never called) if the mod is disabled before the player responds.
 */
typedef void (*PierFormResultCb)(void* user, PierStr result_snbt);

/** Opaque handle to an open key-value database owned by the loader. */
typedef void* PierKvDbHandle;

/*  Cross-mod event bus FFI types
 * A mod cannot hand another mod a function pointer: `ModHost::unload`
 * calls FreeLibrary, so the publisher would be left holding a pointer into an
 * unmapped dylib. The loader therefore owns the subscription table, with the
 * same weak_ptr + ticket discipline as Forms.cpp and the mod-scoped scheduler.
 *
 * The loader never parses `payload` — it is opaque UTF-8 (JSON, SNBT, or
 * anything else the two mods agree on). Keeping the loader format-agnostic is
 * deliberate: the alternative is a schema that every publisher has to satisfy
 * and that the loader has to version.
 *
 * Topics are plain strings; namespace them (`plot:enter`, not `enter`).
 */

/**
 * Subscriber callback. `topic` and `payload` are borrowed for the duration of
 * the call — copy anything you keep.
 *
 * The return value is a veto, and only for `bus_publish_vetoable`:
 *   true  = "refuse this",
 *   false = "no opinion".
 * It is ignored entirely by `bus_publish`. There is deliberately no way to
 * turn a refusal back into an approval: a subscriber can only tighten, never
 * loosen. Letting one mod override another's refusal means the *last*
 * subscriber to run decides, and subscriber order is not something either mod
 * controls.
 *
 * Called on the thread that published. Never called after the owning mod is
 * unloaded or while it is disabled.
 */
typedef bool (*PierBusCb)(void* user, PierStr topic, PierStr payload);

/**
 * Provider callback for the cross-mod service registry (query-style calls,
 * as opposed to the bus's one-way broadcast).
 *
 * Write the answer through `reply(ctx, ...)` — exactly once — and return true.
 * Return false to report failure; anything written first is handed to the
 * caller as the error text, which is what makes "no such plot" and "the
 * database is down" distinguishable at the call site.
 *
 * `request` and `reply` are opaque UTF-8 the two mods agree on out of band. The
 * loader never looks inside either.
 *
 * Runs synchronously on the CALLING thread, inside `service_call`. Never called
 * after the providing mod is unloaded. It IS still called while the provider is
 * merely disabled: `service_call` deliberately does not consult isEnabled()
 * (see Services.cpp) because LeviLamina enables mods only after every on_load
 * has run, and a service that is unreachable during that window breaks every
 * consumer that resolves it in its own on_load.
 */
typedef bool (*PierServiceCb)(
    void* user, PierStr name, PierStr request, void* ctx, PierStrSink reply);

/** service_call return codes. */
#define PIER_SERVICE_OK 0        /* provider ran and wrote a reply */
#define PIER_SERVICE_NOT_FOUND 1 /* nobody provides this name (or is disabled/unloaded) */
#define PIER_SERVICE_ERROR 2     /* provider returned false; reply holds its message */
#define PIER_SERVICE_REFUSED 3   /* bad name, self-call, or call-depth limit */

/*  Same-toolchain fast lane
 *
 * bus and service are both "(name, UTF-8 payload) -> UTF-8 payload". That shape
 * is the cross-language common denominator: a mod in any language can speak it.
 * The price is a serialization round trip per call, with all type information
 * lost inside the string.
 *
 * This lane serves one special case: both sides built by the same toolchain, so
 * the C-layout function tables in the two dynamic libraries are byte-identical
 * and pointers can be handed over directly.
 *
 * The loader owns the name -> lane table (exclusive, like service), validates
 * the fingerprint, issues and collects leases, and holds a liveness flag that it
 * clears the moment the provider goes away. It does not interpret a single byte
 * of data or vtable; both pointers are opaque to it, exactly like a bus payload.
 *
 * The loader has to be involved because ModHost::unload calls FreeLibrary. The
 * provider's memory can stay alive by reference counting, but its code section
 * is unmapped, so the consumer's function pointer becomes a use-after-free. The
 * crash then lands in the consumer, with nothing in the log pointing at the mod
 * that just left. Hence:
 *   1. alive points at one cell on the loader's own heap and is never freed
 *      (lanes number in the dozens, so this leaks a few dozen uint32). The
 *      loader writes 0 when the provider goes away, and the consumer reads the
 *      cell before each call: one plain atomic read, no FFI, no lock. That is
 *      what "fast" means here, the loader runs no code on the hot path.
 *   2. When the provider goes away, the loader calls release for every
 *      outstanding lease before FreeLibrary, so the provider frees its own
 *      objects inside its own dylib with its own allocator.
 *
 * Most native languages have no stable ABI. The same contract type compiled
 * twice into two cdylibs can end up with different field order when compiler
 * metadata differs, and that is silent memory corruption rather than a crash.
 * So the check is a fingerprint, not a version number: compiler version, target
 * triple, contract name and version, and the type identity, size and alignment
 * of the function table, all folded into one u64. Any difference yields a
 * different fingerprint, lane_acquire returns PIER_LANE_FINGERPRINT, and not a
 * single pointer is handed over.
 *
 * The failure mode is "slow" (the consumer falls back to the service channel),
 * never undefined behaviour. That property is the entire reason this lane is
 * allowed to exist.
 *
 * The host compares fingerprints for equality and never interprets them; it has
 * to be that way, or "add one more item to the fingerprint" would become an ABI
 * change.
 */

/** Lane protocol version. Kept separate from PIER_ABI_VERSION: the lane shape
 *  evolves independently, and a mismatch is handled differently (reject this one
 *  lane, not the whole mod). */
#define PIER_LANE_PROTOCOL 1u

/** lane_acquire return values. */
#define PIER_LANE_OK 0          /* acquired; out is filled in */
#define PIER_LANE_NOT_FOUND 1   /* nobody published this name (that mod is not installed) */
#define PIER_LANE_FINGERPRINT 2 /* published, but the fingerprint differs; degrade, hand over nothing */
#define PIER_LANE_REFUSED 3     /* bad name, self-acquire, provider disabled, or protocol mismatch */

/**
 * Reference-count hooks, executed inside the provider's own dylib.
 *
 * The loader calls them from lane_acquire and lane_release, and calls release
 * for every outstanding lease when the provider unloads or calls
 * lane_unpublish. Publishing itself does not hold a count: the loader never
 * calls release for the data handed to lane_publish, and the provider reclaims
 * that itself after unpublishing.
 *
 * These must not call back into the loader (any lane_* slot); that self-
 * deadlocks. A typical implementation is one atomic increment or decrement on
 * the provider's own refcount, touching no lock.
 */
typedef void (*PierLaneRefFn)(void* data);

/** How a provider describes a lane when publishing it. Every field is filled in
 *  by the provider; the loader only carries it. */
typedef struct PierLaneDesc
{
    /** sizeof(PierLaneDesc); same discipline as PierApi::struct_size. */
    uint32_t struct_size;
    /** Must equal PIER_LANE_PROTOCOL or publish is refused. */
    uint32_t protocol;
    /** Build fingerprint. 0 is reserved (it would mean "anyone may connect") and
 *  must not be used. */
    uint64_t fingerprint;
    /** The provider's state pointer, typically a raw pointer handed out by a
     *  reference-counted object. The loader does not interpret it. */
    void* data;
    /** C-layout function table. The loader neither interprets nor copies it; the
     *  provider must keep it alive until the lane is withdrawn (static storage,
     *  or an allocation deliberately never reclaimed). */
    void const* vtable;
    /** May be NULL, in which case leases are not counted and the alive flag is the
     *  only guard. */
    PierLaneRefFn retain;
    PierLaneRefFn release;
} PierLaneDesc;

/** What lane_acquire produces. */
typedef struct PierLaneRef
{
    /** sizeof(PierLaneRef), filled in by the caller before the call; the loader
     *  uses it to decide which trailing fields to write. The direction is the
     *  reverse of elsewhere because here the loader writes the caller's
     *  struct. */
    uint32_t struct_size;
    /** Used when returning the lease. 0 means nothing was acquired. */
    uint64_t lease;
    /** The provider's fingerprint. Filled in even on mismatch, for diagnostics: an
     *  operator needs to read "these two mods were built by different
     *  compilers", not the word "mismatch". */
    uint64_t fingerprint;
    void* data;
    void const* vtable;
    /**
     * Liveness flag owned by the loader; non-zero means the provider is still
     * there. The cell is never freed, so reading it after the provider unloads
     * is still legal, which is the entire reason it exists. Read it with acquire
     * before each call (the writer uses a release store; a relaxed load paired
     * with a release store does not synchronize-with).
     *
     * NULL when the fingerprint did not match.
     */
    uint32_t const* alive;
    /**
     * In-call counter, also owned by the loader and never freed. The consumer
     * increments it before entering a provider entry point and decrements after
     * returning.
     *
     * alive only rules out "the provider was already gone before the call"; it
     * does not close the window between the check and the call. Server-thread-
     * only calling rules out concurrent unload but not reentrant unload: a
     * provider entry point dispatches a command, that command unloads the
     * provider, and FreeLibrary happens underneath a stack frame still sitting
     * in provider code.
     *
     * The loader reads this counter first thing in unload and refuses to unload
     * with a reason when it is non-zero, rather than unloading and crashing.
     *
     * Appended field, guarded by struct_size: an older consumer's struct_size
     * does not reach here, the loader does not write it, and its behaviour is
     * unchanged.
     */
    uint32_t* busy;
} PierLaneRef;

/*  Packet interception FFI types
 * Used by packet_hook_register / packet_conn_hook_register. See the block
 * comment on those fields in PierApi for the full contract. */

/** PierPacketEvent::direction, and the bit positions used by dir_mask. */
#define PIER_PKT_INBOUND 0  /* client -> server */
#define PIER_PKT_OUTBOUND 1 /* server -> client */

#define PIER_PKT_MASK_INBOUND (1 << PIER_PKT_INBOUND)
#define PIER_PKT_MASK_OUTBOUND (1 << PIER_PKT_OUTBOUND)

/** PierPacketCb return value. Anything else is treated as PASS. */
#define PIER_PKT_PASS 0    /* forward unchanged; `replace` output ignored */
#define PIER_PKT_REPLACE 1 /* forward the body handed to `replace` */
#define PIER_PKT_DROP 2    /* swallow the packet entirely */

/**
 * One intercepted packet. Every pointer inside is borrowed and valid only for
 * the duration of the callback — copy anything you keep.
 */
typedef struct PierPacketEvent
{
    /** sizeof(PierPacketEvent) as the LOADER knows it. Check before reading
     *  trailing fields, same discipline as PierApi::struct_size. */
    uint32_t struct_size;
    /** PIER_PKT_INBOUND / PIER_PKT_OUTBOUND. */
    int32_t direction;
    /** NetworkIdentifier::getHash() — stable for the connection's lifetime and
     *  available before the player exists, which is exactly when a login-phase
     *  rewrite needs to key its state. */
    uint64_t conn_id;
    /** "host:port" (NetworkIdentifier::getIPAndPort). */
    PierStr address;
    /** MinecraftPacketIds value decoded from the header. */
    int32_t packet_id;
    uint8_t sender_sub_id;
    uint8_t target_sub_id;
    /** Packet body, header excluded. NULL only when body_len is 0. */
    uint8_t const* body;
    size_t body_len;
} PierPacketEvent;

/**
 * Mutable header fields, pre-filled from the event. Assignments here only take
 * effect when the callback returns PIER_PKT_REPLACE.
 */
typedef struct PierPacketEdit
{
    uint32_t struct_size;
    int32_t packet_id;
    uint8_t sender_sub_id;
    uint8_t target_sub_id;
} PierPacketEdit;

/** Drop via packet_hook_unregister / packet_conn_hook_unregister. */
typedef void* PierPacketHookHandle;

/**
 * Packet interceptor. To rewrite, call `replace(replace_ctx, bytes, len)` with
 * the NEW BODY (header excluded) and return PIER_PKT_REPLACE. Calling
 * `replace` more than once keeps the last body; returning REPLACE without ever
 * calling it means "empty body".
 */
typedef int32_t (*PierPacketCb)(
    void* user,
    PierPacketEvent const* ev,
    PierPacketEdit* edit,
    void* replace_ctx,
    PierBytesSink replace
);

/** Connection lifecycle: `opened` is true on accept, false on close. */
typedef void (*PierConnCb)(void* user, uint64_t conn_id, PierStr address, bool opened);

/*  FFI types for the client capability group. The type declarations are always
 * present (they take no layout); whether the capability is available is decided
 * by whether the client_* slots in PierApi are NULL.  */
/** Opaque handle to a registered key binding owned by the loader's
 * ll::input::KeyRegistry. Drop via client_unregister_key. */
typedef void* PierKeyHandle;

/** Key action: 0 = released (up), 1 = pressed (down).
 * Mirrors ll::event::KeyInputEvent::Action. */
typedef int32_t PierKeyAction;

/** Focus impact level: 0=Neutral 1=ActivateFocus 2=DeactivateFocus.
 * Mirrors ::FocusImpact. */
typedef int32_t PierFocusImpact;

/** Callback for key press/release events. Runs on the client thread.
 *  user   — pointer passed to client_register_key
 *  action — 0=released 1=pressed (see PierKeyAction)
 *  impact — current focus impact (see PierFocusImpact) */
typedef void (*PierKeyCb)(void* user, PierKeyAction action, PierFocusImpact impact);

/*  Property and action keys. APPEND-ONLY: never renumber or remove. Unknown
 * values make the call return false; a safe SDK layer maps that to an
 * "unsupported" error, which is the forward-compatibility negotiation.  */

/** player_get_num / player_set_num keys. (G)=get-only, (S)=settable. */
enum PierPlayerNumProp
{
    PIER_PPROP_GAME_TYPE = 0, /* (G) Player::getPlayerGameType; write via player_set_gamemode */
    PIER_PPROP_LEVEL = 1, /* (S) attribute Player::LEVEL() */
    PIER_PPROP_EXPERIENCE = 2, /* (S) attribute Player::EXPERIENCE() (progress 0..1) */
    PIER_PPROP_HUNGER = 3, /* (S) attribute Player::HUNGER() */
    PIER_PPROP_SATURATION = 4, /* (S) attribute Player::SATURATION() */
    PIER_PPROP_EXHAUSTION = 5, /* (S) attribute Player::EXHAUSTION() */
    PIER_PPROP_XP_NEEDED_NEXT_LEVEL = 6, /* (G) Player::getXpNeededForNextLevel */
    PIER_PPROP_LUCK = 7, /* (G) Player::getLuck */
    PIER_PPROP_SELECTED_SLOT = 8, /* (G) Player::getSelectedItemSlot; set via PIER_PACT_SET_SELECTED_SLOT */
    PIER_PPROP_IS_OPERATOR = 9, /* (G) Player::isOperator */
    PIER_PPROP_CAN_USE_OPERATOR_BLOCKS = 10, /* (G) Player::canUseOperatorBlocks */
    PIER_PPROP_IS_FLYING = 11, /* (G) Player::isFlying */
    PIER_PPROP_CAN_JUMP = 12, /* (G) Player::canJump */
    PIER_PPROP_IS_EMOTING = 13, /* (G) Player::isEmoting */
    PIER_PPROP_IS_IN_RAID = 14, /* (G) Player::isInRaid */
    PIER_PPROP_IS_HURT = 15, /* (G) Player::isHurt */
    PIER_PPROP_IS_SCOPING = 16, /* (G) Player::isScoping */
    PIER_PPROP_CAN_SLEEP = 17, /* (G) Player::canSleep */
    PIER_PPROP_HAS_RESPAWN_POSITION = 18, /* (G) Player::hasRespawnPosition */
    PIER_PPROP_CLIENT_SUB_ID = 19, /* (G) Player::getClientSubId */
    PIER_PPROP_CAN_USE_ABILITY = 20,
    /* (G) Player::canUseAbility; the ability index is passed through the
       player_action GET path, see PIER_PACT_CAN_USE_ABILITY */
    /*  Appended: player gap fill  */
    PIER_PPROP_DIRECTION = 21, /* (G) Player::getDirection (0=S,1=W,2=N,3=E) */
    PIER_PPROP_CHUNK_RADIUS = 22, /* (G) Player::getChunkRadius */
    PIER_PPROP_NETWORK_RTT = 23, /* (G) getNetworkStatus().mPing (ms) */
    PIER_PPROP_PLATFORM = 24, /* (G) Player::getPlatform */
    PIER_PPROP_ENCHANTMENT_SEED = 25, /* (G) Player::getEnchantmentSeed */
    PIER_PPROP_IS_USING_ITEM = 26, /* (G) Player::isUsingItem */
    PIER_PPROP_IS_BLOCKING = 27, /* (G) Player::isBlocking */
    PIER_PPROP_IS_GLIDING = 28, /* (G) Player::isGliding */
    PIER_PPROP_IS_SWIMMING = 29, /* (G) Player::isSwimming */
    PIER_PPROP_PERMISSION_LEVEL = 30, /* (G) Player::getPlayerPermissionLevel */
    PIER_PPROP_SCORE = 31, /* (G) Player::getScore */
    PIER_PPROP_FALL_DISTANCE = 32, /* (G) Actor::getFallDistance */
    PIER_PPROP_IS_DEAD = 33, /* (G) Actor::isDead */
    PIER_PPROP_HAS_DIED_BEFORE = 34, /* (G) Player::hasDiedBefore */
    PIER_PPROP_DIMENSION = 35, /* (G) Actor::getDimensionId */
};

/** player_get_str keys. */
enum PierPlayerStrProp
{
    PIER_PSTR_REAL_NAME = 0, /* Player::getRealName */
    PIER_PSTR_UUID = 1, /* Player::getUuid().asString() */
    PIER_PSTR_XUID = 2, /* Player::getXuid */
    PIER_PSTR_IP_AND_PORT = 3, /* Player::getIPAndPort */
    PIER_PSTR_LOCALE_CODE = 4, /* Player::getLocaleCode */
    PIER_PSTR_NAME_TAG = 5, /* Actor::getNameTag (display name) */
    /*  Appended  */
    PIER_PSTR_LAST_DEATH_POS = 6, /* SNBT {x,y,z} or "" if none */
    PIER_PSTR_LAST_DEATH_DIMENSION = 7, /* dimension id as string */
    PIER_PSTR_NETWORK_STATUS = 8, /* SNBT {ping,avg_ping,packet_loss,max_ping} */
    PIER_PSTR_PLATFORM_ONLINE_ID = 9, /* Player::getPlatformOnlineId */
};

/**
 * player_action verbs.  Args are (sarg, a, b, c); unused args are ignored.
 * `out` (when non-NULL) receives a result string where noted.
 */
enum PierPlayerAction
{
    PIER_PACT_SET_ABILITY = 0,
    /* a=AbilitiesIndex, b=0/1 (bool slots) or float (FlySpeed etc.).
       Restores PlayerPermissionLevel to its pre-write value afterwards: the
       engine's LayeredAbilities::setAbility is the "switch to custom
       permissions" path and pushes the player to Custom, and that level ships
       to the client inside UpdateAbilitiesPacket together with the ability
       layer. To change the level, use PIER_PACT_SET_PERMISSION_LEVEL. */
    PIER_PACT_CAN_USE_ABILITY = 1, /* a=AbilitiesIndex → out "0"/"1" Player::canUseAbility */
    PIER_PACT_SET_SELECTED_SLOT = 2, /* a=slot                          Player::setSelectedSlot */
    PIER_PACT_GIVE_ITEM = 3, /* sarg=item SNBT                  ItemStack::fromTag + Player::addAndRefresh */
    PIER_PACT_SET_SPAWN_POINT = 4, /* a,b,c=pos, sarg=dim id (any registered dim); native Player::setRespawnPosition */
    PIER_PACT_CLEAR_TITLE = 5, /* native SetTitlePacket(Clear) */
    PIER_PACT_SET_TITLE = 6, /* sarg=text, a=slot(0 title,1 subtitle,2 actionbar); native SetTitlePacket, text sent verbatim */
    /*  Appended  */
    PIER_PACT_ADD_EXPERIENCE = 7, /* a=xp                  Player::addExperience */
    PIER_PACT_ADD_LEVELS = 8, /* a=levels              Player::addLevels */
    PIER_PACT_START_COOLDOWN = 9, /* sarg=item name, a=ticks Player::startItemCooldown */
    PIER_PACT_START_RIDING = 10, /* a=vehicle ActorUniqueID (lower 64b) Player::startRiding */
    PIER_PACT_STOP_RIDING = 11, /*                       Player::stopRiding */
    PIER_PACT_ATTACK = 12, /* a=target ActorUniqueID (lower 64b) Player::attack */
    PIER_PACT_DROP = 13, /* sarg=item SNBT, a=random(0/1) Player::drop */
    PIER_PACT_INTERACT = 14, /* a=target ActorUniqueID        Player::interact */
    PIER_PACT_START_USING_ITEM = 15, /* sarg=item SNBT, a=duration    Player::startUsingItem */
    PIER_PACT_STOP_USING_ITEM = 16, /*                       Player::stopUsingItem */
    PIER_PACT_SET_CHUNK_RADIUS = 17, /* a=radius              Player::setChunkRadius */
    PIER_PACT_SET_ENCHANTMENT_SEED = 18, /* a=seed                Player::setEnchantmentSeed */
    PIER_PACT_REGISTER_TRACKED_BOSS = 19, /* a=boss ActorUniqueID  Player::registerTrackedBoss */
    PIER_PACT_UNREGISTER_TRACKED_BOSS = 20, /* a=boss ActorUniqueID Player::unRegisterTrackedBoss */
    PIER_PACT_PLAY_EMOTE = 21, /* sarg=piece id         Player::playEmote */
    PIER_PACT_RESEND_ALL_CHUNKS = 22, /*                       Player::resendAllChunks */
    PIER_PACT_OPEN_INVENTORY = 23, /*                       Player::openInventory */
    PIER_PACT_SIDEBAR_SET = 24, /* sarg="obj\ntitle\nline…"  per-player sidebar */
    PIER_PACT_SIDEBAR_CLEAR = 25, /* sarg=objective        RemoveObjectivePacket */
    PIER_PACT_SET_PERMISSION_LEVEL = 26,
    /* a=PlayerPermissionLevel (0 Visitor, 1 Member, 2 Operator, 3 Custom).
       LayeredAbilities::setPlayerPermissions plus UpdateAbilitiesPacket.
       The read side is PIER_PPROP_PERMISSION_LEVEL. */
};

/** actor_get_num / actor_set_num keys. (S)=settable via actor_set_num. */
enum PierActorNumProp
{
    PIER_APROP_POS_X = 0, /* (G) Actor::getPosition().x (feet: getFeetPos for players; POS_* uses getPosition) */
    PIER_APROP_POS_Y = 1, /* (G) */
    PIER_APROP_POS_Z = 2, /* (G) */
    PIER_APROP_ROT_PITCH = 3, /* (G) Actor::getRotation().x */
    PIER_APROP_ROT_YAW = 4, /* (G) Actor::getRotation().y */
    PIER_APROP_DIMENSION = 5, /* (G) Actor::getDimensionId */
    PIER_APROP_HEALTH = 6, /* (G) Actor::getHealth; heal/hurt via actions */
    PIER_APROP_MAX_HEALTH = 7, /* (G) Actor::getMaxHealth */
    PIER_APROP_IS_ALIVE = 8, /* (G) Actor::isAlive */
    PIER_APROP_IS_ON_GROUND = 9, /* (G) Actor::isOnGround */
    PIER_APROP_IS_IN_WATER = 10, /* (G) Actor::isInWater */
    PIER_APROP_IS_IN_LAVA = 11, /* (G) Actor::isInLava */
    PIER_APROP_IS_ON_FIRE = 12, /* (G) Actor::isOnFire */
    PIER_APROP_IS_INVISIBLE = 13, /* (G) Actor::isInvisible */
    PIER_APROP_IS_SNEAKING = 14, /* (G) Actor::isSneaking */
    PIER_APROP_IS_BABY = 15, /* (G) Actor::isBaby */
    PIER_APROP_IS_RIDING = 16, /* (G) Actor::isRiding */
    PIER_APROP_IS_TAME = 17, /* (G) Actor::isTame */
    PIER_APROP_SPEED = 18, /* (G) Actor::getSpeedInMetersPerSecond */
    /*  Appended: actor gap fill  */
    PIER_APROP_VIEW_X = 19, /* (G) Actor::getViewVector().x */
    PIER_APROP_VIEW_Y = 20, /* (G) Actor::getViewVector().y */
    PIER_APROP_VIEW_Z = 21, /* (G) Actor::getViewVector().z */
    PIER_APROP_VEL_X = 22, /* (G) Actor::getVelocity().x */
    PIER_APROP_VEL_Y = 23, /* (G) Actor::getVelocity().y */
    PIER_APROP_VEL_Z = 24, /* (G) Actor::getVelocity().z */
    PIER_APROP_HEAD_X = 25, /* (G) Actor::getHeadPos().x */
    PIER_APROP_HEAD_Y = 26, /* (G) Actor::getHeadPos().y */
    PIER_APROP_HEAD_Z = 27, /* (G) Actor::getHeadPos().z */
    PIER_APROP_FEET_X = 28, /* (G) Actor::getFeetPos().x */
    PIER_APROP_FEET_Y = 29, /* (G) Actor::getFeetPos().y */
    PIER_APROP_FEET_Z = 30, /* (G) Actor::getFeetPos().z */
    PIER_APROP_FALL_DISTANCE = 31, /* (G) Actor::getFallDistance */
    PIER_APROP_IS_PERSISTENT = 32, /* (G) Actor::isPersistent */
    PIER_APROP_IS_LEASHED = 33, /* (G) Actor::isLeashed */
    PIER_APROP_IS_INVULNERABLE = 34, /* (G) Actor::isInvulnerable */
    PIER_APROP_VARIANT = 35, /* (G) Actor::getVariant */
    PIER_APROP_MARK_VARIANT = 36, /* (G) Actor::getMarkVariant */
    PIER_APROP_SCALE = 37, /* (G) Actor::getScaleFactor */
    PIER_APROP_BRIGHTNESS = 38, /* (G) Actor::getBrightness */
    PIER_APROP_RADIUS = 39, /* (G) Actor::getRadius */
    PIER_APROP_HAS_TOTEM = 40, /* (G) Actor::hasTotemEquipped */
    PIER_APROP_IS_IN_RAIN = 41, /* (G) Actor::isInRain */
    PIER_APROP_IS_IN_SNOW = 42, /* (G) Actor::isInSnow */
    PIER_APROP_IS_IN_THUNDERSTORM = 43, /* (G) Actor::isInThunderstorm */
    PIER_APROP_IS_FROZEN = 44, /* (G) Actor::isFrozen */
    PIER_APROP_IS_IN_LOVE = 45, /* (G) Actor::isInLove */
    PIER_APROP_DEATH_TIME = 46, /* (G) Actor::getDeathTime */
    PIER_APROP_HAS_PASSENGER = 47, /* (G) Actor::hasPassenger */
};

/** actor_get_str keys. */
enum PierActorStrProp
{
    PIER_ASTR_TYPE_NAME = 0, /* Actor::getTypeName */
    PIER_ASTR_NAME_TAG = 1, /* Actor::getNameTag */
    /*  Appended  */
    PIER_ASTR_SCORE_TAG = 2, /* Actor::getScoreTag */
    PIER_ASTR_FILTERED_NAME = 3, /* Actor::getFilteredNameTag */
};

/** actor_action verbs. Args (sarg, a, b, c); `out` receives a result where noted. */
enum PierActorAction
{
    PIER_AACT_KILL = 0, /* Actor::kill */
    PIER_AACT_DESPAWN = 1, /* Actor::despawn */
    PIER_AACT_HEAL = 2, /* a=amount                            Actor::heal */
    PIER_AACT_SET_ON_FIRE = 3, /* a=seconds                           Actor::setOnFire */
    PIER_AACT_TELEPORT = 4, /* a,b,c=pos, sarg=dim ("0".."2")      Actor::teleport */
    PIER_AACT_SET_NAME_TAG = 5, /* sarg=name                           Actor::setNameTag */
    PIER_AACT_ADD_TAG = 6, /* sarg=tag → out "0"/"1"              Actor::addTag */
    PIER_AACT_REMOVE_TAG = 7, /* sarg=tag → out "0"/"1"              Actor::removeTag */
    PIER_AACT_HAS_TAG = 8, /* sarg=tag → out "0"/"1"              Actor::hasTag */
    PIER_AACT_ADD_EFFECT = 9, /* sarg=effect name, a=ticks, b=amplifier, c=visible(0/1)
                                         MobEffect::getByName + Actor::addEffect */
    PIER_AACT_REMOVE_EFFECT = 10, /* sarg=effect name                    Actor::removeEffect(id) */
    PIER_AACT_CLEAR_EFFECTS = 11, /* Actor::removeAllEffects */
    PIER_AACT_HURT = 12, /* a=damage (generic damage source)    Actor::hurt */
    PIER_AACT_ATTRIBUTE_GET = 13, /* sarg=attribute name ("minecraft:health"…) → out value */
    /*  Appended  */
    PIER_AACT_SET_VARIANT = 14, /* a=variant             Actor::setVariant */
    PIER_AACT_SET_MARK_VARIANT = 15, /* a=variant             Actor::setMarkVariant */
    PIER_AACT_SET_PERSISTENT = 16, /*                       Actor::setPersistent */
    PIER_AACT_SET_LEASH_HOLDER = 17, /* a=holder ActorUniqueID Actor::setLeashHolder */
    PIER_AACT_SET_INVISIBLE = 18, /* a=0/1                 Actor::setInvisible */
    PIER_AACT_SET_SNEAKING = 19, /* a=0/1                 Actor::setSneaking */
    PIER_AACT_SET_NAME_TAG_VISIBLE = 20, /* a=0/1                 Actor::setNameTagVisible */
    PIER_AACT_SET_TARGET = 21, /* a=target ActorUniqueID Actor::setTarget */
    PIER_AACT_SET_OWNER = 22, /* a=owner ActorUniqueID  Actor::setOwner */
    PIER_AACT_BURN = 23, /* a=damage              Actor::burn */
    PIER_AACT_STOP_FIRE = 24, /*                       Actor::extinguishFire */
    PIER_AACT_SET_VELOCITY = 25, /* a,b,c=vel             Actor::setVelocity */
    PIER_AACT_APPLY_IMPULSE = 26, /* a,b,c=impulse         Actor::applyImpulse */
    PIER_AACT_SET_SCORE_TAG = 27, /* sarg=text             Actor::setScoreTag */
    PIER_AACT_SET_SKIN_ID = 28, /* a=skin id             Actor::setSkinID */
    PIER_AACT_SET_STRENGTH = 29, /* a=strength            Actor::setStrength */
    PIER_AACT_REMOVE_ALL_PASSENGERS = 30, /*                       Actor::removeAllPassengers */
    PIER_AACT_EXECUTE_EVENT = 31, /* sarg=event name       Actor::executeEvent */
    PIER_AACT_SET_ROTATION = 32, /* a=pitch b=yaw         Actor::setRotationWrapped */
};

/** block_get_num keys. */
enum PierBlockNumProp
{
    PIER_BPROP_IS_AIR = 0, /* Block::isAir */
    PIER_BPROP_DATA = 1, /* Block::getData (legacy data value) */
    PIER_BPROP_BLOCK_ITEM_ID = 2, /* Block::getBlockItemId */
    PIER_BPROP_IS_CRAFTING_BLOCK = 3, /* Block::isCraftingBlock */
    PIER_BPROP_IS_INTERACTIVE_BLOCK = 4, /* Block::isInteractiveBlock */
    PIER_BPROP_HAS_BLOCK_ENTITY = 5, /* BlockSource::getBlockEntity(pos) != null */
    /*  Appended: block gap fill  */
    PIER_BPROP_LIGHT = 6, /* Block::getLight */
    PIER_BPROP_LIGHT_EMISSION = 7, /* Block::getLightEmission */
    PIER_BPROP_DESTROY_SPEED = 8, /* Block::getDestroySpeed */
    PIER_BPROP_EXPLOSION_RESISTANCE = 9, /* Block::getExplosionResistance */
    PIER_BPROP_FRICTION = 10, /* Block::getFriction */
    PIER_BPROP_IS_CONTAINER = 11, /* Block::isContainerBlock */
    PIER_BPROP_IS_DOOR = 12, /* Block::isDoorBlock */
    PIER_BPROP_IS_FENCE = 13, /* Block::isFenceBlock */
    PIER_BPROP_IS_RAIL = 14, /* Block::isRailBlock */
    PIER_BPROP_IS_SLAB = 15, /* Block::isSlabBlock */
    PIER_BPROP_IS_STAIR = 16, /* Block::isStairBlock */
    PIER_BPROP_IS_WALL = 17, /* Block::isWallBlock */
    PIER_BPROP_IS_CROP = 18, /* Block::isCropBlock */
    PIER_BPROP_IS_UNBREAKABLE = 19, /* Block::isUnbreakable */
    PIER_BPROP_REDSTONE_SIGNAL = 20, /* Block::getDirectSignal */
    PIER_BPROP_COMPARATOR_SIGNAL = 21, /* Block::getComparatorSignal */
    PIER_BPROP_IS_SIGNAL_SOURCE = 22, /* Block::isSignalSource */
    PIER_BPROP_VARIANT = 23, /* Block::getVariant */
    PIER_BPROP_BURN_ODDS = 24, /* Block::getBurnOdds */
    PIER_BPROP_FLAME_ODDS = 25, /* Block::getFlameOdds */
    PIER_BPROP_BOUNCINESS = 26, /* Block::getBounciness */
    PIER_BPROP_IS_SOLID = 27, /* Block::isSolid */
    PIER_BPROP_REQUIRES_TOOL = 28, /* Block::requiresCorrectToolForDrops */
};

/** block_get_str keys. */
enum PierBlockStrProp
{
    PIER_BSTR_TYPE_NAME = 0, /* Block::getTypeName */
    PIER_BSTR_SNBT = 1, /* Block::mSerializationId → SNBT {name,states,version} */
    PIER_BSTR_DESCRIPTION_ID = 2, /* Block::getDescriptionId */
    PIER_BSTR_DEBUG_STRING = 3, /* Block::toDebugString */
    PIER_BSTR_TAGS = 4, /* Block::mTags → SNBT string list ["a","b"] */
    /*  Appended  */
    PIER_BSTR_STATE = 5, /* SNBT {state_name:value,…} all block states */
    PIER_BSTR_COLLISION_SHAPE = 6, /* SNBT [{min:[x,y,z],max:[x,y,z]},…] */
    PIER_BSTR_OUTLINE_SHAPE = 7, /* SNBT [{min,max}] render outline */
    PIER_BSTR_DISPLAY_NAME = 8, /* Block::getDisplayName */
};

/** block_action verbs. */
enum PierBlockAction
{
    PIER_BACT_HAS_TAG = 0, /* sarg=tag → out "0"/"1"  Block::hasTag */
    /*  Appended  */
    PIER_BACT_GET_STATE = 1, /* sarg=state name → out value string  Block::getState */
    PIER_BACT_POP_RESOURCE = 2, /* sarg=item SNBT → pop resource at pos  Block::popResource */
    PIER_BACT_AS_ITEM = 3, /* → out item SNBT   Block::asItemInstance */
};

/** item_get_num keys (query a transient ItemStack rebuilt from SNBT). */
enum PierItemNumProp
{
    PIER_IPROP_COUNT = 0, /* ItemStackBase::mCount */
    PIER_IPROP_MAX_STACK_SIZE = 1, /* ItemStackBase::getMaxStackSize */
    PIER_IPROP_AUX_VALUE = 2, /* ItemStackBase::getAuxValue */
    PIER_IPROP_ID = 3, /* ItemStackBase::getId */
    PIER_IPROP_DAMAGE = 4, /* ItemStackBase::getDamageValue */
    PIER_IPROP_IS_NULL = 5, /* ItemStackBase::isNull */
    PIER_IPROP_IS_BLOCK = 6, /* ItemStackBase::isBlock */
    PIER_IPROP_IS_ENCHANTED = 7, /* ItemStackBase::isEnchanted */
    PIER_IPROP_IS_ARMOR = 8, /* ItemStackBase::isArmorItem */
    PIER_IPROP_IS_DAMAGEABLE = 9, /* ItemStackBase::isDamageableItem */
    PIER_IPROP_IS_DAMAGED = 10, /* ItemStackBase::isDamaged */
    /*  Appended: item gap fill  */
    PIER_IPROP_MAX_DAMAGE = 11, /* ItemStackBase::getMaxDamage */
    PIER_IPROP_IS_UNBREAKABLE = 12, /* ItemStackBase::isUnbreakable */
    PIER_IPROP_HAS_DURABILITY = 13, /* ItemStackBase::hasDurability */
    PIER_IPROP_IS_POTION = 14, /* ItemStackBase::isPotionItem */
    PIER_IPROP_IS_THROWABLE = 15, /* ItemStackBase::isThrowable */
    PIER_IPROP_IS_FIRE_RESISTANT = 16, /* ItemStackBase::isFireResistant */
    PIER_IPROP_ATTACK_DAMAGE = 17, /* ItemStackBase::getAttackDamage */
    PIER_IPROP_REPAIR_COST = 18, /* ItemStackBase::getBaseRepairCost */
    PIER_IPROP_ENCHANT_VALUE = 19, /* ItemStackBase::getEnchantValue */
    PIER_IPROP_IS_STACKABLE = 20, /* ItemStackBase::isStackable */
    PIER_IPROP_IS_MUSIC_DISC = 21, /* ItemStackBase::isMusicDiscItem */
    PIER_IPROP_IS_OFFHAND = 22, /* ItemStackBase::isOffhandItem */
    PIER_IPROP_USE_DURATION = 23, /* ItemStackBase::getMaxUseDuration */
    PIER_IPROP_IS_GLINT = 24, /* ItemStackBase::isGlint */
    PIER_IPROP_IS_BUNDLE = 25, /* ItemStackBase::isBundle */
    PIER_IPROP_HAS_USER_DATA = 26, /* ItemStackBase::hasUserData */
    PIER_IPROP_HAS_CUSTOM_NAME = 27, /* ItemStackBase::hasCustomHoverName */
};

/** item_get_str keys. */
enum PierItemStrProp
{
    PIER_ISTR_TYPE_NAME = 0, /* ItemStackBase::getTypeName ("minecraft:apple") */
    PIER_ISTR_NAME = 1, /* ItemStackBase::getName (display) */
    PIER_ISTR_CUSTOM_NAME = 2, /* ItemStackBase::getCustomName */
    PIER_ISTR_RAW_NAME_ID = 3, /* ItemStackBase::getRawNameId */
    /*  Appended  */
    PIER_ISTR_LORE = 4, /* SNBT list ["l1","l2"]  ItemStackBase::getCustomLore */
    PIER_ISTR_CAN_DESTROY = 5, /* SNBT list ["minecraft:stone",…] */
    PIER_ISTR_CAN_PLACE_ON = 6, /* SNBT list */
    PIER_ISTR_USER_DATA = 7, /* full NBT user data as SNBT */
    PIER_ISTR_HOVER_NAME = 8, /* ItemStackBase::getHoverName */
    PIER_ISTR_EFFECT_NAME = 9, /* ItemStackBase::getEffectName */
    PIER_ISTR_COLOR = 10, /* SNBT {r,g,b}  ItemStackBase::getColor */
};

/** item_transform ops: rebuild → mutate → serialize back (out = new SNBT). */
enum PierItemOp
{
    PIER_IOP_SET_CUSTOM_NAME = 0, /* sarg=name             ItemStackBase::setCustomName */
    PIER_IOP_SET_DAMAGE = 1, /* narg=damage           ItemStackBase::setDamageValue */
    PIER_IOP_SET_COUNT = 2, /* narg=count            ItemStackBase::mCount */
    PIER_IOP_SET_LORE = 3, /* sarg=SNBT list ["l1","l2"]  ItemStackBase::setCustomLore */
    /*  Appended  */
    PIER_IOP_SET_UNBREAKABLE = 4, /* narg=0/1               ItemStackBase::setUnbreakable */
    PIER_IOP_HURT_AND_BREAK = 5, /* narg=damage            ItemStackBase::hurtAndBreak */
    PIER_IOP_SET_REPAIR_COST = 6, /* narg=cost              ItemStackBase::setRepairCost */
    PIER_IOP_ADD_ENCHANT = 7, /* sarg="name:level"      saveEnchantsToUserData */
    PIER_IOP_REMOVE_ENCHANTS = 8, /*                        ItemStackBase::removeEnchants */
    PIER_IOP_CLEAR_LORE = 9, /*                        ItemStackBase::clearCustomLore */
    PIER_IOP_RESET_NAME = 10, /*                        ItemStackBase::resetHoverName */
    PIER_IOP_SET_CAN_DESTROY = 11, /* sarg=SNBT list         ItemStackBase::setCanDestroy */
    PIER_IOP_SET_CAN_PLACE_ON = 12, /* sarg=SNBT list         ItemStackBase::setCanPlaceOn */
};

/** scoreboard_op verbs (args a=objective/slot, b=target, n=value). */
enum PierScoreboardOp
{
    PIER_SB_ADD_OBJECTIVE = 0, /* a=name, b=display name → out "1"      Scoreboard::addObjective("dummy") */
    PIER_SB_REMOVE_OBJECTIVE = 1, /* a=name                                Scoreboard::removeObjective */
    PIER_SB_LIST_OBJECTIVES = 2, /* → out SNBT [{name,display},…]        Scoreboard::getObjectives */
    PIER_SB_GET_SCORE = 3, /* a=objective, b=fake-player name → out value  Objective::getPlayerScore */
    PIER_SB_SET_SCORE = 4, /* a=objective, b=name, n=value          Scoreboard::modifyPlayerScore(Set) */
    PIER_SB_ADD_SCORE = 5, /* a=objective, b=name, n=value         … (Add) */
    PIER_SB_REDUCE_SCORE = 6, /* a=objective, b=name, n=value         … (Subtract) */
    PIER_SB_RESET_SCORE = 7, /* a=objective, b=name                   Scoreboard::resetPlayerScore */
    PIER_SB_SET_DISPLAY = 8, /* a=slot("sidebar"/"list"/"belowname"), b=objective  setDisplayObjective */
    PIER_SB_CLEAR_DISPLAY = 9, /* a=slot                                clearDisplayObjective */
};

/** sys_info_str keys. */
/** Per-dimension behaviour rules for md_set_dimension_rule.
 *
 *  These are deliberately NOT a mirror of any engine enum: they name things
 *  the loader intercepts itself. Values are ABI — append only, never renumber.
 */
enum PierDimRule
{
    PIER_DIMRULE_SPAWN_MONSTER = 0, /* natural hostile spawns */
    PIER_DIMRULE_SPAWN_ANIMAL = 1, /* natural passive spawns */
    PIER_DIMRULE_SPAWN_SPAWNER = 2, /* spawns from mob spawners */
    PIER_DIMRULE_EXPLODE_BLOCKS = 3, /* explosions damaging terrain */
    PIER_DIMRULE_FIRE_SPREAD = 4, /* fire spreading to neighbours */
    PIER_DIMRULE_MOB_GRIEFING = 5, /* mobs changing blocks */
    PIER_DIMRULE_PROJECTILE = 6, /* projectile spawns */
    /*  Second batch; hook points follow LegacyScriptEngine's same-named events  */
    PIER_DIMRULE_PISTON_PUSH = 7, /* pistons moving blocks */
    PIER_DIMRULE_LIQUID_FLOW = 8, /* water/lava spreading */
    PIER_DIMRULE_FARMLAND_DECAY = 9, /* farmland trampled back to dirt */
    PIER_DIMRULE_RIDE = 10, /* mounting boats/minecarts/animals */
    /*  Plot-boundary confinement (needs md_set_plot_grid)  */
    /* Pistons moving blocks ACROSS a plot boundary. Distinct from
     * PIER_DIMRULE_PISTON_PUSH, which disables pistons for the whole
     * dimension: this one leaves them working inside a plot and only refuses
     * the push that would cross the edge. Both apply — either one denying is
     * enough to stop the push. Inert in dimensions with no registered grid. */
    PIER_DIMRULE_PISTON_CROSS_PLOT = 11,
    /* Entities crossing a plot boundary. Players and ridden vehicles are
     * never confined — see PlotConfine.cpp for why. */
    PIER_DIMRULE_ENTITY_CROSS_PLOT = 12,
};

enum PierSysInfoProp
{
    PIER_SYS_OS_NAME = 0, /* sys_utils::getSystemName */
    PIER_SYS_OS_VERSION = 1, /* sys_utils::getSystemVersion → string */
    PIER_SYS_LOCALE = 2, /* sys_utils::getSystemLocaleCode */
    PIER_SYS_LOCAL_TIME = 3, /* sys_utils::getLocalTime → SNBT {year,month,day,hour,minute,second,ms} */
};

/** server_info_str keys. */
enum PierServerInfoProp
{
    PIER_SRV_BDS_VERSION = 0, /* Common::getGameVersionString */
    PIER_SRV_PROTOCOL_VERSION = 1, /* SharedConstants::NetworkProtocolVersion → string */
};

/**
 * Function table handed to the mod at load time.
 * Pointer remains valid for the whole lifetime of the mod.
 */
/*
 * Before adding a slot, read the existing slot names.
 *
 * Duplicates have shipped twice: level_actors_in_box duplicated list_actors,
 * and actor_despawn / actor_set_health duplicated actor_action's AACT_DESPAWN
 * and AACT_HEAL, which the SDK already wrapped. Both were caught only at
 * compile time by a redefinition; had the names merely differed, the two
 * implementations would have coexisted until they drifted, leaving "why can I
 * remove it here but not there" with no answer.
 *
 * Check these three catch-all slots first, they cover a lot of ground:
 *
 *   actor_action     remove, heal, ignite, teleport, add effect (PIER_AACT_*)
 *   actor_get_num    position, health, rotation, misc numbers (PIER_APROP_*)
 *   list_actors      every actor in a dimension, with type names
 *
 * A stem-matching duplicate detector was tried and produced too much noise to
 * be usable (actor_get_* alone pairs up into a screenful), so this is prose
 * rather than a script.
 */
/** legacymoney event types, used by the server-side economy capability. */
typedef enum PierMoneyEvent
{
    PIER_MONEY_SET = 0,
    PIER_MONEY_ADD = 1,
    PIER_MONEY_REDUCE = 2,
    PIER_MONEY_TRANS = 3
} PierMoneyEvent;

/**
 * legacymoney event callback. Return false to veto the change; only the before
 * callback's return value is honoured, an after callback's is ignored.
 *
 * from and to are less obvious than their names suggest. This is LegacyMoney's
 * own shape, forwarded as-is rather than "corrected":
 *
 *   - PIER_MONEY_TRANS: from is the payer, to the payee, both non-empty.
 *   - PIER_MONEY_ADD / REDUCE / SET: from is always the empty string and to is
 *     the xuid being operated on, including for REDUCE, where the money is
 *     taken from to. To learn whose balance changed, always read to.
 *
 * value is the delta for ADD, REDUCE and TRANS, and the target balance (not a
 * delta) for SET.
 *
 * Every mod's callback is invoked; an earlier veto does not skip the rest, so
 * the outcome does not depend on registration order. The change is vetoed if
 * any callback returns false.
 */
typedef bool (*PierMoneyCb)(PierMoneyEvent type, PierStr from, PierStr to, int64_t value);

typedef struct PierApi
{
    /** sizeof(PierApi), filled in by the host from the table it compiled. This is
     *  the whole basis of forward compatibility: the SDK compares against it at
     *  every non-core slot's call site. */
    uint32_t struct_size;
    /** Equals the host's PIER_ABI_VERSION. */
    uint32_t abi_version;
    /** Bitwise OR of PIER_FLAG_*. Bit 0 means a client build. */
    uint32_t host_flags;
    /** Reserved, always 0. Rounds the header out to 16 bytes and leaves room for
     *  future header scalars. */
    uint32_t _reserved0;

    /**
     * Log a message through the mod's own LeviLamina logger.
     * level: -1=Off, 0=Fatal, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Trace
     * (mirrors ll::io::LogLevel). Thread-safe.
     */
    void (*log)(PierModHandle mod, int32_t level, PierStr msg);

    /**
     * Current gaming status: 0=Default, 1=Starting, 2=Running, 3=Stopping
     * (mirrors ll::GamingStatus). Thread-safe.
     */
    int32_t (*gaming_status)();

    /** Queue a task onto the server thread ASAP. Thread-safe. */
    void (*schedule)(PierTaskCb cb, void* user);

    /** Queue a task onto the server thread after `delay_ms`. Thread-safe. */
    void (*schedule_after)(PierTaskCb cb, void* user, uint64_t delay_ms);

    /**
     * Subscribe to a LeviLamina event by id (server thread only).
     *   event_id : full id, e.g. "ll::event::PlayerChatEvent". If no exact
     *              match exists, the loader falls back to a unique suffix
     *              match ("PlayerChatEvent" works if unambiguous).
     *   priority : 0..4 (Highest..Lowest), 2 = Normal
     *              (mirrors ll::event::EventPriority).
     * Returns NULL on failure (unknown/ambiguous id).
     */
    PierListenerHandle (*subscribe_event)(
        PierModHandle mod,
        PierStr event_id,
        int32_t priority,
        PierEventCb cb,
        void* user
    );

    /** Remove a listener previously returned by subscribe_event. Server thread only. */
    bool (*unsubscribe_event)(PierModHandle mod, PierListenerHandle listener);

    /** Enumerate all currently registered event ids. Server thread only. */
    void (*list_events)(void* ctx, PierStrSink sink);

    /**
     * Execute a command as the server console (permission: Owner) and collect
     * its output. Server thread only. Returns false if the level is not ready.
     */
    bool (*execute_command)(PierStr cmd, void* ctx, PierCmdOutputSink sink);

    /**
     * Register a custom command `/name [args: raw text]`.
     *   permission: 0=Any,1=GameDirectors,2=Admin,3=Host,4=Owner
     *               (mirrors CommandPermissionLevel).
     * Call during on_enable, on the server thread. The command stays
     * registered for the lifetime of the server (Bedrock cannot unregister
     * commands); callbacks for disabled mods are muted by the loader.
     */
    bool (*register_command)(
        PierModHandle mod,
        PierStr name,
        PierStr description,
        int32_t permission,
        PierCommandCb cb,
        void* user
    );

    /**
     * Current server tick (the tickID from Level::getCurrentTick()).
     * Returns 0 when the level is not ready. Server thread only.
     */
    uint64_t (*get_current_tick)();

    /**
     * Seconds taken by the last tick (mTickDeltaTime; 0.05 at 20 TPS).
     * TPS = 1.0 / tick_delta_time when > 0. Returns -1.0 if unavailable.
     * Server thread only.
     */
    double (*get_tick_delta_time)();

    /**
     * Number of currently connected players
     * (Level::getActivePlayerCount()). Server thread only.
     */
    int32_t (*get_player_count)();

    /**
     * Whether the simulation is currently paused
     * (Level::getSimPaused()). Server thread only.
     */
    bool (*get_sim_paused)();

    /*  Appended  */

    /**
     * Spawn a particle effect at a world coordinate. Used to outline a
     * selection box edge-by-edge. Server thread only. Returns false if the
     * level/dimension is not ready.
     *   dimension   : 0 = overworld, 1 = nether, 2 = the end.
     *   effect_name : e.g. "minecraft:basic_flame_particle" or
     *                 "minecraft:redstone_wire_dust_particle".
     */
    bool (*spawn_particle)(int32_t dimension, PierStr effect_name, double x, double y, double z);

    /**
     * Look up a connected player's feet position and dimension by name.
     * Used to pick selection corners from where the player is standing.
     * Server thread only.
     */
    PierPlayerPos (*get_player_position)(PierStr name);

    /**
     * Scan a cuboid region, corners inclusive (order-independent). For every
     * cell in the box, blocks_sink is called with the block name + full SNBT.
     * For every entity whose position lies within the box, entities_sink is
     * called with the containing cell and the entity's SNBT. Both sinks run
     * synchronously within this call; nothing is retained afterwards.
     * Server thread only. Returns false if the level/dimension is not ready.
     */
    bool (*scan_region)(
        int32_t dimension,
        int32_t x1,
        int32_t y1,
        int32_t z1,
        int32_t x2,
        int32_t y2,
        int32_t z2,
        void* ctx,
        PierBlockSink blocks_sink,
        PierEntitySink entities_sink
    );


    /*  Append-only region; never reorder.
     * Everything below: SERVER THREAD ONLY unless noted. All calls return
     * false / do nothing while the level is not ready. Unknown enum keys
     * return false (forward-compat negotiation).                        */

    /*  §A world read/write & clock  */

    /** Read one block: sink called once with (x,y,z, type name, full SNBT). */
    bool (*get_block)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierBlockSink sink);
    /** Place a block natively (BlockSource::setBlock, DEFAULT update flags).
     *  block_spec = "minecraft:stone" / "stone" (default state) or a full
     *  {name,states,...} SNBT. Unknown names fail instead of placing a placeholder. */
    bool (*set_block)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr block_spec);
    /** World time (Level::getTime). */
    bool (*get_time)(int64_t* out);
    /** Set world time natively (Level::setTime). */
    bool (*set_time)(int64_t t);
    /** 0=clear 1=rain 2=thunder, native (Level::updateWeather). */
    bool (*set_weather)(int32_t weather);

    /*  §B player management  */

    /** One SNBT per online player: {name,xuid,uuid,dim,x,y,z}. */
    void (*list_players)(void* ctx, PierStrSink snbt_sink);
    /** Resolve a player selector to their ActorUniqueID (bridges into the actor_* API). */
    bool (*player_resolve)(PierPlayerSel sel, PierActorId* out);
    bool (*player_send_message)(PierPlayerSel sel, PierStr msg);
    bool (*player_disconnect)(PierPlayerSel sel, PierStr reason);
    /** sendMessage to every online player. */
    void (*broadcast_message)(PierStr msg);
    /** 0=survival 1=creative 2=adventure 6=spectator, native (Player::setPlayerGameType). */
    bool (*player_set_gamemode)(PierPlayerSel sel, int32_t mode);
    /** Teleport natively (Actor::teleport). Custom dimensions (id >= 3) are allowed;
     *  the dimension bridge must produce an engine instance whose id matches, or the
     *  call fails instead of sending the player into a mismatched dimension. */
    bool (*player_teleport)(PierPlayerSel sel, int32_t dim, double x, double y, double z);
    bool (*player_get_num)(PierPlayerSel sel, int32_t prop, double* out);
    bool (*player_get_str)(PierPlayerSel sel, int32_t prop, void* ctx, PierStrSink sink);
    bool (*player_set_num)(PierPlayerSel sel, int32_t prop, double v);
    bool (*player_action)(
        PierPlayerSel sel,
        int32_t action,
        PierStr sarg,
        double a,
        double b,
        double c,
        void* ctx,
        PierStrSink out
    );

    /*  §C actors (players resolve here too, via player_resolve)  */

    /** Enumerate live actors; dim = -1 for all dimensions. */
    void (*list_actors)(int32_t dim, void* ctx, PierActorSink sink);
    /** Full Actor::save NBT as SNBT. */
    bool (*actor_snapshot)(PierActorId id, void* ctx, PierStrSink snbt_sink);
    bool (*actor_get_num)(PierActorId id, int32_t prop, double* out);
    bool (*actor_get_str)(PierActorId id, int32_t prop, void* ctx, PierStrSink sink);
    bool (*actor_action)(
        PierActorId id,
        int32_t action,
        PierStr sarg,
        double a,
        double b,
        double c,
        void* ctx,
        PierStrSink out
    );
    /** Spawn a mob (Spawner::spawnMob); on success *out = its ActorUniqueID. */
    bool (*spawn_mob)(int32_t dim, PierStr type_name, double x, double y, double z, PierActorId* out);
    /** Level::explode. source may be 0 (no source actor). */
    bool (*explode)(
        int32_t dim,
        double x,
        double y,
        double z,
        float radius,
        float max_resistance,
        PierActorId source,
        bool fire,
        bool breaks_blocks,
        bool allow_underwater
    );

    /*  §D blocks & block entities  */

    bool (*block_get_num)(int32_t dim, int32_t x, int32_t y, int32_t z, int32_t prop, double* out);
    bool (*block_get_str)(int32_t dim, int32_t x, int32_t y, int32_t z, int32_t prop, void* ctx, PierStrSink sink);
    bool (*block_action)(
        int32_t dim,
        int32_t x,
        int32_t y,
        int32_t z,
        int32_t action,
        PierStr sarg,
        void* ctx,
        PierStrSink out
    );
    /** BlockActor::save (with default SaveContext) as SNBT; false if none there. */
    bool (*block_entity_snbt)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);

    /*  §E items (SNBT value objects) & containers  */

    bool (*item_get_num)(PierStr item_snbt, int32_t prop, double* out);
    bool (*item_get_str)(PierStr item_snbt, int32_t prop, void* ctx, PierStrSink sink);
    /** Rebuild → mutate → serialize; out receives the NEW item SNBT. */
    bool (*item_transform)(PierStr item_snbt, int32_t op, PierStr sarg, double narg, void* ctx, PierStrSink out);
    bool (*container_size)(PierContainerRef ref, int32_t* out);
    /** Slot content as item SNBT (empty slots yield the air item's SNBT). */
    bool (*container_get_item)(PierContainerRef ref, int32_t slot, void* ctx, PierStrSink sink);
    bool (*container_set_item)(PierContainerRef ref, int32_t slot, PierStr item_snbt);
    bool (*container_add_item)(PierContainerRef ref, PierStr item_snbt);
    bool (*container_remove_item)(PierContainerRef ref, int32_t slot, int32_t count);
    bool (*container_clear)(PierContainerRef ref);

    /*  §F scoreboard  */

    bool (*scoreboard_op)(int32_t op, PierStr a, PierStr b, int64_t n, void* ctx, PierStrSink out);

    /*  §G forms (async result callback)  */

    /**
     * kind: 0=SimpleForm 1=CustomForm 2=ModalForm. form_snbt describes the
     * form (see docs/api/gui). The callback fires once, on the server thread,
     * and is muted if the mod is disabled before the player responds.
     */
    bool (*form_send)(
        PierModHandle mod,
        PierPlayerSel sel,
        int32_t kind,
        PierStr form_snbt,
        PierFormResultCb cb,
        void* user
    );

    /*  §H parameterized commands & enums  */

    /**
     * Like register_command, but with typed overloads. overloads_snbt:
     *   {overloads:[[{name:"target",kind:"player",optional:0b},…],…]}
     * kinds: int|bool|float|string|enum|soft_enum|actor|player|block_pos|vec3|
     *        raw_text|message|json|item|block_name|effect|actor_type|command|
     *        relative_float|file_path (enum/soft_enum also need "enum":"Name").
     * The callback's `args` receives the parse result as SNBT
     *   {overload:N, args:{<name>:…}}   and `origin_name` becomes origin SNBT
     *   {name,type,dim,x,y,z}.
     */
    bool (*register_command_ex)(
        PierModHandle mod,
        PierStr name,
        PierStr description,
        int32_t permission,
        PierStr overloads_snbt,
        PierCommandCb cb,
        void* user
    );
    /** values_snbt = {values:[["name",1L],…]}  → tryRegisterRuntimeEnum. */
    bool (*register_command_enum)(PierStr name, PierStr values_snbt);
    /** values_snbt = {values:["a","b"]}         → tryRegisterSoftEnum. */
    bool (*register_command_soft_enum)(PierStr name, PierStr values_snbt);
    /** op: 0=set 1=add 2=remove. */
    bool (*update_command_soft_enum)(PierStr name, int32_t op, PierStr values_snbt);

    /*  §I NBT binary, KvDb (thread-safe), system & server info  */

    /** fmt: 0=disk little-endian, 1=network. */
    bool (*nbt_snbt_to_binary)(PierStr snbt, int32_t fmt, void* ctx, PierBytesSink sink);
    bool (*nbt_binary_to_snbt)(uint8_t const* data, size_t len, int32_t fmt, void* ctx, PierStrSink sink);

    /* KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's
     * own data directory; ".." and absolute paths are rejected. Handles are
     * owned by the loader and force-closed (with a warning) at mod unload. */
    PierKvDbHandle (*kvdb_open)(PierModHandle mod, PierStr path, bool create_if_missing);
    void (*kvdb_close)(PierKvDbHandle h);
    bool (*kvdb_get)(PierKvDbHandle h, PierStr key, void* ctx, PierStrSink sink);
    bool (*kvdb_set)(PierKvDbHandle h, PierStr key, PierStr value);
    bool (*kvdb_del)(PierKvDbHandle h, PierStr key);
    bool (*kvdb_has)(PierKvDbHandle h, PierStr key);
    bool (*kvdb_is_empty)(PierKvDbHandle h);
    void (*kvdb_iter)(PierKvDbHandle h, void* ctx, PierKvSink sink);

    /* System info: THREAD-SAFE (plain OS calls). */
    bool (*sys_info_str)(int32_t prop, void* ctx, PierStrSink sink);
    bool (*sys_get_env)(PierStr name, void* ctx, PierStrSink sink);
    bool (*sys_set_env)(PierStr name, PierStr value);
    bool (*sys_is_wine)();

    /* Server / world-level settings. */
    bool (*get_difficulty)(int32_t* out); /* Level::getDifficulty */
    bool (*set_difficulty)(int32_t d); /* native Level::setDifficulty */
    bool (*get_seed)(int64_t* out); /* Level::getLevelSeed64 */
    /** out sink receives SNBT {type:"bool"|"int"|"float", value:…}; false if unknown rule. */
    bool (*game_rule_get)(PierStr name, void* ctx, PierStrSink sink);
    bool (*game_rule_set)(PierStr name, PierStr value); /* /gamerule */
    bool (*server_info_str)(int32_t prop, void* ctx, PierStrSink sink);

    /*
     * Per-player particle packet (additive, gated by struct_size).
     * Sends a SpawnParticleEffectPacket ONLY to the resolved player
     * (Player::sendNetworkPacket) instead of Level::spawnParticleEffect's
     * dimension-wide broadcast — other clients never receive it.
     * `dimension` is the vanilla dimension id carried in the packet; pass the
     * dimension the coordinates refer to (normally the player's own — clients
     * don't render particles for another dimension).
     * False if the player is offline / can't be resolved.
     */
    bool (*spawn_particle_for)(
        PierPlayerSel sel, int32_t dimension, PierStr effect_name, double x, double y, double z);

    /*
     * Raw per-connection packet send (additive, gated by struct_size) — the
     * generic primitive spawn_particle_for derives from.
     * `packet_id` is a MinecraftPacketIds value; `body`/`body_len` is the
     * packet's wire-format body for the CURRENT game version. The bridge
     * deserialises it into a real packet object (MinecraftPackets::createPacket
     * + Packet::read) and delivers it to the resolved player's connection only.
     * False if: player offline, unknown/unconstructible id, body fails to
     * parse, or bytes are left over after parsing (wrong shape for this
     * version). ESCAPE HATCH: the wire format is version-specific and is the
     * caller's responsibility; prefer typed entries when one exists.
     */
    bool (*send_packet)(PierPlayerSel sel, int32_t packet_id, uint8_t const* body, size_t body_len);

    /*
     * Tick control (additive, gated by struct_size). Backed by a bridge-owned
     * detour on Level::tick, installed lazily on the first control call and
     * left in place (idle cost: one predictable branch per frame — a control
     * call can arrive from a command handler that is executing INSIDE the
     * tick, where unpatching would not be safe). Server thread only.
     * While frozen, mobs/blocks/redstone/time stop; players can still move
     * and chat (movement is client-authoritative, network runs outside the
     * level tick).
     */
    bool (*tick_freeze)(bool on);
    /** Only while frozen: queue exactly n extra frames. False if not frozen or n == 0. */
    bool (*tick_step)(uint32_t n);
    /** 0 < factor <= 100. Fractional = slow motion (accumulator), 1.0 restores normal. */
    bool (*tick_warp)(double factor);

    /*
     * Per-subsystem MSPT profiler (additive, gated by struct_size). Backed by
     * five timing detours (Level/Dimension tick, redstone, chunk block ticks,
     * block entities), installed lazily on the first profile_begin and left
     * in place. One sampling window at a time. Server thread only.
     */
    /** Arm a window of `ticks` level ticks (1..12000). False if 0, too big, or already sampling. */
    bool (*profile_begin)(uint32_t ticks);
    /**
     * Poll for the finished report. False while sampling / nothing armed;
     * true exactly once per window, sinking one SNBT report:
     * {ticks:N, buckets:{level_tick:{us,calls}, dimension_tick:{…}, redstone:{…},
     *  chunk_blocks:{…}, block_entities:{…}}}. Bucket times are INCLUSIVE
     * (nested subsystems), report side by side, don't sum.
     */
    bool (*profile_take)(void* ctx, PierStrSink sink);

    /*
     * Simulated ("fake") players (additive, gated by struct_size).
     * sim_spawn creates a real ServerPlayer with that name — every existing
     * per-player entry (teleport, health, inventory, kick,…) works on it via
     * the usual name selector. sim_do multiplexes the simulate* verb family:
     * the action vocabulary grows bridge-side without new table slots
     * (verbs: despawn stop jump attack interact use_item drop respawn
     * move_to navigate_to look_at destroy_block destroy_look stop_destroy
     * interact_block sneak fly chat — args as SNBT, see docs). Gated on
     * isSimulatedPlayer(): a real player can never be puppeted. False on
     * unknown verb, malformed args, offline/non-sim target.
     */
    bool (*sim_spawn)(PierStr name, int32_t dimension, double x, double y, double z);
    bool (*sim_do)(PierPlayerSel sel, PierStr action, PierStr args_snbt);
    /** True if the selector resolves to a live simulated player. Lets a mod
     *  re-validate a bot after a restart (the SimulatedPlayer persists in the
     *  world, but in-memory handles don't). */
    bool (*sim_is)(PierPlayerSel sel);
    /** Enumerate the names of all live simulated players (sink receives each
     *  name). Rebuild a handle from a name to drive a bot that outlived the
     *  session that spawned it. */
    void (*sim_list)(void* ctx, PierStrSink name_sink);

    /*
     * Read-only world-data queries (additive, gated by struct_size). Both
     * stream one SNBT object per result through the sink; observational only.
     * Server thread only.
     */
    /** Enumerate villages in a dimension. Each: {uuid, center:[x,y,z],
     *  bounds:{min,max}, poi_count}. */
    void (*villages)(int32_t dimension, void* ctx, PierStrSink snbt_sink);
    /** Hardcoded spawn areas (nether fortress / witch hut / ocean monument /
     *  pillager outpost) whose chunks intersect a radius around (x,y,z). Each:
     *  {type, bounds:{min,max}}. Only LOADED chunks are inspected — a
     *  read-only query never force-loads. */
    void (*structures_near)(
        int32_t dimension, int32_t x, int32_t y, int32_t z, int32_t radius, void* ctx,
        PierStrSink snbt_sink);

    /*
     * Send a message of a specific TextPacketType to one player (additive,
     * gated by struct_size). `type` is a TextPacketType value:
     *   0 Raw · 1 Chat · 2 Translate · 3 Popup · 4 JukeboxPopup · 5 Tip ·
     *   6 SystemMessage · 7 Whisper · 8 Announcement · 9 TextObjectWhisper ·
     *   10 TextObject · 11 TextObjectAnnouncement.
     * Out-of-range falls back to Raw. Single-string body (like LSE tell): the
     * author/param kinds (Chat/Whisper/Translate) arrive as plain text.
     * plain `player_send_message` remains the Raw/Chat convenience path.
     */
    bool (*player_send_message_typed)(PierPlayerSel sel, PierStr msg, int32_t type);

    /*  Money (appended)
     *
     * Backed by LegacyMoney, which is delay-loaded. The whole family degrades
     * rather than crashing when the backend is absent or disabled, returning
     * each slot's failure value. The semantics below come from LegacyMoney's
     * source, not from guesswork:
     *
     *   - Amounts are always non-negative. val < 0 is rejected by the backend
     *     itself (the first check in LLMoney_Trans), and a negative set_money
     *     fails because the balance cannot be reduced to that target.
     *   - trans_money rejects from == to and applies the backend's configured
     *     pay_tax: the payee receives val - val * pay_tax, not val. To hand
     *     over the full amount, use add and reduce separately.
     *   - set_money's money is a target balance; the backend turns it into a
     *     single transfer internally.
     */

    /** Balance. Returns -1 on failure (empty xuid, database error, or absent
     *  backend); a real balance is never negative, so < 0 means "cannot say".
     *  Note that it opens an account at the configured default for an unseen
     *  xuid, so this is not a side-effect-free read. */
    int64_t (*get_money)(PierStr xuid);
    /** Set to money, which is a target balance rather than a delta. */
    bool (*set_money)(PierStr xuid, int64_t money);
    bool (*add_money)(PierStr xuid, int64_t money);
    bool (*reduce_money)(PierStr xuid, int64_t money);
    /** An empty from or to means created from or destroyed into nothing. What the
     *  payee receives is reduced by pay_tax (see above). from == to fails. */
    bool (*trans_money)(PierStr from, PierStr to, int64_t val, PierStr note);
    void (*money_get_hist)(PierStr xuid, int32_t timediff, void* ctx, PierStrSink sink);
    void (*money_clear_hist)(int32_t difftime);
    /** Register a before callback, which may veto. Several mods may each register
     *  one without overwriting the others, and registering the same function
     *  pointer twice is idempotent. The loader attributes each callback to its
     *  module and removes it when that mod unloads; LegacyMoney itself has no
     *  unregister interface, so this bookkeeping is the loader's. Registration
     *  does not require the backend to be ready yet: the loader installs the
     *  forwarding trampoline once it becomes available. */
    void (*money_listen_before_event)(PierMoneyCb callback);
    /** As above, but invoked after the change has happened; the return value is
     *  ignored. */
    void (*money_listen_after_event)(PierMoneyCb callback);
    void (*money_ranking)(uint16_t num, void* ctx, PierStrSink sink);

    /*  Appended: API gap fill, struct_size-gated.
     * All entries below are additive: older loaders (smaller struct_size)
     * simply won't have these fields. The SDK's init-time check rejects
     * mods built against a larger table than the loader provides. Unknown enum
     * keys return false. SERVER THREAD ONLY unless noted.                    */

    /*  Player: equipment, cooldown, network (dedicated fns)  */
    bool (*player_get_carried_item)(PierPlayerSel sel, void* ctx, PierStrSink sink);
    bool (*player_get_item)(PierPlayerSel sel, int32_t slot, void* ctx, PierStrSink sink);
    bool (*player_set_item)(PierPlayerSel sel, int32_t slot, PierStr item_snbt);
    /** All equipment as SNBT: [{slot, item_snbt},…] slot: 0=mainhand 1=offhand 2-5=armor */
    bool (*player_get_equipment)(PierPlayerSel sel, void* ctx, PierStrSink sink);
    /** Ticks remaining for an item cooldown (-1 if not on cooldown / player offline). */
    int32_t (*player_get_cooldown)(PierPlayerSel sel, PierStr item_name);
    bool (*player_start_cooldown)(PierPlayerSel sel, PierStr item_name, int32_t ticks);
    bool (*player_get_network_status)(PierPlayerSel sel, void* ctx, PierStrSink sink);

    /*  Actor: relationships, equipment, effects, geometry (dedicated fns)  */
    bool (*actor_get_vehicle)(PierActorId id, PierActorId* out);
    bool (*actor_get_first_passenger)(PierActorId id, PierActorId* out);
    bool (*actor_get_owner)(PierActorId id, PierActorId* out);
    bool (*actor_get_target)(PierActorId id, PierActorId* out);
    /** slot: 0=mainhand 1=offhand 2=helmet 3=chestplate 4=leggings 5=boots */
    bool (*actor_get_equipped_item)(PierActorId id, int32_t slot, void* ctx, PierStrSink sink);
    bool (*actor_set_equipped_item)(PierActorId id, int32_t slot, PierStr item_snbt);
    /** SNBT [{id, ticks, amplifier, visible},…] */
    bool (*actor_get_effects)(PierActorId id, void* ctx, PierStrSink sink);
    /** flag_index: ActorFlags enum value (0-based). */
    bool (*actor_get_status_flag)(PierActorId id, int32_t flag_index);
    bool (*actor_set_status_flag)(PierActorId id, int32_t flag_index, bool value);
    /** SNBT {type:"entity"|"block"|"none", pos:[x,y,z], entity_id?, block_name?} */
    bool (*actor_trace_ray)(PierActorId id, float max_dist, bool include_actors, bool include_blocks, void* ctx,
                            PierStrSink sink);
    bool (*actor_distance_to)(PierActorId id, PierActorId other, double* out);
    /** SNBT {min:[x,y,z], max:[x,y,z]} */
    bool (*actor_get_aabb)(PierActorId id, void* ctx, PierStrSink sink);
    bool (*actor_clone)(PierActorId id, int32_t dim, double x, double y, double z, PierActorId* out);

    /*  Block: state get/set, collision shape (dedicated fns)  */
    bool (*block_get_state)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr state_name, void* ctx,
                            PierStrSink sink);
    bool (*block_set_state)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr state_name, PierStr value);
    bool (*block_get_collision_shape)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);

    /*  Item: enchants, matching, NBT (dedicated fns)  */
    /** SNBT [{id, level},…] */
    bool (*item_get_enchants)(PierStr item_snbt, void* ctx, PierStrSink sink);
    /** enchants_snbt = [{id, level},…]; out = new item SNBT. */
    bool (*item_set_enchants)(PierStr item_snbt, PierStr enchants_snbt, void* ctx, PierStrSink out);
    bool (*item_matches)(PierStr a, PierStr b);
    bool (*item_get_user_data)(PierStr item_snbt, void* ctx, PierStrSink sink);

    /*  Level: biome, spawn, save, weather, path, sleep (dedicated fns)  */
    bool (*level_get_biome)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);
    bool (*level_get_default_spawn)(int32_t* x, int32_t* y, int32_t* z);
    bool (*level_set_default_spawn)(int32_t x, int32_t y, int32_t z);
    bool (*level_save)();
    /** SNBT {sleeping, total_players, active_sleeping} */
    bool (*level_get_sleep_status)(void* ctx, PierStrSink sink);
    bool (*level_update_weather)(float rain_level, int32_t rain_time, float lightning_level, int32_t lightning_time);
    /** SNBT {nodes:[{x,y,z},…], reached:1b/0b} */
    bool (*level_find_path)(PierActorId id, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);

    /*  Packet interception, appended and struct_size-gated.
     * Raw wire-format interception in both directions. This is the primitive
     * `send_packet` could not provide: it observes and rewrites bytes that
     * already exist, instead of manufacturing new ones.
     *
     * Delivery unit is exactly ONE packet — the leading unsigned-varint
     * header followed by the packet body. Batching and compression live
     * further down the peer chain (BatchedNetworkPeer splits inbound batches
     * and re-batches outbound ones), so a callback never sees a batch and
     * never has to produce a length prefix.
     *
     * The bridge decodes the header: `packet_id` is its low 10 bits,
     * `sender_sub_id` / `target_sub_id` the two 2-bit fields above it, and
     * `body`/`body_len` point PAST the header. A REPLACE verdict supplies a
     * new BODY only; the bridge re-encodes the header from `edit`, so a
     * rewrite never reproduces varint framing and packet-id remapping is a
     * field assignment rather than a byte-surgery exercise.
     *
     * Dispatch chains: with several subscribers, each one sees the output of
     * the previous, in registration order. The first DROP wins and the rest
     * are skipped. Subscriber lists are snapshotted before dispatch, so a
     * callback may register or unregister (including itself) safely.
     *
     * Threading — read this before touching game state. Inbound callbacks run
     * wherever the connection is pumped and outbound ones wherever the send
     * originates. In practice that is the server thread, but async flush
     * means it is not guaranteed. Treat these as "not necessarily the game
     * thread": keep them short, guard your own state, and route anything that
     * touches the world through `schedule`.
     *
     * Detours install lazily on the first subscriber and are never unpatched
     * (an unsubscribe can arrive from inside the hooked function). With no
     * subscribers the hook bodies fast-path straight to origin. */

    /**
     * Register a raw packet interceptor.
     * `dir_mask` is PIER_PKT_MASK_INBOUND | PIER_PKT_MASK_OUTBOUND (a
     * zero mask registers nothing and returns NULL). Returns NULL on failure.
     */
    PierPacketHookHandle (*packet_hook_register)(
        PierModHandle mod,
        int32_t dir_mask,
        PierPacketCb cb,
        void* user
    );

    /** Unregister. Safe to call from inside the callback. */
    bool (*packet_hook_unregister)(PierModHandle mod, PierPacketHookHandle handle);

    /**
     * Register a connection open/close observer. Returns NULL on failure.
     * The close notification is the only reliable signal for dropping
     * per-connection state: a connection that never finishes the login
     * handshake never becomes a Player, so no player event covers it.
     */
    PierPacketHookHandle (*packet_conn_hook_register)(PierModHandle mod, PierConnCb cb, void* user);

    /** Unregister. Safe to call from inside the callback. */
    bool (*packet_conn_hook_unregister)(PierModHandle mod, PierPacketHookHandle handle);

    /* Two capability groups follow, client and dimensions. They are unconditionally
     * present in the layout; when the capability package was not built into the
     * host, their slots are NULL (see the file header). New slots still go at the
     * real end of the struct, never inside a capability group. */

    /*  Capability group: client (client_*). All NULL on a server host.
     * Every callback fires on the client thread.  */
    /** Local player's name via ll::service::getClientInstance()->getLocalPlayer().
     *  sink receives the name, or the call returns false if not in a level. */
    bool (*client_get_local_player)(void* ctx, PierStrSink sink);

    /** True when the client is inside a level (a world is loaded). */
    bool (*client_is_in_level)();

    /** Current screen / UI name (e.g. "hud_screen", "pause_screen"). */
    bool (*client_get_screen_name)(void* ctx, PierStrSink sink);

    /** Register a key binding via ll::input::KeyRegistry::getOrCreateKey.
     *  Returns NULL on failure. The handle is owned by the caller; drop with
     *  client_unregister_key. down_cb/up_cb fire on the client thread. */
    PierKeyHandle (*client_register_key)(
        PierModHandle mod,
        PierStr name,
        int32_t const* key_codes,
        int32_t key_count,
        bool allow_remap,
        PierKeyCb down_cb,
        PierKeyCb up_cb,
        void* user
    );

    /** Unregister a key binding (destroys the ll::input::KeyHandle). */
    bool (*client_unregister_key)(PierKeyHandle handle);

    /** Currently assigned key codes (may differ from defaults if remapped).
     *  sink receives a JSON-style array string "[1,2,3]". */
    bool (*client_get_key_codes)(PierKeyHandle handle, void* ctx, PierStrSink sink);

    /*  Capability group: custom dimensions (md_*). All NULL when pier-dimensions
     * was not built into the host. Probing availability means checking whether
     * the md_is_available slot is NULL; that slot exists only to give the probe
     * a name, and always returns true when it is filled in.  */
    /** Check whether MoreDimensions is available in this loader build. */
    bool (*md_is_available)(void);

    /** Add a SimpleCustomDimension.
     *
     *  generatorType is ::GeneratorType verbatim — 1=Overworld, 2=Flat,
     *  3=Nether, 4=TheEnd, 5=Void. (This comment used to claim
     *  "0=Overworld 1=Nether 2=TheEnd 3=Flat 4=Void", which is the numbering
     *  bug that made "superflat" generate a nether. Values outside 1..5 are
     *  rejected rather than silently building a void world.)
     *
     *  Returns dim id (>=3) or -1 on failure. */
    int32_t (*md_add_simple_dimension)(PierStr name, uint32_t seed, int32_t generatorType);

    /** Per-dimension rules, consulted by the loader's own hooks.
     *
     *  Why this exists instead of gamerules: Bedrock gamerules are
     *  server-wide. Setting `doMobSpawning=false` to quiet a creative plot
     *  world also stops spawning in the survival world. These flags are
     *  checked inside hooks on the actual call sites (Spawner::spawnMob,
     *  Level::explode, ...), so they really are per-dimension.
     *
     *  `rule` is one of PierDimRule. Setting a rule on a dimension the
     *  loader doesn't know about is harmless — the tables are keyed by raw
     *  dimension id and consulted only when that id shows up in a hook.
     *
     *  Dimensions with no entry are left completely alone: the hooks fall
     *  through to origin(), so vanilla dimensions keep vanilla behaviour
     *  without the caller having to opt out. */
    void (*md_set_dimension_rule)(int32_t dimension, int32_t rule, bool allow);

    /** Read back a rule. `outAllow` is only written when the dimension has an
     *  explicit entry for that rule; returns false otherwise. */
    bool (*md_get_dimension_rule)(int32_t dimension, int32_t rule, bool* outAllow);

    /** Drop every rule for a dimension (used when a world is deleted). */
    void (*md_clear_dimension_rules)(int32_t dimension);

    /** Resolve a dimension name to its id. Returns -1 if not found.
     *
     *  Only returns an id for names that are ACTUALLY registered: unknown
     *  names yield -1, never VanillaDimensions::Undefined() (whose numeric
     *  value is mutated at runtime and looks like a valid id).
     *
     *  Note this is rarely what you want. `md_add_simple_dimension` and
     *  `md_add_plot_dimension` are idempotent — re-registering the same name
     *  on a later boot returns the same persisted id — so callers should
     *  register unconditionally at startup instead of probing first. */
    int32_t (*md_get_dimension_id)(PierStr name);

    /** Add a plot-world dimension: a custom dimension whose chunk generator
     *  produces a plot grid (plots / roads / borders) at generation time,
     *  instead of the caller painting blocks afterwards.
     *
     *  `layout_snbt` is a CompoundTag SNBT string:
     *    {plotSize:64, roadWidth:7, borderWidth:1, floorY:64,
     *     floorBlock:"minecraft:grass_block", fillBlock:"minecraft:dirt",
     *     roadBlock:"minecraft:birch_planks",
     *     borderBlock:"minecraft:stone_block_slab", biome:"minecraft:plains"}
     *  Missing keys fall back to those defaults; all values are clamped to a
     *  safe range on the C++ side. The layout is persisted with the dimension,
     *  so it stays fixed across restarts even if the caller's config changes.
     *
     *  Grid convention (the SDK MUST match): with cell = plotSize +
     *  roadWidth, a column at world (x, z) is road when
     *  mod(x,cell) >= plotSize || mod(z,cell) >= plotSize; otherwise it is
     *  border when within borderWidth of the plot edge; otherwise plot.
     *
     *  Idempotent, like md_add_simple_dimension. Returns dim id (>=3) or -1. */
    int32_t (*md_add_plot_dimension)(PierStr name, uint32_t seed, PierStr layout_snbt);

    /*  Append tail, struct_size-gated.
     * The struct's only append point. SDK mirrors declare every field
     * unconditionally, with no cfg or ifdef branches, because the layout is the
     * same on every target.
     *
     * Mod-scoped scheduling.
     * `schedule` / `schedule_after` above take a bare callback with no owner.
     * That is a use-after-free waiting to happen: a mod that schedules a task
     * and is then unloaded leaves the executor holding a function pointer into
     * a freed dylib. These replacements attribute each task to a mod, so the
     * loader can drop still-pending tasks when that mod goes away — the same
     * weak_ptr + ticket discipline the form callbacks already use.
     *
     * The old slots remain (ABI is additive) and still work. The loader now
     * attributes them by the callback's module (address to DLL) and drops
     * pending tasks at unload; mods should still prefer the owned slots below,
     * because attribution by address cannot see a callback that lives in a
     * different module. */

    /** Run `cb(user)` on the server (or client) thread ASAP, owned by `mod`.
     *  Thread-safe. Returns a task id (>0), or 0 if the task was rejected.
     *  If `mod` unloads before the task runs, the task is dropped and `cb` is
     *  never called — `user` is then leaked by design, because the only code
     *  that could free it lives in the dylib that just went away. */
    uint64_t (*schedule_for)(PierModHandle mod, PierTaskCb cb, void* user);

    /** As above, delayed by `delay_ms`. Thread-safe. Returns a task id (>0),
     *  or 0 if rejected. The timer itself is not cancelled on unload — it
     *  still expires — but the task is dropped when it does, so nothing calls
     *  into the freed dylib. */
    uint64_t (*schedule_after_for)(PierModHandle mod, PierTaskCb cb, void* user, uint64_t delay_ms);

    /** Drop a task scheduled by this mod if it has not run yet. Returns true
     *  if a pending task was actually dropped. Safe to call from any thread
     *  and from inside another task. Cancelling leaks `user` for the same
     *  reason as above, so prefer letting short tasks run. */
    bool (*schedule_cancel)(PierModHandle mod, uint64_t task_id);

    /** Number of tasks this mod still has pending. Intended for a mod to
     *  assert it has drained its own work in on_disable / on_unload, which is
     *  a precondition for being marked "reload_safe" in its manifest. */
    uint32_t (*schedule_pending_count)(PierModHandle mod);

    /*  Client-side container resync
     * `container_set_item` / `_clear` / `_add_item` all write through
     * `Container::setItem`, which mutates the server's copy and sends nothing.
     * The client keeps rendering whatever it last received, so a bulk rewrite
     * (swapping a player's inventory on a cross-dimension teleport, say) looks
     * like it did nothing until the player clicks a slot and forces a resync.
     *
     * Call this once after a batch of writes. Batching matters: this pushes
     * the whole container, so calling it per-slot inside a loop is a packet
     * storm for no benefit. */

    /** Resend a player-owned container (which 0..3) to its owner. Returns
     *  false for block containers (which == 4) — a chest has no single owner
     *  to resend to; its viewers are refreshed by the engine's own container
     *  transaction path. */
    bool (*container_refresh)(PierContainerRef ref);

    /*  Titles
     * `PACT_SET_TITLE` (player_action opcode 6) reaches the client by running
     * the console command `title "<name>" title <text>`. Three things are
     * wrong with that and none of them are theoretical:
     *   - the text is pasted into a command line unquoted, so a plot named
     *     `He said "hi"` truncates the command;
     *   - `title`'s text parameter is a `message`, which expands selectors —
     *     a plot named `@e` is a command injection, not a name;
     *   - `/title` has no way to set fade/stay for the same call, so timing is
     *     whatever the client last stored.
     * This slot builds a real SetTitlePacket instead. No wire format crosses
     * the FFI (the packet is constructed field-by-field on this side), so it
     * survives protocol bumps the way `spawn_particle_for` does.
     *
     * `type` is SetTitlePacketPayload::TitleType:
     *   0 Clear · 1 Reset · 2 Title · 3 Subtitle · 4 Actionbar · 5 Times
     * The TextObject variants (6..8) need a ResolvedTextObject and are refused.
     * `text` is ignored for Clear/Reset/Times.
     *
     * Durations are in TICKS. For 2/3/4, when all three are >= 0 a Times
     * packet is sent first so the timing is deterministic rather than
     * inherited from whatever the client last stored; pass -1 for all three to
     * keep the client's current timing. Mixing (-1 with >=0) is refused rather
     * than guessed at — a half-specified duration set has no sane meaning.
     * Server thread only. */
    bool (*player_send_title)(
        PierPlayerSel sel, int32_t type, PierStr text, int32_t fade_in_ticks,
        int32_t stay_ticks, int32_t fade_out_ticks);

    /*  Cross-mod event bus
     * See PierBusCb above for why the loader owns the table instead of mods
     * exchanging pointers. All four are thread-safe; callbacks run on the
     * publishing thread.
     *
     * A mod does not receive its own publishes. Two reasons: a mod that
     * wants to notify itself has a direct function call available, and
     * self-delivery is the one loop shape that no depth limit can distinguish
     * from legitimate work. Cross-mod loops (A publishes → B's handler
     * publishes → A's handler publishes →…) are caught by a depth cap
     * instead; hitting it drops the innermost publish and logs once. */

    /** Subscribe `mod` to `topic`. Returns a subscription id (>0), or 0 if the
     *  topic is empty/oversized, the callback is null, or the mod is unknown.
     *  Subscriptions are dropped automatically when the mod unloads. */
    uint64_t (*bus_subscribe)(PierModHandle mod, PierStr topic, PierBusCb cb, void* user);

    /** Drop one of this mod's subscriptions. Scoped to the caller — a mod
     *  cannot unsubscribe another mod. Returns true if one was removed.
     *  Safe to call from inside a callback (including one's own). */
    bool (*bus_unsubscribe)(PierModHandle mod, uint64_t sub_id);

    /** Deliver `payload` to every *other* mod subscribed to `topic`. Returns
     *  how many subscribers actually ran (0 is normal — nobody is listening).
     *  Return values from subscribers are ignored. */
    uint32_t (*bus_publish)(PierModHandle mod, PierStr topic, PierStr payload);

    /** As above, but collects the veto bit: returns true when any
     *  subscriber returned true. Every subscriber still runs — no
     *  short-circuit — so observers see a consistent stream whether or not an
     *  earlier one refused. `out_delivered` may be NULL. */
    bool (*bus_publish_vetoable)(
        PierModHandle mod, PierStr topic, PierStr payload, uint32_t* out_delivered);

    /** How many subscribers a topic has right now, across all mods. Intended
     *  for skipping the cost of building a payload nobody will read. */
    uint32_t (*bus_subscriber_count)(PierStr topic);

    /*  Plot-boundary confinement
     * Backing store for PIER_DIMRULE_PISTON_CROSS_PLOT and
     * PIER_DIMRULE_ENTITY_CROSS_PLOT. Those two rules ask "are these two
     * columns in the same plot?", and the answer needs the grid geometry plus
     * the merge markers. The question is asked from
     * `PistonBlockActor::_checkAttachedBlocks` and `Actor::move` — engine tick
     * paths, hundreds of calls a second — so the data is pushed here once and
     * read natively rather than queried back across the FFI.
     *
     * The ownership rule implemented on the loader side mirrors the plugin's
     * own `owning_plot`: a seam between two merged plots counts as plot, a
     * junction counts as plot only when all four surrounding edges are merged.
     * Divergence does not show up as "one column judged wrong" — it shows up as
     * an owner who can place a block by hand on their merged plot but whose
     * piston refuses to push there. Server thread only. */

    /** Register (or update) the plot grid of a dimension. `plot_size <= 0`
     *  clears it. Values are clamped loader-side — `cell = plot_size +
     *  road_width` is a modulus, and a caller-supplied 0 would divide by zero
     *  in a tick path. Clears the merge table when the geometry changes. */
    void (*md_set_plot_grid)(int32_t dimension, int32_t plot_size, int32_t road_width);

    /** Drop a dimension's grid and merge table (world deleted, or the world
     *  stopped using the plot model). */
    void (*md_clear_plot_grid)(int32_t dimension);

    /** Replace a dimension's merge markers wholesale. `entries` is `count`
     *  triples `(x, z, mask)`, i.e. `count * 3` int32s; `mask` is a bitset of
     *  1=north, 2=east, 4=south, 8=west matching the plugin's `merged[]`
     *  indices. Only plots that actually carry a marker need to be sent.
     *
     *  Wholesale, not incremental: incremental requires both sides to agree
     *  forever on what is currently in the table, and `unlink` clears the
     *  neighbour before storing itself — a failure in between leaves the two
     *  views apart with no way back. Replacing pulls them into agreement on
     *  every push. Call `md_set_plot_grid` first; a push for an unregistered
     *  dimension is dropped with a warning. */
    void (*md_set_plot_merges)(int32_t dimension, int32_t const* entries, int32_t count);

    /*  Cross-mod service registry (query-style calls)
     * The bus is one-way broadcast; this is request/response. The shapes differ
     * on every axis, which is why they are separate tables rather than one:
     *
     *   - providers per name: bus any / service exactly one
     *   - nobody registered:  bus normal / service an error the caller handles
     *   - return value:       bus none / service the entire point
     *   - ordering:           bus undefined and must not matter / service n/a
     *
     * Registration is EXCLUSIVE. Two mods answering `plot:can` is not "both
     * run" — it is an ambiguous answer with no way for the caller to pick, so
     * the second registrar is refused loudly. Silent last-wins would make the
     * answer depend on mod load order, which nobody controls and which changes
     * when an unrelated mod is installed.
     *
     * Ownership follows the same weak_ptr + ticket discipline as the bus and
     * the forms: the loader keeps the table, and the call path revalidates the
     * provider immediately before crossing into its dylib.
     *
     * Synchronous, on the caller's thread, no timeout. A provider that blocks
     * blocks the server thread exactly like any other callback; returning
     * "timed out" while the callback kept running would hand the caller a wrong
     * answer AND leave the provider running. */

    /** Register `mod` as the provider of `name`. Returns a registration id
     *  (>0), or 0 if the name is empty/oversized/already taken, the callback is
     *  null, or the mod is unknown. Dropped automatically on unload. */
    uint64_t (*service_register)(
        PierModHandle mod, PierStr name, PierServiceCb cb, void* user);

    /** Drop one of this mod's registrations. Scoped to the caller — a mod
     *  cannot unregister another mod's service. */
    bool (*service_unregister)(PierModHandle mod, uint64_t reg_id);

    /** Call `name` with `request`; the provider's answer arrives through
     *  `reply`. Returns one of PIER_SERVICE_*. A mod cannot call its own
     *  service (it has a direct function call, and self-calls are the least
     *  legible loop shape). */
    int32_t (*service_call)(
        PierModHandle mod, PierStr name, PierStr request, void* ctx, PierStrSink reply);

    /** Every registered service as a JSON array of `{"name":…,"mod":…}`.
     *  For diagnostics and for a caller deciding whether to build a request
     *  nobody can answer. */
    void (*service_list)(void* ctx, PierStrSink sink);

    /*  Bulk world editing, appended and struct_size-gated.
     * Native write paths that bypass the console-command route used by
     * set_block (`execute in <dim> run setblock…`). With these, block
     * states come from structured NBT instead of command-string splicing,
     * block entities can be written back, and entities can be respawned from
     * saved NBT — all via existing engine entry points.
     *
     * update_flags is a bitmask: 1 = notify neighbours, 2 = sync client,
     * 3 = both (equivalent to /setblock), 0 = neither (fastest for bulk
     * fills, but the caller must resync afterwards). Server thread only. */

    /** Write a block from serialized NBT ({name,states,version}, i.e. the
     *  shape get_block produces). */
    bool (*edit_set_block_nbt)(
        int32_t dim, int32_t x, int32_t y, int32_t z, PierStr snbt, int32_t update_flags);

    /** Write a block from a name + optional partial states. An empty
     *  states_snbt means all-default states; the version is taken from the
     *  default state on the loader side — the caller must not supply one. */
    bool (*edit_set_block_states)(
        int32_t dim, int32_t x, int32_t y, int32_t z, PierStr name, PierStr states_snbt,
        int32_t update_flags);

    /** Write a block entity's NBT back (BlockActor::load). The cell must
     *  already hold the matching block. */
    bool (*edit_set_block_entity)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr snbt);

    /** Spawn an entity from full NBT (the inverse of actor_snapshot). When
     *  use_pos is true, (x,y,z) overrides the Pos tag; the UniqueID is
     *  reassigned by the engine and returned via out. */
    bool (*edit_spawn_entity_nbt)(
        int32_t dim, PierStr snbt, bool use_pos, double x, double y, double z,
        PierActorId* out);

    /** Ray trace yielding the BLOCK coordinate and hit face:
     *  {type, block:[x,y,z], facing, pos:[x,y,z], entity}. */
    bool (*edit_trace_ray)(
        PierActorId id, float max_dist, bool include_actors, bool include_blocks, void* ctx,
        PierStrSink sink);

    /*  Same-toolchain fast lane, appended and struct_size-gated.
     * Five slots appended without touching PIER_ABI_VERSION: a pure append is
     * not a version change, and struct_size is the precise gate.
     *
     * Both directions hold. A new loader running an old mod: the old table is a
     * byte-identical prefix of the new one, the mod cannot reach these five
     * slots, and it works unchanged. A new mod on an old loader: SDK runtime
     * init compares struct_size, finds the loader's table shorter than the one
     * it was compiled against, and refuses to load. That is the right outcome,
     * since a mod that reads the lane_publish cell on a loader without it would
     * read out of bounds.
     *
     * In short: the version number tracks "semantics changed", struct_size
     * tracks "the table grew". This change is only the latter.
     *
     * See the long comment at PierLaneDesc above. In one line: service is the
     * cross-language (name, JSON) -> JSON channel, while this is a direct
     * function-table call that holds only when both sides were built by the same
     * toolchain; a fingerprint mismatch yields no pointer and the consumer falls
     * back to service.
     *
     * Server thread only. */

    /** Publish a lane. Exclusive, same discipline as service_register: if the name
     *  is taken, return 0 and name the incumbent in the log. Returns a publish
     *  id (> 0), withdrawn automatically at unload. */
    uint64_t (*lane_publish)(PierModHandle mod, PierStr name, PierLaneDesc const* desc);

    /** Withdraw one of your own lanes. Calls release for every outstanding lease
     *  and clears the liveness flag, so a consumer's next check sees the lane
     *  gone instead of jumping through a dead pointer. */
    bool (*lane_unpublish)(PierModHandle mod, uint64_t pub_id);

    /** Acquire a lane. want_fingerprint must be the value the consumer computed
     *  itself.
     *
     *  0 is not a valid fingerprint and always yields PIER_LANE_FINGERPRINT.
     *  It must not mean "skip the check": this call hands over raw vtable and
     *  data pointers, which the consumer then calls through its own table
     *  offsets, so skipping the check is type confusion. To inspect which lanes
     *  exist and what their fingerprints are, use lane_list, which hands over
     *  no pointers at all.
     *
     *  Returns a PIER_LANE_* value. After a successful acquire, lane_release is
     *  mandatory, or the provider's state stays retained. */
    int32_t (*lane_acquire)(
        PierModHandle mod, PierStr name, uint64_t want_fingerprint, PierLaneRef* out);

    /** Return a lease. Only your own. Returns false when the provider is already
     *  gone: the loader has called release on your behalf by then, and calling
     *  it again would be a double free. */
    bool (*lane_release)(PierModHandle mod, uint64_t lease);

    /** Every lane, as a JSON array:
     *  [{"name":…,"mod":…,"fingerprint":"0x…","protocol":1,"leases":N,"alive":true}] */
    void (*lane_list)(void* ctx, PierStrSink sink);

    /**
     * List every registered custom dimension as a JSON array:
     * [{"name":"plot_world","dim":1000,"snbt":"{…}"}].
     *
     * Without this slot the md_* family can only be queried by name
     * (md_get_dimension_id), so a caller must already know the name. A world
     * manager taking over an existing save would then be blind to dimensions
     * created by a previous plugin: they sit in dimension_config.json, they are
     * alive in the engine, players can teleport into them, and the manager's
     * table has no row for them. The consequence is not a short listing but
     * dimensions governed by no rules at all, plus the risk that a newly created
     * world is assigned a number that collides with one of them, leaving two
     * worlds sharing a dimension id.
     *
     * name comes from the config file key, dim is the engine-assigned and
     * persisted number, and snbt is the verbatim generation parameters (a plot
     * world gives {layout:{…},seed:N}, a simple world {generatorType:Flat,
     * seed:N}) for the caller to interpret.
     *
     * The sink is invoked ONCE PER DIMENSION, each with one JSON object -- not
     * once with an array. Contrast lane_list above, which hands over a single
     * JSON array. Both shapes exist in this table; each slot says which.
     *
     * The callback is not invoked at all when md_is_available() is false.
     */
    void (*md_list_dimensions)(void* ctx, PierStrSink sink);

    /**
     * Delete every save-file key belonging to one chunk, so the engine
     * regenerates it from the generator on next load.
     *
     * Restoring an area by writing every cell with set_block is the wrong
     * approach: a 32x32 plot times the world height is hundreds of thousands of
     * cells and as many FFI crossings, and it still misses things, because block
     * entities, actors and pending ticks (redstone, crop growth) are not block
     * data. After such a rewrite the chests are still there and the redstone is
     * still running.
     *
     * Erasing the save keys has neither problem: one forEachKeyWithPrefix yields
     * every key of the chunk (all tags, all subchunks, actors, block entities,
     * pending ticks), one pass deletes them, and the engine regenerates from the
     * generator on next load.
     *
     * Key shape: a BDS chunk key is prefixed with
     * <chunkX:i32 LE><chunkZ:i32 LE>, followed by <dimension:i32 LE> outside the
     * overworld. After the prefix come a tag byte and a subchunk index, which
     * this slot does not interpret; deleting everything with the prefix is
     * exactly "everything in this chunk".
     *
     * The chunk must be unloaded. A loaded chunk has a LevelChunk in memory that
     * the engine writes back on unload, recreating the deleted keys verbatim, so
     * the deletion is silently undone. The caller is responsible for getting the
     * chunk unloaded first (move players away, wait for it to leave tick range).
     * This slot does not do that: deciding who is nearby and when unloading is
     * acceptable needs the caller's domain knowledge, which this layer must not
     * have.
     *
     * @return number of keys deleted; -1 if the save layer is unavailable. 0 is
     *         a normal result, meaning that chunk was never generated.
     *
     * Pure append: PIER_ABI_VERSION is unchanged, struct_size is the gate.
     */
    int32_t (*level_delete_chunk_keys)(int32_t dim, int32_t chunk_x, int32_t chunk_z);

    /**
     * Are the chunks covering [min..max] currently loaded in memory?
     *
     * Companion to level_delete_chunk_keys: erasing save keys only works on
     * unloaded chunks, since a loaded one lives in memory and writes the deleted
     * keys back verbatim on unload, while the erase itself "succeeds" and
     * reports a positive key count. Without this slot a caller can only guess
     * from "nobody is nearby", and guessing wrong fails silently.
     *
     * @return 1 if all are loaded, 0 if at least one is not, -1 if the dimension
     *         is unavailable.
     *
     * Pure append: PIER_ABI_VERSION is unchanged, struct_size is the gate.
     */
    int32_t (*level_chunks_loaded)(int32_t dim, int32_t min_x, int32_t min_z, int32_t max_x, int32_t max_z);

    /**
     * This player's connection id — the same number packet interceptors see
     * in the packet context.
     *
     * A packet callback has only conn_id, not a player, so per-player rewriting
     * of outbound packets is impossible without this: locking the sky colour
     * needs the dimension of the person on that connection, which needs to know
     * who they are.
     *
     * The alternative is to periodically send packets that override the
     * server's, and that is wrong: the server sends real time while the mod
     * sends locked time, the two kinds interleave, and the client's sky flickers
     * between them. Rewriting is correct, and rewriting needs this slot.
     *
     * @return the connection id; 0 if the player is offline or their network
     *         identifier is unavailable.
     *
     * Pure append: PIER_ABI_VERSION is unchanged, struct_size is the gate.
     */
    uint64_t (*player_conn_id)(PierPlayerSel who);

    /**
     * List every save-file key belonging to one chunk. One callback per key.
     *
     * Listing and deleting are two slots rather than one because collecting the
     * keys into a std::vector<std::string> on the C++ side and deleting them in
     * a loop crashed on real hardware, in the vector's destructor at return,
     * with a string's inline buffer being treated as a heap pointer.
     *
     * The root cause was never pinned down (std::string lifetime across a DLL
     * boundary, not findable without a debugger), but the whole class of problem
     * comes from accumulating a string container on the C++ side and crossing a
     * virtual call with it. So the container lives on the caller's side and the
     * C++ side does one thing at a time, owning nothing.
     *
     * Keys are binary and contain 0 bytes, hence PierStr with an explicit length
     * rather than a C string.
     *
     * @return how many keys were reported; -1 if the save layer is unavailable.
     */
    int32_t (*level_chunk_keys)(int32_t dim, int32_t chunk_x, int32_t chunk_z, void* ctx, PierStrSink sink);

    /**
     * Delete one chunk-category key, verbatim.
     *
     * Companion to level_chunk_keys. The key's content is not interpreted:
     * whatever is passed is what gets deleted, which is exactly why it is safe,
     * since it need not understand the subchunk format.
     */
    bool (*level_delete_key)(PierStr key);

    /*  Appended slots (190). Added at the tail only, guarded by struct_size.
     *
     * Removing an actor and healing one already exist as actor_action's
     * AACT_DESPAWN and AACT_HEAL. A separate slot would do the same job twice,
     * and two implementations eventually drift.
     */

    /* Actor enumeration already exists as list_actors (everything in a dimension,
     * with type names); combined with actor_get_num for positions it filters to a
     * box. An actors_in_box slot would do the same job twice, and two
     * implementations eventually drift.
     */

    /**
     * Set the biome over an area.
     *
     * Applied per whole column, so no y is taken: setBiome3d works per y, but
     * Bedrock stores biomes per column. biome is a biome name such as
     * "minecraft:plains".
     *
     * Returns how many columns were set. 0 means none were, either because the
     * chunks are not loaded or because the name was not recognised.
     */
    int32_t (*level_set_biome)(int32_t dim,
                               int32_t minX, int32_t minZ,
                               int32_t maxX, int32_t maxZ,
                               PierStr biome);

    /*  Appended: the liquid layer (waterlogged blocks).
     *
     * In Bedrock, waterlogging is not a block state but a second block in the
     * same cell: stairs, fences or coral in the main layer and water in the
     * liquid layer. get_block and set_block see the main layer only, so copying
     * and pasting waterlogged stairs loses all the water: the main layer is
     * exactly right and the other layer is missing.
     *
     * These two slots expose the liquid layer. An empty layer reads back as
     * "minecraft:air".
     */

    /** Read the liquid layer. The sink receives a block name such as
     *  "minecraft:water"; an empty layer gives air. */
    bool (*get_extra_block)(int32_t dim, int32_t x, int32_t y, int32_t z,
                            void* ctx, PierStrSink sink);

    /** Write the liquid layer. block_spec takes a bare block name or full SNBT;
     *  write "minecraft:air" to clear it. update_flags is as in
     *  edit_set_block_nbt: bit 1 notifies neighbours, bit 2 syncs the client. */
    bool (*set_extra_block)(int32_t dim, int32_t x, int32_t y, int32_t z,
                            PierStr block_spec, int32_t update_flags);
} PierApi;

/**
 * Filled in by the mod inside pier_main. instance is the mod's own opaque
 * pointer; the three callbacks may be NULL, which counts as always succeeding.
 * This struct follows the same append rules as PierApi: with struct_size, new
 * lifecycle callbacks can be added at the tail without a version bump, and the
 * host calls one only if it can reach it.
 */
typedef struct PierModVTable
{
    /** sizeof(PierModVTable), filled in by the mod from the definition it compiled. */
    uint32_t struct_size;
    /** Equals the PIER_ABI_VERSION the mod was compiled against. */
    uint32_t abi_version;
    /** Bitwise OR of PIER_FLAG_*. Bit 0 means built for a client target. */
    uint32_t mod_flags;
    /** Reserved, always 0. */
    uint32_t _reserved0;
    void* instance;
    bool (*on_enable)(void* instance);
    bool (*on_disable)(void* instance);
    bool (*on_unload)(void* instance);
} PierModVTable;

/**
 * The single symbol every mod must export:
 *
 *   bool pier_main(const PierApi* api, PierModHandle self,
 *                     PierModVTable* out_vtable);
 *
 * Called once on the server thread while the mod is being loaded.
 * Return false to abort loading.
 */
typedef bool (*PierMainFn)(const PierApi* api, PierModHandle self, PierModVTable* out_vtable);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PIER_SDK_ABI_H */

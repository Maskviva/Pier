# Cross-mod: bus, services and lanes

??? note "Section notes in abi.h"

    **Cross-mod event bus**

    See PierBusCb above for why the loader owns the table instead of mods exchanging pointers. All four are thread-safe; callbacks run on the publishing thread.

    A mod does not receive its own publishes. Two reasons: a mod that wants to notify itself has a direct function call available, and self-delivery is the one loop shape that no depth limit can distinguish from legitimate work. Cross-mod loops (A publishes → B's handler publishes → A's handler publishes →…) are caught by a depth cap instead; hitting it drops the innermost publish and logs once.

    **Cross-mod service registry (query-style calls)**

    The bus is one-way broadcast; this is request/response. The shapes differ on every axis, which is why they are separate tables rather than one:

    ```text
    - providers per name: bus any / service exactly one
    - nobody registered:  bus normal / service an error the caller handles
    - return value:       bus none / service the entire point
    - ordering:           bus undefined and must not matter / service n/a
    ```

    Registration is EXCLUSIVE. Two mods answering `plot:can` is not "both run" — it is an ambiguous answer with no way for the caller to pick, so the second registrar is refused loudly. Silent last-wins would make the answer depend on mod load order, which nobody controls and which changes when an unrelated mod is installed.

    Ownership follows the same `weak_ptr` + ticket discipline as the bus and the forms: the loader keeps the table, and the call path revalidates the provider immediately before crossing into its dylib.

    Synchronous, on the caller's thread, no timeout. A provider that blocks blocks the server thread exactly like any other callback; returning "timed out" while the callback kept running would hand the caller a wrong answer AND leave the provider running.

    **Same-toolchain fast lane, appended and struct\_size-gated.**

    Five slots appended without touching `PIER_ABI_VERSION`: a pure append is not a version change, and `struct_size` is the precise gate.

    Both directions hold. A new loader running an old mod: the old table is a byte-identical prefix of the new one, the mod cannot reach these five slots, and it works unchanged. A new mod on an old loader: SDK runtime init compares `struct_size`, finds the loader's table shorter than the one it was compiled against, and refuses to load. That is the right outcome, since a mod that reads the `lane_publish` cell on a loader without it would read out of bounds.

    In short: the version number tracks "semantics changed", `struct_size` tracks "the table grew". This change is only the latter.

    See the long comment at PierLaneDesc above. In one line: service is the cross-language (name, JSON) -&gt; JSON channel, while this is a direct function-table call that holds only when both sides were built by the same toolchain; a fingerprint mismatch yields no pointer and the consumer falls back to service.

    Server thread only.

    **Appended: bulk block reads and writes**

## Slots {#slots}

### `bus_subscribe` {#bus_subscribe}

```c
uint64_t (*bus_subscribe)(PierModHandle mod, PierStr topic, PierBusCb cb, void* user);
```

Subscribe `mod` to `topic`. Returns a subscription id (&gt;0), or 0 if the topic is empty/oversized, the callback is null, or the mod is unknown. Subscriptions are dropped automatically when the mod unloads.

- Call: `api->bus_subscribe(mod, topic, cb, user)`
- Parameters:
    - mod : `PierModHandle`
    - topic : `PierStr`
    - cb : `PierBusCb`
    - user : `void*`
- Return type: `uint64_t`
- Section of abi.h: Cross-mod event bus
- Position in the table: slot 157, counting from 0
- Callers in each binding:
    - Rust: [`bus::subscribe`](../rust/bus.md#fn.subscribe)
    - Go: [`BusSubscribe`](../go/crossmod.md#BusSubscribe)
    - Zig: [`busSubscribe`](../zig/crossmod.md#busSubscribe)

### `bus_unsubscribe` {#bus_unsubscribe}

```c
bool (*bus_unsubscribe)(PierModHandle mod, uint64_t sub_id);
```

Drop one of this mod's subscriptions. Scoped to the caller — a mod cannot unsubscribe another mod. Returns true if one was removed. Safe to call from inside a callback (including one's own).

- Call: `api->bus_unsubscribe(mod, sub_id)`
- Parameters:
    - mod : `PierModHandle`
    - sub_id : `uint64_t`
- Return type: `bool`
- Section of abi.h: Cross-mod event bus
- Position in the table: slot 158, counting from 0
- Callers in each binding:
    - Go: [`Subscription.Unsubscribe`](../go/crossmod.md#Subscription.Unsubscribe), [`Raw.BusUnsubscribe`](../go/raw.md#Raw.BusUnsubscribe)
    - Zig: [`Subscription.unsubscribe`](../zig/crossmod.md#Subscription.unsubscribe)

### `bus_publish` {#bus_publish}

```c
uint32_t (*bus_publish)(PierModHandle mod, PierStr topic, PierStr payload);
```

Deliver `payload` to every \*other\* mod subscribed to `topic`. Returns how many subscribers actually ran (0 is normal — nobody is listening). Return values from subscribers are ignored.

- Call: `api->bus_publish(mod, topic, payload)`
- Parameters:
    - mod : `PierModHandle`
    - topic : `PierStr`
    - payload : `PierStr`
- Return type: `uint32_t`
- Section of abi.h: Cross-mod event bus
- Position in the table: slot 159, counting from 0
- Callers in each binding:
    - Rust: [`bus::publish`](../rust/bus.md#fn.publish)
    - Go: [`BusPublish`](../go/crossmod.md#BusPublish), [`Raw.BusPublish`](../go/raw.md#Raw.BusPublish)
    - Zig: [`busPublish`](../zig/crossmod.md#busPublish)

### `bus_publish_vetoable` {#bus_publish_vetoable}

```c
bool (*bus_publish_vetoable)(
    PierModHandle mod, PierStr topic, PierStr payload, uint32_t* out_delivered);
```

As above, but collects the veto bit: returns true when any subscriber returned true. Every subscriber still runs — no short-circuit — so observers see a consistent stream whether or not an earlier one refused. `out_delivered` may be NULL.

- Call: `api->bus_publish_vetoable(mod, topic, payload, out_delivered)`
- Parameters:
    - mod : `PierModHandle`
    - topic : `PierStr`
    - payload : `PierStr`
    - out_delivered : `uint32_t*`
- Return type: `bool`
- Section of abi.h: Cross-mod event bus
- Position in the table: slot 160, counting from 0
- Callers in each binding:
    - Rust: [`bus::publish_vetoable`](../rust/bus.md#fn.publish_vetoable)
    - Go: [`BusPublishVetoable`](../go/crossmod.md#BusPublishVetoable), [`Raw.BusPublishVetoable`](../go/raw.md#Raw.BusPublishVetoable)
    - Zig: [`busPublishVetoable`](../zig/crossmod.md#busPublishVetoable)

### `bus_subscriber_count` {#bus_subscriber_count}

```c
uint32_t (*bus_subscriber_count)(PierStr topic);
```

How many subscribers a topic has right now, across all mods. Intended for skipping the cost of building a payload nobody will read.

- Call: `api->bus_subscriber_count(topic)`
- Parameters:
    - topic : `PierStr`
- Return type: `uint32_t`
- Section of abi.h: Cross-mod event bus
- Position in the table: slot 161, counting from 0
- Callers in each binding:
    - Rust: [`bus::subscriber_count`](../rust/bus.md#fn.subscriber_count), [`bus::subscriber_count`](../rust/bus.md#fn.subscriber_count)
    - Go: [`BusSubscriberCount`](../go/crossmod.md#BusSubscriberCount), [`Raw.BusSubscriberCount`](../go/raw.md#Raw.BusSubscriberCount)
    - Zig: [`busSubscriberCount`](../zig/crossmod.md#busSubscriberCount)

### `service_register` {#service_register}

```c
uint64_t (*service_register)(
    PierModHandle mod, PierStr name, PierServiceCb cb, void* user);
```

Register `mod` as the provider of `name`. Returns a registration id (&gt;0), or 0 if the name is empty/oversized/already taken, the callback is null, or the mod is unknown. Dropped automatically on unload.

- Call: `api->service_register(mod, name, cb, user)`
- Parameters:
    - mod : `PierModHandle`
    - name : `PierStr`
    - cb : `PierServiceCb`
    - user : `void*`
- Return type: `uint64_t`
- Section of abi.h: Cross-mod service registry (query-style calls)
- Position in the table: slot 163, counting from 0
- Callers in each binding:
    - Rust: [`service::register`](../rust/service.md#fn.register), [`service::register_json`](../rust/service.md#fn.register_json)
    - Go: [`RegisterService`](../go/crossmod.md#RegisterService)
    - Zig: [`registerService`](../zig/crossmod.md#registerService)

### `service_unregister` {#service_unregister}

```c
bool (*service_unregister)(PierModHandle mod, uint64_t reg_id);
```

Drop one of this mod's registrations. Scoped to the caller — a mod cannot unregister another mod's service.

- Call: `api->service_unregister(mod, reg_id)`
- Parameters:
    - mod : `PierModHandle`
    - reg_id : `uint64_t`
- Return type: `bool`
- Section of abi.h: Cross-mod service registry (query-style calls)
- Position in the table: slot 164, counting from 0
- Callers in each binding:
    - Go: [`ServiceRegistration.Unregister`](../go/crossmod.md#ServiceRegistration.Unregister), [`Raw.ServiceUnregister`](../go/raw.md#Raw.ServiceUnregister)
    - Zig: [`ServiceRegistration.unregister`](../zig/crossmod.md#ServiceRegistration.unregister)

### `service_call` {#service_call}

```c
int32_t (*service_call)(
    PierModHandle mod, PierStr name, PierStr request, void* ctx, PierStrSink reply);
```

Call `name` with `request`; the provider's answer arrives through `reply`. Returns one of PIER\_SERVICE\_\*. A mod cannot call its own service (it has a direct function call, and self-calls are the least legible loop shape).

- Call: `api->service_call(mod, name, request, ctx, reply)`
- Parameters:
    - mod : `PierModHandle`
    - name : `PierStr`
    - request : `PierStr`
    - ctx : `void*`
    - reply : `PierStrSink`
- Return type: `int32_t`
- Section of abi.h: Cross-mod service registry (query-style calls)
- Position in the table: slot 165, counting from 0
- Callers in each binding:
    - Rust: [`service::call`](../rust/service.md#fn.call), [`service::call_json`](../rust/service.md#fn.call_json), [`service::call_with`](../rust/service.md#fn.call_with), [`service::call_optional`](../rust/service.md#fn.call_optional)
    - Go: [`CallService`](../go/crossmod.md#CallService), [`CallServiceOptional`](../go/crossmod.md#CallServiceOptional), [`Raw.ServiceCall`](../go/raw.md#Raw.ServiceCall)
    - Zig: [`callService`](../zig/crossmod.md#callService)

### `service_list` {#service_list}

```c
void (*service_list)(void* ctx, PierStrSink sink);
```

Every registered service as a JSON array of `{"name":…,"mod":…}`. For diagnostics and for a caller deciding whether to build a request nobody can answer.

- Call: `api->service_list(ctx, sink)`
- Parameters:
    - ctx : `void*`
    - sink : `PierStrSink`
- Section of abi.h: Cross-mod service registry (query-style calls)
- Position in the table: slot 166, counting from 0
- Callers in each binding:
    - Rust: [`service::exists`](../rust/service.md#fn.exists), [`service::list_json`](../rust/service.md#fn.list_json), [`service::list`](../rust/service.md#fn.list)
    - Go: [`ListServices`](../go/crossmod.md#ListServices), [`ServiceExists`](../go/crossmod.md#ServiceExists), [`Raw.ServiceList`](../go/raw.md#Raw.ServiceList)
    - Zig: [`listServicesJson`](../zig/crossmod.md#listServicesJson)

### `lane_publish` {#lane_publish}

```c
uint64_t (*lane_publish)(PierModHandle mod, PierStr name, PierLaneDesc const* desc);
```

Publish a lane. Exclusive, same discipline as `service_register`: if the name is taken, return 0 and name the incumbent in the log. Returns a publish id (&gt; 0), withdrawn automatically at unload.

- Call: `api->lane_publish(mod, name, desc)`
- Parameters:
    - mod : `PierModHandle`
    - name : `PierStr`
    - desc : `PierLaneDesc const*`
- Return type: `uint64_t`
- Section of abi.h: Same-toolchain fast lane, appended and struct\_size-gated.
- Position in the table: slot 172, counting from 0
- Callers in each binding:
    - Rust: [`lane::publish`](../rust/lane.md#fn.publish)

### `lane_unpublish` {#lane_unpublish}

```c
bool (*lane_unpublish)(PierModHandle mod, uint64_t pub_id);
```

Withdraw a lane owned by this mod. Calls release for every outstanding lease and clears the liveness flag, so a consumer's next check sees the lane gone instead of jumping through a dead pointer.

- Call: `api->lane_unpublish(mod, pub_id)`
- Parameters:
    - mod : `PierModHandle`
    - pub_id : `uint64_t`
- Return type: `bool`
- Section of abi.h: Same-toolchain fast lane, appended and struct\_size-gated.
- Position in the table: slot 173, counting from 0
- Callers in each binding:
    - Go: [`Raw.LaneUnpublish`](../go/raw.md#Raw.LaneUnpublish)

### `lane_acquire` {#lane_acquire}

```c
int32_t (*lane_acquire)(
    PierModHandle mod, PierStr name, uint64_t want_fingerprint, PierLaneRef* out);
```

Acquire a lane. `want_fingerprint` must be the value the consumer computed itself.

0 is not a valid fingerprint and always yields `PIER_LANE_FINGERPRINT`. It must not mean "skip the check": this call hands over raw vtable and data pointers, which the consumer then calls through its own table offsets, so skipping the check is type confusion. To inspect which lanes exist and what their fingerprints are, use `lane_list`, which hands over no pointers at all.

Returns a PIER\_LANE\_\* value. After a successful acquire, `lane_release` is mandatory, or the provider's state stays retained.

- Call: `api->lane_acquire(mod, name, want_fingerprint, out)`
- Parameters:
    - mod : `PierModHandle`
    - name : `PierStr`
    - want_fingerprint : `uint64_t`
    - out : `PierLaneRef*`
- Return type: `int32_t`
- Section of abi.h: Same-toolchain fast lane, appended and struct\_size-gated.
- Position in the table: slot 174, counting from 0
- Callers in each binding:
    - Rust: [`lane::acquire`](../rust/lane.md#fn.acquire), [`lane::acquire`](../rust/lane.md#fn.acquire)

### `lane_release` {#lane_release}

```c
bool (*lane_release)(PierModHandle mod, uint64_t lease);
```

Return a lease. Only a lease held by this mod. Returns false when the provider is already gone, since the loader has called release for it by then and calling again would be a double free.

- Call: `api->lane_release(mod, lease)`
- Parameters:
    - mod : `PierModHandle`
    - lease : `uint64_t`
- Return type: `bool`
- Section of abi.h: Same-toolchain fast lane, appended and struct\_size-gated.
- Position in the table: slot 175, counting from 0
- Callers in each binding:
    - Go: [`Raw.LaneRelease`](../go/raw.md#Raw.LaneRelease)

### `lane_list` {#lane_list}

```c
void (*lane_list)(void* ctx, PierStrSink sink);
```

Every lane, as a JSON array: \[{"name":…,"mod":…,"fingerprint":"0x…","protocol":1,"leases":N,"alive":true}\]

- Call: `api->lane_list(ctx, sink)`
- Parameters:
    - ctx : `void*`
    - sink : `PierStrSink`
- Section of abi.h: Same-toolchain fast lane, appended and struct\_size-gated.
- Position in the table: slot 176, counting from 0
- Callers in each binding:
    - Rust: [`lane::list`](../rust/lane.md#fn.list), [`lane::list`](../rust/lane.md#fn.list), [`lane::list_json`](../rust/lane.md#fn.list_json), [`lane::list_json`](../rust/lane.md#fn.list_json)
    - Go: [`Raw.LaneList`](../go/raw.md#Raw.LaneList)

### `service_caller` {#service_caller}

```c
void (*service_caller)(void* ctx, PierStrSink sink);
```

Who is calling the service callback that is running right now.

A service callback receives a request and nothing about its sender, so a provider that keys anything on a name inside the request (an owner, an acting player) is trusting the request to tell the truth. This slot lets the provider ask the host instead: inside a PierServiceCb, the sink receives the manifest name of the mod whose `service_call` is on the stack. Calls nest, and the innermost one is reported.

Outside a callback, or when the call came without a mod handle, the sink is not called at all; a provider then knows it cannot attribute the request rather than attributing it to an empty name. Reads a thread-local, so any thread; a callback that hops threads loses the attribution, which is the correct answer there.

- Call: `api->service_caller(ctx, sink)`
- Parameters:
    - ctx : `void*`
    - sink : `PierStrSink`
- Section of abi.h: Appended: bulk block reads and writes
- Position in the table: slot 197, counting from 0
- Callers in each binding:
    - Rust: [`service::caller`](../rust/service.md#fn.caller)
    - Go: [`ServiceCaller`](../go/crossmod.md#ServiceCaller), [`Raw.ServiceCaller`](../go/raw.md#Raw.ServiceCaller)
    - Zig: [`serviceCaller`](../zig/crossmod.md#serviceCaller)

## Macros {#macros}

### `PIER_LANE_PROTOCOL` {#PIER_LANE_PROTOCOL}

```c
#define PIER_LANE_PROTOCOL 1u
```

Lane protocol version. Kept separate from `PIER_ABI_VERSION`: the lane shape evolves independently, and a mismatch is handled differently (reject this one lane, not the whole mod).

## Types {#types}

### `PierBusCb` {#PierBusCb}

```c
typedef bool (*PierBusCb)(void* user, PierStr topic, PierStr payload);
```

Subscriber callback. `topic` and `payload` are borrowed for the duration of the call. Anything kept past it must be copied.

The return value is a veto, and only for `bus_publish_vetoable`:

```text
true  = "refuse this",
false = "no opinion".
```

It is ignored entirely by `bus_publish`. There is deliberately no way to turn a refusal back into an approval: a subscriber can only tighten, never loosen. Letting one mod override another's refusal means the \*last\* subscriber to run decides, and subscriber order is not something either mod controls.

Called on the thread that published. Never called after the owning mod is unloaded or while it is disabled.

### `PierServiceCb` {#PierServiceCb}

```c
typedef bool (*PierServiceCb)(
    void* user, PierStr name, PierStr request, void* ctx, PierStrSink reply);
```

Provider callback for the cross-mod service registry (query-style calls, as opposed to the bus's one-way broadcast).

Write the answer through `reply(ctx, ...)` — exactly once — and return true. Return false to report failure; anything written first is handed to the caller as the error text, which is what makes "no such plot" and "the database is down" distinguishable at the call site.

`request` and `reply` are opaque UTF-8 the two mods agree on out of band. The loader never looks inside either.

Runs synchronously on the CALLING thread, inside `service_call`. Never called after the providing mod is unloaded. It IS still called while the provider is merely disabled: `service_call` deliberately does not consult isEnabled() (see Services.cpp) because LeviLamina enables mods only after every `on_load` has run, and a service that is unreachable during that window breaks every consumer that resolves it in its own `on_load`.

### `PierLaneRefFn` {#PierLaneRefFn}

```c
typedef void (*PierLaneRefFn)(void* data);
```

Reference-count hooks, executed inside the provider's own dylib.

The loader calls them from `lane_acquire` and `lane_release`, and calls release for every outstanding lease when the provider unloads or calls `lane_unpublish`. Publishing itself does not hold a count: the loader never calls release for the data handed to `lane_publish`, and the provider reclaims that itself after unpublishing.

These must not call back into the loader (any lane\_\* slot); that self- deadlocks. A typical implementation is one atomic increment or decrement on the provider's own refcount, touching no lock.

### `PierLaneDesc` {#PierLaneDesc}

```c
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
```

How a provider describes a lane when publishing it. Every field is filled in by the provider; the loader only carries it.

### `PierLaneRef` {#PierLaneRef}

```c
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
     * does not reach here, the loader does not write it, and its behavior is
     * unchanged.
     */
    uint32_t* busy;
} PierLaneRef;
```

What `lane_acquire` produces.

## `PIER_SERVICE_*` {#PIER_SERVICE_OK-group}

`service_call` return codes.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_SERVICE_OK"></span>`PIER_SERVICE_OK` | `0` | provider ran and wrote a reply |
| <span id="PIER_SERVICE_NOT_FOUND"></span>`PIER_SERVICE_NOT_FOUND` | `1` | nobody provides this name (or is disabled/unloaded) |
| <span id="PIER_SERVICE_ERROR"></span>`PIER_SERVICE_ERROR` | `2` | provider returned false; reply holds its message |
| <span id="PIER_SERVICE_REFUSED"></span>`PIER_SERVICE_REFUSED` | `3` | bad name, self-call, or call-depth limit |

## `PIER_LANE_*` {#PIER_LANE_OK-group}

`lane_acquire` return values.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_LANE_OK"></span>`PIER_LANE_OK` | `0` | acquired; out is filled in |
| <span id="PIER_LANE_NOT_FOUND"></span>`PIER_LANE_NOT_FOUND` | `1` | nobody published this name (that mod is not installed) |
| <span id="PIER_LANE_FINGERPRINT"></span>`PIER_LANE_FINGERPRINT` | `2` | published, but the fingerprint differs; degrade, hand over nothing |
| <span id="PIER_LANE_REFUSED"></span>`PIER_LANE_REFUSED` | `3` | bad name, self-acquire, provider disabled, or protocol mismatch |

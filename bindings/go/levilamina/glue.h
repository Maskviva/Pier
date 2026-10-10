/* glue.h: the C side of the Go binding that is not generated. The host calls C function
 * pointers and Go can neither be one nor call one, so each callback the host sees is an
 * exported Go function reached through a getter here, and a sink is called through
 * piergo_sink. */
#ifndef PIERGO_GLUE_H
#define PIERGO_GLUE_H

#include <stdint.h>

#include "sdk/abi.h"

/** Fills the vtable with the three lifecycle callbacks and this build's target flag. */
void piergo_fill_vtable(PierModVTable* out, uint32_t mod_flags);

PierTaskCb piergo_task_cb(void);
PierEventCb piergo_event_cb(void);
PierCommandCb piergo_command_cb(void);
PierStrSink piergo_str_sink(void);
PierCmdOutputSink piergo_cmd_output_sink(void);
PierBlockSink piergo_block_sink(void);
PierActorSink piergo_actor_sink(void);
PierSlotSink piergo_slot_sink(void);
PierKvSink piergo_kv_sink(void);
PierBytesSink piergo_bytes_sink(void);
PierFormResultCb piergo_form_cb(void);
PierBusCb piergo_bus_cb(void);
PierServiceCb piergo_service_cb(void);
PierMoneyCb piergo_money_before_cb(void);
PierMoneyCb piergo_money_after_cb(void);
PierPacketCb piergo_packet_cb(void);
PierConnCb piergo_conn_cb(void);
PierEntitySink piergo_entity_sink(void);
PierPaletteSink piergo_palette_sink(void);
PierCellSink piergo_cell_sink(void);
PierKeyCb piergo_key_cb(void);
PierGenerateChunkFn piergo_generate_fn(void);

/** Calls a byte sink the host handed over. */
void piergo_bytes(PierBytesSink sink, void* ctx, const uint8_t* data, size_t len);

/** Calls a sink the host handed over; Go cannot call a function pointer itself. */
void piergo_sink(PierStrSink sink, void* ctx, PierStr s);

/** A runtime/cgo handle as the `void* user` the host carries back unread. Converting in C
 *  keeps the uintptr-to-pointer step out of Go, where vet rightly distrusts it. */
void* piergo_handle(uintptr_t h);

#endif /* PIERGO_GLUE_H */

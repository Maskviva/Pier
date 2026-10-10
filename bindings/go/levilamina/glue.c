/* glue.c: the getters and helpers glue.h declares, forwarding to the exported Go functions
 * that _cgo_export.h declares. */
#include "_cgo_export.h"
#include "glue.h"

void piergo_fill_vtable(PierModVTable* out, uint32_t mod_flags)
{
    out->struct_size = sizeof(PierModVTable);
    out->abi_version = PIER_ABI_VERSION;
    out->mod_flags = mod_flags;
    out->_reserved0 = 0;
    out->instance = NULL;
    out->on_enable = piergoOnEnable;
    out->on_disable = piergoOnDisable;
    out->on_unload = piergoOnUnload;
}

PierTaskCb piergo_task_cb(void) { return piergoTask; }
PierEventCb piergo_event_cb(void) { return piergoEvent; }
PierCommandCb piergo_command_cb(void) { return piergoCommand; }
PierStrSink piergo_str_sink(void) { return piergoStrSink; }
PierCmdOutputSink piergo_cmd_output_sink(void) { return piergoCmdOutput; }
PierBlockSink piergo_block_sink(void) { return piergoBlockSink; }
PierActorSink piergo_actor_sink(void) { return piergoActorSink; }
PierSlotSink piergo_slot_sink(void) { return piergoSlotSink; }
PierKvSink piergo_kv_sink(void) { return piergoKvSink; }
/* cgo writes the exported prototype without const, so the pointer type differs by that
 * qualifier alone; the cast is the whole difference and every ABI passes both alike. */
PierBytesSink piergo_bytes_sink(void) { return (PierBytesSink)piergoBytesSink; }
PierFormResultCb piergo_form_cb(void) { return piergoFormResult; }
PierBusCb piergo_bus_cb(void) { return piergoBus; }
PierServiceCb piergo_service_cb(void) { return piergoService; }
PierMoneyCb piergo_money_before_cb(void) { return piergoMoneyBefore; }
PierMoneyCb piergo_money_after_cb(void) { return piergoMoneyAfter; }
/* The two below differ from their exported prototypes by a const on a struct pointer
 * parameter only, as piergo_bytes_sink does. */
PierPacketCb piergo_packet_cb(void) { return (PierPacketCb)piergoPacket; }
PierConnCb piergo_conn_cb(void) { return piergoConn; }
PierEntitySink piergo_entity_sink(void) { return piergoEntitySink; }
PierPaletteSink piergo_palette_sink(void) { return piergoPaletteSink; }
PierCellSink piergo_cell_sink(void) { return piergoCellSink; }
PierKeyCb piergo_key_cb(void) { return piergoKey; }
PierGenerateChunkFn piergo_generate_fn(void) { return (PierGenerateChunkFn)piergoGenerate; }

void piergo_bytes(PierBytesSink sink, void* ctx, const uint8_t* data, size_t len)
{
    if (sink != NULL) sink(ctx, data, len);
}

void piergo_sink(PierStrSink sink, void* ctx, PierStr s)
{
    if (sink != NULL) sink(ctx, s);
}

void* piergo_handle(uintptr_t h) { return (void*)h; }

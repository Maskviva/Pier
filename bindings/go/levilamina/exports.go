package levilamina

// Every function the host calls. A file with //export may hold only declarations in its
// preamble, so the gates and calls of slots_gen.h are used from the other files.

/*
#include "sdk/abi.h"
#include "glue.h"
*/
import "C"

import (
	"runtime/cgo"
	"unsafe"
)

//export pier_main
func pier_main(a *C.PierApi, s C.PierModHandle, out *C.PierModVTable) C.bool {
	return C.bool(enter(a, s, out))
}

//export piergoOnEnable
func piergoOnEnable(instance unsafe.Pointer) C.bool {
	return C.bool(runStage("on_enable", func(c *Context) error { return current.Enable(c) }))
}

//export piergoOnDisable
func piergoOnDisable(instance unsafe.Pointer) C.bool {
	return C.bool(runStage("on_disable", func(c *Context) error { return current.Disable(c) }))
}

//export piergoOnUnload
func piergoOnUnload(instance unsafe.Pointer) C.bool {
	u, isUnloader := current.(Unloader)
	if !isUnloader {
		return C.bool(true)
	}
	return C.bool(runStage("on_unload", u.Unload))
}

//export piergoTask
func piergoTask(user unsafe.Pointer) {
	var ok bool
	defer recoverTo(&ok, "a scheduled task")
	// Deleted before the task runs and inside the recover: Delete panics on a handle that
	// is not valid, and a panic deferred past the recover would unwind into the host.
	h := cgo.Handle(uintptr(user))
	fn, isFunc := h.Value().(func())
	h.Delete()
	if isFunc {
		fn()
	}
}

//export piergoEvent
func piergoEvent(user unsafe.Pointer, id C.PierStr, snbt C.PierStr, writeCtx unsafe.Pointer, writeBack C.PierStrSink) {
	var ok bool
	defer recoverTo(&ok, "an event callback")
	fn, isFunc := cgo.Handle(uintptr(user)).Value().(func(*Event))
	if !isFunc {
		return
	}
	ev := &Event{ID: goString(id), SNBT: goString(snbt)}
	fn(ev)
	if ev.edits == nil {
		return
	}
	// Only the keys set go back; the host merges them into the event, so two mods writing
	// different keys do not erase each other.
	text, err := ev.edits.SNBT()
	if err != nil {
		logAt(levelError, "an event edit could not be written back: "+err.Error())
		return
	}
	C.piergo_sink(writeBack, writeCtx, str(text))
}

//export piergoCommand
func piergoCommand(user unsafe.Pointer, args C.PierStr, origin C.PierStr, ctx unsafe.Pointer, ok C.PierStrSink, fail C.PierStrSink) {
	var done bool
	defer recoverTo(&done, "a command callback")
	fn, isFunc := cgo.Handle(uintptr(user)).Value().(func(*Invocation))
	if !isFunc {
		return
	}
	fn(&Invocation{Args: goString(args), Origin: goString(origin), ctx: ctx, ok: ok, fail: fail})
}

//export piergoStrSink
func piergoStrSink(ctx unsafe.Pointer, s C.PierStr) {
	var ok bool
	defer recoverTo(&ok, "a string sink")
	if c, isCollector := cgo.Handle(uintptr(ctx)).Value().(*collector); isCollector {
		c.items = append(c.items, goString(s))
	}
}

//export piergoCmdOutput
func piergoCmdOutput(ctx unsafe.Pointer, success C.bool, output C.PierStr) {
	var ok bool
	defer recoverTo(&ok, "a command output sink")
	if c, isC := cgo.Handle(uintptr(ctx)).Value().(*outputCollector); isC {
		c.lines = append(c.lines, CommandOutput{Success: bool(success), Text: goString(output)})
	}
}

//export piergoBlockSink
func piergoBlockSink(ctx unsafe.Pointer, x, y, z C.int32_t, name, snbt C.PierStr) {
	var ok bool
	defer recoverTo(&ok, "a block sink")
	if c, isC := cgo.Handle(uintptr(ctx)).Value().(interface{ addBlock(BlockInfo) }); isC {
		c.addBlock(BlockInfo{X: int32(x), Y: int32(y), Z: int32(z), Name: goString(name), SNBT: goString(snbt)})
	}
}

//export piergoEntitySink
func piergoEntitySink(ctx unsafe.Pointer, x, y, z C.int32_t, typeName, snbt C.PierStr) {
	var ok bool
	defer recoverTo(&ok, "an entity sink")
	if c, isC := cgo.Handle(uintptr(ctx)).Value().(interface{ addEntity(EntityInfo) }); isC {
		c.addEntity(EntityInfo{X: int32(x), Y: int32(y), Z: int32(z), Type: goString(typeName), SNBT: goString(snbt)})
	}
}

//export piergoPaletteSink
func piergoPaletteSink(ctx unsafe.Pointer, index C.uint32_t, name, snbt C.PierStr) {
	var ok bool
	defer recoverTo(&ok, "a palette sink")
	if c, isC := cgo.Handle(uintptr(ctx)).Value().(*indexedCollector); isC {
		c.palette = append(c.palette, PaletteEntry{Index: uint32(index), Name: goString(name), SNBT: goString(snbt)})
	}
}

//export piergoCellSink
func piergoCellSink(ctx unsafe.Pointer, x, y, z C.int32_t, index C.uint32_t) {
	var ok bool
	defer recoverTo(&ok, "a cell sink")
	if c, isC := cgo.Handle(uintptr(ctx)).Value().(*indexedCollector); isC {
		c.cells = append(c.cells, BlockCell{X: int32(x), Y: int32(y), Z: int32(z), Index: uint32(index)})
	}
}

//export piergoPacket
func piergoPacket(user unsafe.Pointer, ev *C.PierPacketEvent, edit *C.PierPacketEdit, replaceCtx unsafe.Pointer, replace C.PierBytesSink) C.int32_t {
	// A panic forwards the packet unchanged: PASS is the zero value the host also reads
	// for anything it does not know.
	verdict := C.int32_t(0)
	var ok bool
	defer recoverTo(&ok, "a packet hook")
	fn, isFunc := cgo.Handle(uintptr(user)).Value().(func(*PacketCall) PacketVerdict)
	if !isFunc || ev == nil {
		return verdict
	}
	call := &PacketCall{edit: edit, rctx: replaceCtx, rsink: replace, Event: PacketEvent{
		Direction:   int32(ev.direction),
		ConnID:      uint64(ev.conn_id),
		Address:     goString(ev.address),
		PacketID:    int32(ev.packet_id),
		SenderSubID: uint8(ev.sender_sub_id),
		TargetSubID: uint8(ev.target_sub_id),
	}}
	if ev.body != nil && ev.body_len > 0 {
		call.Event.Body = C.GoBytes(unsafe.Pointer(ev.body), C.int(ev.body_len))
	}
	verdict = C.int32_t(fn(call))
	return verdict
}

//export piergoConn
func piergoConn(user unsafe.Pointer, connID C.uint64_t, address C.PierStr, opened C.bool) {
	var ok bool
	defer recoverTo(&ok, "a connection hook")
	if fn, isFunc := cgo.Handle(uintptr(user)).Value().(func(uint64, string, bool)); isFunc {
		fn(uint64(connID), goString(address), bool(opened))
	}
}

//export piergoKey
func piergoKey(user unsafe.Pointer, action C.PierKeyAction, impact C.PierFocusImpact) {
	var ok bool
	defer recoverTo(&ok, "a key binding")
	if fn, isFunc := cgo.Handle(uintptr(user)).Value().(func(bool, int32)); isFunc {
		fn(action == 1, int32(impact))
	}
}

//export piergoGenerate
func piergoGenerate(user unsafe.Pointer, req *C.PierChunkRequest) C.int32_t {
	// Runs on chunk worker threads, several at once. A panic leaves the chunk unfilled; the
	// host writes air and says so.
	var ok bool
	defer recoverTo(&ok, "a terrain generator")
	fill, isFunc := cgo.Handle(uintptr(user)).Value().(func(*ChunkRequest) bool)
	if !isFunc || req == nil {
		return 0
	}
	height := int(req.height)
	r := &ChunkRequest{Dim: int32(req.dim_id), ChunkX: int32(req.chunk_x), ChunkZ: int32(req.chunk_z),
		MinY: int32(req.min_y), Height: int32(height)}
	if req.out_materials != nil && height > 0 {
		r.Materials = unsafe.Slice((*uint16)(unsafe.Pointer(req.out_materials)), 256*height)
	}
	if req.out_biomes != nil {
		r.Biomes = unsafe.Slice((*uint16)(unsafe.Pointer(req.out_biomes)), 256)
	}
	if !fill(r) {
		return 0
	}
	ok = true
	return 1
}

//export piergoActorSink
func piergoActorSink(ctx unsafe.Pointer, id C.PierActorId, typeName C.PierStr) {
	var ok bool
	defer recoverTo(&ok, "an actor sink")
	if c, isC := cgo.Handle(uintptr(ctx)).Value().(*actorCollector); isC {
		c.actors = append(c.actors, ActorInfo{ID: ActorID(id), Type: goString(typeName)})
	}
}

//export piergoSlotSink
func piergoSlotSink(ctx unsafe.Pointer, slot C.int32_t, item C.PierStr) {
	var ok bool
	defer recoverTo(&ok, "a slot sink")
	if c, isC := cgo.Handle(uintptr(ctx)).Value().(*slotCollector); isC {
		c.slots = append(c.slots, SlotItem{Slot: int32(slot), SNBT: goString(item)})
	}
}

//export piergoKvSink
func piergoKvSink(ctx unsafe.Pointer, key, value C.PierStr) {
	var ok bool
	defer recoverTo(&ok, "a key-value sink")
	if c, isC := cgo.Handle(uintptr(ctx)).Value().(*kvCollector); isC {
		c.pairs = append(c.pairs, KeyValue{Key: goString(key), Value: goString(value)})
	}
}

//export piergoBytesSink
func piergoBytesSink(ctx unsafe.Pointer, data *C.uint8_t, n C.size_t) {
	var ok bool
	defer recoverTo(&ok, "a byte sink")
	if c, isC := cgo.Handle(uintptr(ctx)).Value().(*bytesCollector); isC && data != nil {
		c.data = append(c.data, C.GoBytes(unsafe.Pointer(data), C.int(n))...)
	}
}

//export piergoFormResult
func piergoFormResult(user unsafe.Pointer, result C.PierStr) {
	var ok bool
	defer recoverTo(&ok, "a form callback")
	// The host calls it once, so the handle goes with this call.
	h := cgo.Handle(uintptr(user))
	fn, isFunc := h.Value().(func(string))
	h.Delete()
	if isFunc {
		fn(goString(result))
	}
}

//export piergoBus
func piergoBus(user unsafe.Pointer, topic, payload C.PierStr) C.bool {
	// A panic is no veto: a veto is the stronger act and a bug should not cast one, as in
	// the Rust binding. recoverTo leaves veto false.
	veto := false
	defer recoverTo(&veto, "a bus subscriber")
	if fn, isFunc := cgo.Handle(uintptr(user)).Value().(func(string, string) bool); isFunc {
		veto = fn(goString(topic), goString(payload))
	}
	return C.bool(veto)
}

//export piergoService
func piergoService(user unsafe.Pointer, name, request C.PierStr, ctx unsafe.Pointer, reply C.PierStrSink) C.bool {
	var ok bool
	defer recoverTo(&ok, "a service provider")
	fn, isFunc := cgo.Handle(uintptr(user)).Value().(func(string) (string, error))
	if !isFunc {
		return C.bool(false)
	}
	answer, err := callProvider(fn, goString(request))
	if err != nil {
		// What is written before a false is the caller's error text, which is how it tells
		// "no such plot" from "the database is down".
		C.piergo_sink(reply, ctx, str(err.Error()))
		return C.bool(false)
	}
	C.piergo_sink(reply, ctx, str(answer))
	ok = true
	return C.bool(ok)
}

//export piergoMoneyBefore
func piergoMoneyBefore(kind C.PierMoneyEvent, from, to C.PierStr, value C.int64_t) C.bool {
	allow := true
	defer recoverTo(&allow, "an economy listener")
	ev := MoneyEvent{Kind: MoneyKind(kind), From: goString(from), To: goString(to), Value: int64(value)}
	for _, fn := range moneyListeners(true) {
		if !fn(ev) {
			allow = false
		}
	}
	return C.bool(allow)
}

//export piergoMoneyAfter
func piergoMoneyAfter(kind C.PierMoneyEvent, from, to C.PierStr, value C.int64_t) C.bool {
	var ok bool
	defer recoverTo(&ok, "an economy listener")
	ev := MoneyEvent{Kind: MoneyKind(kind), From: goString(from), To: goString(to), Value: int64(value)}
	for _, fn := range moneyListeners(false) {
		fn(ev)
	}
	return C.bool(true)
}

// collector gathers what a sink receives during one call.
type collector struct {
	items []string
}

package levilamina

// The slots whose callbacks run off the server thread or hand over structures: packet and
// connection hooks, region scans, bulk block writes, client key bindings and supplied
// terrain.

/*
#include <stdlib.h>
#include "sdk/abi.h"
#include "slots_gen.h"
#include "glue.h"
*/
import "C"

import (
	"errors"
	"fmt"
	"runtime/cgo"
	"strings"
	"unsafe"
)

// PacketVerdict is what a packet hook decides about one packet.
type PacketVerdict int32

// The packet verdicts. Anything else is read as PacketPass.
const (
	PacketPass    PacketVerdict = 0
	PacketReplace PacketVerdict = 1
	PacketDrop    PacketVerdict = 2
)

// PacketEvent is one packet a hook sees. Body is a copy.
type PacketEvent struct {
	Direction   int32
	ConnID      uint64
	Address     string
	PacketID    int32
	SenderSubID uint8
	TargetSubID uint8
	Body        []byte
}

// PacketCall is one packet inside a hook, with the ways to change it.
type PacketCall struct {
	Event PacketEvent
	edit  *C.PierPacketEdit
	rctx  unsafe.Pointer
	rsink C.PierBytesSink
}

// SetPacketID makes the forwarded packet carry another id.
func (c *PacketCall) SetPacketID(id int32) {
	if c.edit != nil {
		c.edit.packet_id = C.int32_t(id)
	}
}

// SetSubIDs changes the sender and target sub-client ids of the forwarded packet.
func (c *PacketCall) SetSubIDs(sender, target uint8) {
	if c.edit != nil {
		c.edit.sender_sub_id = C.uint8_t(sender)
		c.edit.target_sub_id = C.uint8_t(target)
	}
}

// Replace hands over the body to forward; it takes effect when the hook returns
// PacketReplace.
func (c *PacketCall) Replace(body []byte) {
	C.piergo_bytes(c.rsink, c.rctx, bytesPtr(body), C.size_t(len(body)))
}

// PacketHook is a registered packet or connection hook, kept to remove it.
type PacketHook struct {
	h    PacketHookHandle
	fn   cgo.Handle
	conn bool
}

// RegisterPacketHook calls fn for every packet in the directions of dirMask, the
// PIER_PKT_MASK_* bits of abi.h: 1 for inbound, 2 for outbound. fn runs where the host pumps
// the connection or sends the packet, which is usually the server thread and not always,
// since a flush can be asynchronous; fn keeps to its own state and reaches the world through
// Schedule.
func RegisterPacketHook(dirMask int32, fn func(*PacketCall) PacketVerdict) (*PacketHook, error) {
	if api == nil || !bool(C.piergo_has_packet_hook_register(api)) {
		return nil, notProvided("packet_hook_register")
	}
	h := cgo.NewHandle(fn)
	ph := C.piergo_call_packet_hook_register(api, self, C.int32_t(dirMask), C.piergo_packet_cb(), handle(uintptr(h)))
	if ph == nil {
		h.Delete()
		return nil, errors.New("the packet hook was refused")
	}
	return &PacketHook{h: PacketHookHandle{p: unsafe.Pointer(ph)}, fn: h}, nil
}

// RegisterPacketHookIDs is RegisterPacketHook for the listed packet ids only: a packet whose
// id no hook listed is passed on before fn runs or any lock is taken.
func RegisterPacketHookIDs(dirMask int32, ids []int32, fn func(*PacketCall) PacketVerdict) (*PacketHook, error) {
	if len(ids) == 0 {
		return nil, errors.New("no packet id was given; RegisterPacketHook hooks them all")
	}
	if api == nil || !bool(C.piergo_has_packet_hook_register_ids(api)) {
		return nil, notProvided("packet_hook_register_ids")
	}
	h := cgo.NewHandle(fn)
	ph := C.piergo_call_packet_hook_register_ids(api, self, C.int32_t(dirMask), int32sPtr(ids), C.size_t(len(ids)),
		C.piergo_packet_cb(), handle(uintptr(h)))
	if ph == nil {
		h.Delete()
		return nil, errors.New("the packet hook was refused")
	}
	return &PacketHook{h: PacketHookHandle{p: unsafe.Pointer(ph)}, fn: h}, nil
}

// RegisterConnHook calls fn when a connection opens and when it closes, with the threading
// RegisterPacketHook describes.
func RegisterConnHook(fn func(connID uint64, address string, opened bool)) (*PacketHook, error) {
	if api == nil || !bool(C.piergo_has_packet_conn_hook_register(api)) {
		return nil, notProvided("packet_conn_hook_register")
	}
	h := cgo.NewHandle(fn)
	ph := C.piergo_call_packet_conn_hook_register(api, self, C.piergo_conn_cb(), handle(uintptr(h)))
	if ph == nil {
		h.Delete()
		return nil, errors.New("the connection hook was refused")
	}
	return &PacketHook{h: PacketHookHandle{p: unsafe.Pointer(ph)}, fn: h, conn: true}, nil
}

// Unregister removes the hook. Calling it twice is harmless.
func (p *PacketHook) Unregister() error {
	if p == nil || p.h.IsZero() {
		return nil
	}
	var known bool
	var err error
	if p.conn {
		known, err = Raw.PacketConnHookUnregister(p.h)
	} else {
		known, err = Raw.PacketHookUnregister(p.h)
	}
	if err != nil {
		return err
	}
	p.h = PacketHookHandle{}
	p.fn.Delete()
	if !known {
		return errors.New("the host did not know this hook")
	}
	return nil
}

// EntityInfo is one entity a region scan found: the cell holding it, its type and its SNBT.
type EntityInfo struct {
	X, Y, Z int32
	Type    string
	SNBT    string
}

type scanCollector struct {
	blocks   []BlockInfo
	entities []EntityInfo
}

func (c *scanCollector) addBlock(b BlockInfo) { c.blocks = append(c.blocks, b) }

func (c *scanCollector) addEntity(e EntityInfo) { c.entities = append(c.entities, e) }

// ScanRegion reads every block and entity in a box. Server thread only.
func ScanRegion(dim, x1, y1, z1, x2, y2, z2 int32) ([]BlockInfo, []EntityInfo, error) {
	if api == nil || !bool(C.piergo_has_scan_region(api)) {
		return nil, nil, notProvided("scan_region")
	}
	col := &scanCollector{}
	h := cgo.NewHandle(col)
	defer h.Delete()
	if !bool(C.piergo_call_scan_region(api, C.int32_t(dim), C.int32_t(x1), C.int32_t(y1), C.int32_t(z1),
		C.int32_t(x2), C.int32_t(y2), C.int32_t(z2), handle(uintptr(h)), C.piergo_block_sink(), C.piergo_entity_sink())) {
		return nil, nil, errors.New("the region could not be scanned")
	}
	return col.blocks, col.entities, nil
}

// PaletteEntry is one distinct block state of an indexed scan.
type PaletteEntry struct {
	Index uint32
	Name  string
	SNBT  string
}

// BlockCell is one cell and an index into a palette, for ScanRegionIndexed and SetBlocks.
type BlockCell struct {
	X, Y, Z int32
	Index   uint32
}

type indexedCollector struct {
	palette []PaletteEntry
	cells   []BlockCell
}

// ScanRegionIndexed reads a box as a palette of distinct block states and the cells that
// index it, which is far smaller than ScanRegion for a large box. Server thread only.
func ScanRegionIndexed(dim, x1, y1, z1, x2, y2, z2 int32) ([]PaletteEntry, []BlockCell, error) {
	if api == nil || !bool(C.piergo_has_scan_region_indexed(api)) {
		return nil, nil, notProvided("scan_region_indexed")
	}
	col := &indexedCollector{}
	h := cgo.NewHandle(col)
	defer h.Delete()
	if !bool(C.piergo_call_scan_region_indexed(api, C.int32_t(dim), C.int32_t(x1), C.int32_t(y1), C.int32_t(z1),
		C.int32_t(x2), C.int32_t(y2), C.int32_t(z2), handle(uintptr(h)), C.piergo_palette_sink(), C.piergo_cell_sink())) {
		return nil, nil, errors.New("the region could not be scanned")
	}
	return col.palette, col.cells, nil
}

// SetBlocks writes many blocks in one call: palette holds block specs and each cell names one
// by index. It returns how many cells changed. Server thread only.
func SetBlocks(dim int32, palette []string, cells []BlockCell, updateFlags int32) (int64, error) {
	if api == nil || !bool(C.piergo_has_edit_set_blocks(api)) {
		return 0, notProvided("edit_set_blocks")
	}
	if len(palette) == 0 || len(cells) == 0 {
		return 0, nil
	}
	// The palette is an array of views, and an array holding pointers may not be Go memory
	// when C reads it; the cells hold none and are passed as they are.
	pal := unsafe.Slice((*C.PierStr)(C.malloc(C.size_t(len(palette))*C.size_t(unsafe.Sizeof(C.PierStr{})))), len(palette))
	defer C.free(unsafe.Pointer(&pal[0]))
	owned := make([]*C.char, 0, len(palette))
	defer func() {
		for _, p := range owned {
			C.free(unsafe.Pointer(p))
		}
	}()
	for i, s := range palette {
		cs := C.CString(s)
		owned = append(owned, cs)
		pal[i] = C.PierStr{ptr: cs, len: C.size_t(len(s))}
	}
	cc := make([]C.PierBlockCell, len(cells))
	for i, c := range cells {
		cc[i] = C.PierBlockCell{x: C.int32_t(c.X), y: C.int32_t(c.Y), z: C.int32_t(c.Z), index: C.uint32_t(c.Index)}
	}
	n := int64(C.piergo_call_edit_set_blocks(api, C.int32_t(dim), &pal[0], C.uint32_t(len(palette)), &cc[0],
		C.size_t(len(cc)), C.int32_t(updateFlags)))
	if n < 0 {
		return 0, refused("the bulk block write")
	}
	return n, nil
}

// KeyBinding is a registered client key binding.
type KeyBinding struct {
	h  KeyHandle
	fn cgo.Handle
}

// RegisterKey binds keys on the client: fn runs with pressed true on press and false on
// release, and the current focus impact. Client hosts only; a server answers not provided.
func RegisterKey(name string, keyCodes []int32, allowRemap bool, fn func(pressed bool, impact int32)) (*KeyBinding, error) {
	if api == nil || !bool(C.piergo_has_client_register_key(api)) {
		return nil, notProvided("client_register_key")
	}
	h := cgo.NewHandle(fn)
	kh := C.piergo_call_client_register_key(api, self, str(name), int32sPtr(keyCodes), C.int32_t(len(keyCodes)),
		C.bool(allowRemap), C.piergo_key_cb(), C.piergo_key_cb(), handle(uintptr(h)))
	if kh == nil {
		h.Delete()
		return nil, fmt.Errorf("key binding %s was refused", name)
	}
	return &KeyBinding{h: KeyHandle{p: unsafe.Pointer(kh)}, fn: h}, nil
}

// Unregister stops the binding's callbacks.
func (k *KeyBinding) Unregister() error {
	if k == nil || k.h.IsZero() {
		return nil
	}
	known, err := Raw.ClientUnregisterKey(k.h)
	if err != nil {
		return err
	}
	k.h = KeyHandle{}
	k.fn.Delete()
	if !known {
		return errors.New("the host did not know this key binding")
	}
	return nil
}

// ChunkRequest is one chunk a supplied-terrain generator fills. Materials has 256*Height
// entries indexed (x*16+z)*Height + (y-MinY), and Biomes 256, one per column; both hold
// indices into the palettes given at registration, and material 0 is air.
type ChunkRequest struct {
	Dim, ChunkX, ChunkZ int32
	MinY, Height        int32
	Materials           []uint16
	Biomes              []uint16
}

// AddGeneratedDimension registers a dimension whose terrain fill writes, and returns its id.
//
// fill runs on the host's chunk worker threads, several at once: it must be safe to run
// concurrently, give the same answer for the same chunk forever, and call nothing on this
// package but the request itself. Returning false leaves the chunk to air. materials[0]
// must be "minecraft:air". The function is kept for the dimension's whole life.
func AddGeneratedDimension(name, spec string, materials, biomes []string, fill func(*ChunkRequest) bool) (int32, error) {
	if len(materials) == 0 || materials[0] != "minecraft:air" {
		return 0, errors.New("material palette entry 0 must be minecraft:air; the host writes it where nothing was filled")
	}
	if api == nil || !bool(C.piergo_has_md_add_dimension_generated(api)) {
		return 0, notProvided("md_add_dimension_generated")
	}
	// Never deleted, even when refused: a generator built during the attempt may hold it.
	h := cgo.NewHandle(fill)
	id := int32(C.piergo_call_md_add_dimension_generated(api, str(name), str(spec),
		str(strings.Join(materials, "\n")), str(strings.Join(biomes, "\n")), C.piergo_generate_fn(), handle(uintptr(h))))
	if id < 0 {
		return 0, refused("dimension " + name)
	}
	return id, nil
}

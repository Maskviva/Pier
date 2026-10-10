package levilamina

/*
#include "sdk/abi.h"
#include "slots_gen.h"
#include "glue.h"
*/
import "C"

import "unsafe"

// SelKind says how a PlayerSel names its player.
type SelKind int32

// The selector kinds. A decision about permissions, money or ownership keys on the xuid: a
// name can change and be taken by someone else.
const (
	SelName SelKind = 0
	SelXuid SelKind = 1
	SelUuid SelKind = 2
)

// PlayerSel names one player by name, xuid or uuid.
type PlayerSel struct {
	Kind  SelKind
	Value string
}

// ByName selects a player by name.
func ByName(name string) PlayerSel { return PlayerSel{Kind: SelName, Value: name} }

// ByXuid selects a player by xuid.
func ByXuid(xuid string) PlayerSel { return PlayerSel{Kind: SelXuid, Value: xuid} }

// ByUuid selects a player by uuid.
func ByUuid(uuid string) PlayerSel { return PlayerSel{Kind: SelUuid, Value: uuid} }

func (s PlayerSel) c() C.PierPlayerSel {
	return C.PierPlayerSel{kind: C.int32_t(s.Kind), value: str(s.Value)}
}

// ActorID is an actor's unique id, the `uid` of event payloads. 0 never resolves.
type ActorID int64

// ContainerRef names a container: a player's inventory, ender chest, armor or offhand, or
// the container of a block.
type ContainerRef struct {
	Which   int32
	Player  PlayerSel
	Dim     int32
	X, Y, Z int32
}

// The values of ContainerRef.Which.
const (
	ContainerInventory  int32 = 0
	ContainerEnderChest int32 = 1
	ContainerArmor      int32 = 2
	ContainerOffhand    int32 = 3
	ContainerBlock      int32 = 4
)

func (r ContainerRef) c() C.PierContainerRef {
	return C.PierContainerRef{which: C.int32_t(r.Which), player: r.Player.c(), dim: C.int32_t(r.Dim),
		x: C.int32_t(r.X), y: C.int32_t(r.Y), z: C.int32_t(r.Z)}
}

// PlayerPos is where a player is; Found is false when nobody matched.
type PlayerPos struct {
	X, Y, Z   float64
	Dimension int32
	Found     bool
}

func playerPosOf(p C.PierPlayerPos) PlayerPos {
	return PlayerPos{X: float64(p.x), Y: float64(p.y), Z: float64(p.z), Dimension: int32(p.dimension), Found: bool(p.found)}
}

// KvDbHandle is an open key-value store; the zero value is no store.
type KvDbHandle struct{ p unsafe.Pointer }

// KeyHandle is a registered client key binding.
type KeyHandle struct{ p unsafe.Pointer }

// PacketHookHandle is a registered packet hook.
type PacketHookHandle struct{ p unsafe.Pointer }

// ListenerHandle is an event listener as the host names it.
type ListenerHandle struct{ p unsafe.Pointer }

// IsZero reports whether the handle names nothing.
func (h KvDbHandle) IsZero() bool { return h.p == nil }

// IsZero reports whether the handle names nothing.
func (h KeyHandle) IsZero() bool { return h.p == nil }

// IsZero reports whether the handle names nothing.
func (h PacketHookHandle) IsZero() bool { return h.p == nil }

// IsZero reports whether the handle names nothing.
func (h ListenerHandle) IsZero() bool { return h.p == nil }

// bytesPtr passes a byte slice to C for the duration of one call.
func bytesPtr(b []byte) *C.uint8_t {
	if len(b) == 0 {
		return nil
	}
	return (*C.uint8_t)(unsafe.Pointer(&b[0]))
}

// int32sPtr passes an int32 slice to C for the duration of one call.
func int32sPtr(v []int32) *C.int32_t {
	if len(v) == 0 {
		return nil
	}
	return (*C.int32_t)(unsafe.Pointer(&v[0]))
}

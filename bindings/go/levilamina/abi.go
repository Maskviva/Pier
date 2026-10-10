package levilamina

/*
#cgo CFLAGS: -I${SRCDIR}/include -I${SRCDIR}
#include "sdk/abi.h"
#include "slots_gen.h"
#include "glue.h"
*/
import "C"

import (
	"fmt"
	"unsafe"
)

// The table and this mod's handle, set once by pier_main and only read after it.
var (
	api  *C.PierApi
	self C.PierModHandle
)

// The levels of the log slot, which mirror ll::io::LogLevel.
const (
	levelError = 1
	levelWarn  = 2
	levelInfo  = 3
	levelDebug = 4
)

const clientFlag = uint32(C.PIER_FLAG_CLIENT)

// modFlags may hold the client bit and nothing else; any other bit fails to compile here.
var _ = [1]struct{}{}[modFlags&^clientFlag]

// str views a Go string as a PierStr without copying. The host reads it during the call
// only, which the cgo pointer rules allow for memory that holds no Go pointers.
func str(s string) C.PierStr {
	if len(s) == 0 {
		return C.PierStr{}
	}
	return C.PierStr{ptr: (*C.char)(unsafe.Pointer(unsafe.StringData(s))), len: C.size_t(len(s))}
}

// goString copies a PierStr into Go memory; the host's bytes are valid only during the call.
func goString(s C.PierStr) string {
	if s.ptr == nil || s.len == 0 {
		return ""
	}
	return C.GoStringN(s.ptr, C.int(s.len))
}

func logAt(level int32, msg string) {
	if api == nil || !bool(C.piergo_has_log(api)) {
		return
	}
	C.piergo_call_log(api, self, C.int32_t(level), str(msg))
}

func handle(h uintptr) unsafe.Pointer {
	return C.piergo_handle(C.uintptr_t(h))
}

// notProvided is the error of a call whose slot this host does not have.
func notProvided(what string) error {
	e := &NotProvidedError{What: what}
	if api != nil {
		e.HostABI, e.TableSize = uint32(api.abi_version), uint32(api.struct_size)
	}
	return e
}

// recoverTo turns a panic into a logged failure. It is the deferred function itself, which
// is what lets its recover see the panic.
func recoverTo(ok *bool, where string) {
	if r := recover(); r != nil {
		*ok = false
		logAt(levelError, fmt.Sprintf("a panic left %s: %v", where, r))
	}
}

// enter is pier_main: the handshake in the order contract section 10 gives, then Load.
func enter(a *C.PierApi, s C.PierModHandle, out *C.PierModVTable) (ok bool) {
	defer recoverTo(&ok, "pier_main")
	// Until the table is known to reach log nothing can be said, so this failure is
	// silent here and reported by the host.
	if a == nil || out == nil || !bool(C.piergo_has_log(a)) {
		return false
	}
	api, self = a, s
	if uint32(a.abi_version) < uint32(C.PIER_ABI_VERSION) {
		logAt(levelError, fmt.Sprintf("this host speaks Pier ABI v%d and this mod was built against v%d; upgrade Pier",
			uint32(a.abi_version), uint32(C.PIER_ABI_VERSION)))
		return false
	}
	if uint32(a.host_flags)&clientFlag != modFlags&clientFlag {
		logAt(levelError, "this mod was built for the other target, server or client, than this host")
		return false
	}
	if current == nil {
		logAt(levelError, "no mod is registered; call levilamina.Register from an init function")
		return false
	}
	if l, isLoader := current.(Loader); isLoader {
		if err := l.Load(&Context{}); err != nil {
			logAt(levelError, modName+": load failed: "+err.Error())
			return false
		}
	}
	C.piergo_fill_vtable(out, C.uint32_t(modFlags))
	return true
}

// runStage runs one lifecycle step and turns an error or a panic into a refusal.
func runStage(stage string, fn func(*Context) error) (ok bool) {
	defer recoverTo(&ok, stage)
	if err := fn(&Context{}); err != nil {
		logAt(levelError, modName+": "+stage+" failed: "+err.Error())
		return false
	}
	return true
}

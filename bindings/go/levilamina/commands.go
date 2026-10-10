package levilamina

/*
#include "sdk/abi.h"
#include "slots_gen.h"
#include "glue.h"
*/
import "C"

import (
	"fmt"
	"runtime/cgo"
	"unsafe"
)

// Permission is who may run a command; it mirrors CommandPermissionLevel.
type Permission int32

// The permission levels, from everyone to the server owner.
const (
	PermissionAny           Permission = 0
	PermissionGameDirectors Permission = 1
	PermissionAdmin         Permission = 2
	PermissionHost          Permission = 3
	PermissionOwner         Permission = 4
)

// Invocation is one run of a command. Success and Error may be called any number of times
// during the callback and not after it returns.
type Invocation struct {
	// Args is the raw text after the command name, possibly empty.
	Args string
	// Origin is the display name of whoever ran it: a player's name, or "Server".
	Origin string

	ctx  unsafe.Pointer
	ok   C.PierStrSink
	fail C.PierStrSink
}

// Success sends a line of output.
func (i *Invocation) Success(msg string) { C.piergo_sink(i.ok, i.ctx, str(msg)) }

// Error sends a line of error output.
func (i *Invocation) Error(msg string) { C.piergo_sink(i.fail, i.ctx, str(msg)) }

// RegisterCommand registers /name, whose text after the name reaches fn as Args. Call it
// from Enable, on the server thread. A command stays registered while the server runs,
// since Bedrock cannot unregister one; the host mutes it while the mod is disabled.
func RegisterCommand(name, description string, permission Permission, fn func(*Invocation)) error {
	if api == nil || !bool(C.piergo_has_register_command(api)) {
		return notProvided("register_command")
	}
	h := cgo.NewHandle(fn)
	if !bool(C.piergo_call_register_command(api, self, str(name), str(description), C.int32_t(permission), C.piergo_command_cb(), handle(uintptr(h)))) {
		h.Delete()
		return fmt.Errorf("registering /%s failed: another mod may already own that name", name)
	}
	return nil
}

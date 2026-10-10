// Package levilamina writes LeviLamina mods in Go, on Pier's C ABI.
//
// A mod is a DLL built with `go build -buildmode=c-shared`. It implements Mod, calls
// Register from an init function, and has an empty main. The host calls pier_main after the
// Go runtime has run every init, so the registered mod is there when the handshake asks.
//
// Everything that reaches the host goes through the slot gates of contract section 10: a
// call to a slot the host does not have returns a *NotProvidedError without calling anything.
package levilamina

import (
	"errors"
	"fmt"
)

// Mod is the lifecycle a Go mod implements. Both steps run on the server thread, and a
// returned error is logged and refuses the step.
type Mod interface {
	Enable(ctx *Context) error
	Disable(ctx *Context) error
}

// Loader is implemented by a mod with work to do at load, before it is enabled.
type Loader interface {
	Load(ctx *Context) error
}

// Unloader is implemented by a mod with work to do at unload, after it is disabled.
type Unloader interface {
	Unload(ctx *Context) error
}

var (
	current Mod
	modName string
)

// Register names the mod this DLL is. Call it once, from an init function; a DLL with
// nothing registered refuses to load, and a second call panics, since a DLL is one mod.
func Register(name string, m Mod) {
	if current != nil {
		panic("levilamina.Register called twice; a DLL is one mod")
	}
	current, modName = m, name
}

// Context is what a lifecycle step receives.
type Context struct{}

// Logger returns the logger of this mod.
func (c *Context) Logger() Logger { return Logger{} }

// Logger writes to the server log under this mod's name. The zero value is ready, and it
// is safe from any goroutine.
type Logger struct{}

// Error logs at error level.
func (Logger) Error(msg string) { logAt(levelError, msg) }

// Warn logs at warning level.
func (Logger) Warn(msg string) { logAt(levelWarn, msg) }

// Info logs at info level.
func (Logger) Info(msg string) { logAt(levelInfo, msg) }

// Debug logs at debug level.
func (Logger) Debug(msg string) { logAt(levelDebug, msg) }

// NotProvidedError is the error of a call whose slot this host does not have: the host is
// older than this binding, or the capability package was not built into it.
type NotProvidedError struct {
	What      string
	HostABI   uint32
	TableSize uint32
}

func (e *NotProvidedError) Error() string {
	return fmt.Sprintf("this Pier host does not provide %s (host ABI v%d, table of %d bytes)",
		e.What, e.HostABI, e.TableSize)
}

// IsNotProvided reports whether err says the host lacks a slot.
func IsNotProvided(err error) bool {
	var np *NotProvidedError
	return errors.As(err, &np)
}

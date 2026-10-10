# Your first Go mod

This builds `examples/hello-pier-go`: a command, a listener on player joins, and a task
that runs one second after the mod is enabled.

## The module

A mod is an ordinary Go module whose main package is built as a DLL:

```
go.mod
main.go
manifest.json
```

`go.mod` requires the binding. Inside the Pier repository the example points at it with a
`replace`; a mod of your own requires it by version instead.

```
module hello-pier-go

go 1.21

require github.com/Maskviva/pier/bindings/go v0.0.0

replace github.com/Maskviva/pier/bindings/go => ../../bindings/go
```

## main.go

```go
package main

import (
	"time"

	"github.com/Maskviva/pier/bindings/go/levilamina"
)

type hello struct {
	joins *levilamina.Listener
}

func (h *hello) Enable(ctx *levilamina.Context) error {
	log := ctx.Logger()
	joins, err := levilamina.Subscribe("ll::event::PlayerJoinEvent", levilamina.PriorityNormal, func(ev *levilamina.Event) {
		log.Info("a player joined: " + ev.SNBT)
	})
	if err != nil {
		return err
	}
	h.joins = joins

	err = levilamina.RegisterCommand("hellogo", "Says hello from a Go mod.", levilamina.PermissionAny, func(inv *levilamina.Invocation) {
		inv.Success("hello from a Go mod, " + inv.Origin)
	})
	if err != nil {
		log.Warn(err.Error())
	}

	_, err = levilamina.ScheduleAfter(time.Second, func() { log.Info("one second later") })
	return err
}

func (h *hello) Disable(ctx *levilamina.Context) error {
	return h.joins.Unsubscribe()
}

func init() {
	levilamina.Register("hello-pier-go", &hello{})
}

func main() {}
```

`Register` runs from `init`, because the host calls `pier_main` after the Go runtime has run
every `init` and before anything else. `main` is required by the build mode and never runs.

A failed command registration logs a warning and enabling goes on: the rest of the mod does
not need the command, and the warning says which command is missing and why.

## Build

```
set CGO_ENABLED=1
go build -buildmode=c-shared -trimpath -o hello_pier_go.dll .
```

`build.bat` in the example does the same and says what is missing when Go or gcc is not on
`PATH`.

## Install

`manifest.json` names the DLL and declares the mod as type `pier`:

```json
{
    "name": "hello-pier-go",
    "entry": "hello_pier_go.dll",
    "type": "pier",
    "version": "0.1.0",
    "dependencies": [{ "name": "pier" }]
}
```

Put both files in `plugins/hello-pier-go/` and start the server. The log shows the mod
loading and enabling, and one second later the line from the task. `/hellogo` answers in
game and from the console.

If the mod does not load, [Troubleshooting](../guide/troubleshooting.md) lists the log lines and
what each means.

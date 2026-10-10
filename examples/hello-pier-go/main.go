// Command hello-pier-go is the smallest Go mod that does something: one command, one event
// listener, one delayed task, and a service and a bus topic any other mod can use, in
// whatever language it is written, with a log line at every lifecycle step.
//
// It is built as a DLL, not run as a program: `go build -buildmode=c-shared`. The host calls
// pier_main, which the binding exports, after the Go runtime has run init below; main is
// required by the build mode and never runs.
package main

import (
	"errors"
	"fmt"
	"time"

	"github.com/Maskviva/pier/bindings/go/levilamina"
)

type hello struct {
	joins *levilamina.Listener
	greet *levilamina.ServiceRegistration
	pings *levilamina.Subscription
}

func (h *hello) Load(ctx *levilamina.Context) error {
	ctx.Logger().Info("loaded")
	return nil
}

func (h *hello) Enable(ctx *levilamina.Context) error {
	log := ctx.Logger()
	log.Info("enabled")

	joins, err := levilamina.Subscribe("ll::event::PlayerJoinEvent", levilamina.PriorityNormal, func(ev *levilamina.Event) {
		// The payload is SNBT; its fields are in the event payload reference.
		log.Info("a player joined: " + ev.SNBT)
	})
	if err != nil {
		return err
	}
	h.joins = joins

	err = levilamina.RegisterCommand("hellogo", "Says hello from a Go mod.", levilamina.PermissionAny, func(inv *levilamina.Invocation) {
		reply := "hello from a Go mod, " + inv.Origin
		if inv.Args != "" {
			reply += " (you said: " + inv.Args + ")"
		}
		inv.Success(reply)
	})
	if err != nil {
		// Not fatal: the mod still works without its command.
		log.Warn(err.Error())
	}

	// Another mod, in any language, calls this with service "hello-pier-go:greet".
	h.greet, err = levilamina.RegisterService("hello-pier-go:greet", func(request string) (string, error) {
		if request == "" {
			return "", errors.New("say who to greet")
		}
		return "hello, " + request + ", from a Go mod", nil
	})
	if err != nil {
		return err
	}

	// Another mod publishes on "hello:ping"; this one answers on "hello:pong".
	h.pings, err = levilamina.BusSubscribe("hello:ping", func(topic, payload string) bool {
		log.Info("ping from another mod: " + payload)
		if _, perr := levilamina.BusPublish("hello:pong", payload); perr != nil {
			log.Warn(perr.Error())
		}
		return false
	})
	if err != nil {
		return err
	}

	_, err = levilamina.ScheduleAfter(time.Second, func() {
		log.Info("a task scheduled one second ago ran on the server thread")
		if services, lerr := levilamina.ListServices(); lerr == nil {
			log.Info(fmt.Sprintf("%d service(s) are registered on this server", len(services)))
		}
	})
	return err
}

func (h *hello) Disable(ctx *levilamina.Context) error {
	ctx.Logger().Info("disabled")
	return errors.Join(h.joins.Unsubscribe(), h.pings.Unsubscribe(), h.greet.Unregister())
}

func (h *hello) Unload(ctx *levilamina.Context) error {
	ctx.Logger().Info("unloaded")
	return nil
}

func init() {
	levilamina.Register("hello-pier-go", &hello{})
}

func main() {}

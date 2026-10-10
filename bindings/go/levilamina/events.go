package levilamina

/*
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
)

// Priority orders the listeners of one event; it mirrors ll::event::EventPriority.
type Priority int32

// The priorities, from the listener that runs first to the one that runs last.
const (
	PriorityHighest Priority = 0
	PriorityHigh    Priority = 1
	PriorityNormal  Priority = 2
	PriorityLow     Priority = 3
	PriorityLowest  Priority = 4
)

// Event is one event as a listener receives it. SNBT is the payload, whose fields the
// event payload reference documents; it is a copy, valid after the callback returns.
type Event struct {
	ID   string
	SNBT string

	parsed *Nbt
	edits  *Nbt
}

// Payload is SNBT parsed, once per event.
func (e *Event) Payload() (*Nbt, error) {
	if e.parsed == nil {
		v, err := ParseSNBT(e.SNBT)
		if err != nil {
			return nil, err
		}
		e.parsed = v
	}
	return e.parsed, nil
}

// Unresolved lists the fields the host could not resolve, from _unresolved. It is empty
// too when the payload does not parse, which CheckComplete does not count as complete.
func (e *Event) Unresolved() []string {
	v, err := e.Payload()
	if err != nil {
		return nil
	}
	var out []string
	if list := v.Get("_unresolved"); list != nil && list.Kind == NbtList {
		for _, item := range list.List {
			if item.Kind == NbtString {
				out = append(out, item.Str)
			}
		}
	}
	return out
}

// CheckComplete reports whether the payload parsed and nothing in it is unresolved. A
// protection decision on an incomplete payload should refuse rather than guess.
func (e *Event) CheckComplete() bool {
	_, err := e.Payload()
	return err == nil && len(e.Unresolved()) == 0
}

// Dim is the dimension the event happened in. An unreadable or incomplete payload is an
// error and never 0: read as the overworld it would let an event in a custom dimension
// pass a rule made for the overworld.
func (e *Event) Dim() (int32, error) {
	v, err := e.Payload()
	if err != nil {
		return 0, fmt.Errorf("the payload of %s could not be parsed, so the dimension is unknown: %w", e.ID, err)
	}
	if miss := e.Unresolved(); len(miss) > 0 {
		return 0, fmt.Errorf("the payload of %s is incomplete, the host could not resolve %s, so the dimension is unknown",
			e.ID, strings.Join(miss, ", "))
	}
	d, ok := v.OptInt("dim")
	if !ok {
		return 0, fmt.Errorf("the payload of %s has no dimension", e.ID)
	}
	return int32(d), nil
}

// Set writes key back into the event when the callback returns. Only the keys set go back,
// and the host merges them, so two listeners setting different keys keep both.
func (e *Event) Set(key string, value *Nbt) {
	if e.edits == nil {
		e.edits = NewCompound()
	}
	e.edits.Set(key, value)
}

// Cancel asks the host to cancel the event once the callback returns. An event that cannot
// be cancelled ignores it; the documentation of each event says whether it can be.
func (e *Event) Cancel() { e.Set("cancelled", NbtByteOf(1)) }

// Uncancel undoes a Cancel of this callback. A cancel made by an earlier listener stays.
func (e *Event) Uncancel() { e.Set("cancelled", NbtByteOf(0)) }

// Listener is one subscription, kept to end it.
type Listener struct {
	handle C.PierListenerHandle
	fn     cgo.Handle
}

// Subscribe calls fn for every event with this id, on the server thread. The id is the full
// one, such as "ll::event::PlayerJoinEvent", or a unique suffix of it. Server thread only.
func Subscribe(id string, priority Priority, fn func(*Event)) (*Listener, error) {
	if api == nil || !bool(C.piergo_has_subscribe_event(api)) {
		return nil, notProvided("subscribe_event")
	}
	h := cgo.NewHandle(fn)
	lh := C.piergo_call_subscribe_event(api, self, str(id), C.int32_t(priority), C.piergo_event_cb(), handle(uintptr(h)))
	if lh == nil {
		h.Delete()
		return nil, fmt.Errorf("subscribing to %q failed: the id is unknown or ambiguous, or the event is turned off in config.json", id)
	}
	return &Listener{handle: lh, fn: h}, nil
}

// Unsubscribe ends the subscription. Calling it twice is harmless. Server thread only.
func (l *Listener) Unsubscribe() error {
	if l == nil || l.handle == nil {
		return nil
	}
	if api == nil || !bool(C.piergo_has_unsubscribe_event(api)) {
		return notProvided("unsubscribe_event")
	}
	known := bool(C.piergo_call_unsubscribe_event(api, self, l.handle))
	l.handle = nil
	l.fn.Delete()
	if !known {
		return errors.New("the host did not know this listener")
	}
	return nil
}

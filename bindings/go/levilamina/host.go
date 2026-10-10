package levilamina

/*
#include "sdk/abi.h"
#include "slots_gen.h"
#include "glue.h"
*/
import "C"

import (
	"errors"
	"runtime/cgo"
	"time"
)

// GamingStatus is the server's life stage, as LeviLamina reports it.
type GamingStatus int32

// The stages of GamingStatus, in the order a server passes through them.
const (
	StatusDefault  GamingStatus = 0
	StatusStarting GamingStatus = 1
	StatusRunning  GamingStatus = 2
	StatusStopping GamingStatus = 3
)

// Status reports the server's life stage. Safe from any goroutine.
func Status() (GamingStatus, error) {
	if api == nil || !bool(C.piergo_has_gaming_status(api)) {
		return 0, notProvided("gaming_status")
	}
	return GamingStatus(C.piergo_call_gaming_status(api)), nil
}

// TaskID names a scheduled task, for Cancel.
type TaskID uint64

// Schedule runs fn on the server thread as soon as it can. Safe from any goroutine: most of
// the host is server-thread only, and this is how a goroutine gets back onto that thread.
//
// The task belongs to this mod: if the mod unloads first, the host drops it and fn never
// runs, so nothing calls into a DLL that is gone.
func Schedule(fn func()) (TaskID, error) {
	if api == nil || !bool(C.piergo_has_schedule_for(api)) {
		return 0, notProvided("schedule_for")
	}
	h := cgo.NewHandle(fn)
	id := uint64(C.piergo_call_schedule_for(api, self, C.piergo_task_cb(), handle(uintptr(h))))
	if id == 0 {
		h.Delete()
		return 0, errors.New("the host refused the task")
	}
	return TaskID(id), nil
}

// ScheduleAfter runs fn on the server thread once d has passed, under the same ownership as
// Schedule. Safe from any goroutine.
func ScheduleAfter(d time.Duration, fn func()) (TaskID, error) {
	if api == nil || !bool(C.piergo_has_schedule_after_for(api)) {
		return 0, notProvided("schedule_after_for")
	}
	ms := d.Milliseconds()
	if ms < 0 {
		ms = 0
	}
	h := cgo.NewHandle(fn)
	id := uint64(C.piergo_call_schedule_after_for(api, self, C.piergo_task_cb(), handle(uintptr(h)), C.uint64_t(ms)))
	if id == 0 {
		h.Delete()
		return 0, errors.New("the host refused the task")
	}
	return TaskID(id), nil
}

// Cancel voids a task that has not run. It reports false for a task that already ran, was
// cancelled, or is not this mod's. The function itself is released when the process ends,
// so cancelling in bulk on a hot path leaks.
func Cancel(task TaskID) (bool, error) {
	if api == nil || !bool(C.piergo_has_schedule_cancel(api)) {
		return false, notProvided("schedule_cancel")
	}
	return bool(C.piergo_call_schedule_cancel(api, self, C.uint64_t(task))), nil
}

// PendingTasks counts this mod's tasks that have not run, which an Unload can check is 0.
// A host that cannot count returns an error, which that check receives in place of a 0.
func PendingTasks() (uint32, error) {
	if api == nil || !bool(C.piergo_has_schedule_pending_count(api)) {
		return 0, notProvided("schedule_pending_count")
	}
	return uint32(C.piergo_call_schedule_pending_count(api, self)), nil
}

// ListEvents lists every event id this host can subscribe to. Server thread only.
func ListEvents() ([]string, error) {
	if api == nil || !bool(C.piergo_has_list_events(api)) {
		return nil, notProvided("list_events")
	}
	col := &collector{}
	h := cgo.NewHandle(col)
	defer h.Delete()
	C.piergo_call_list_events(api, handle(uintptr(h)), C.piergo_str_sink())
	return col.items, nil
}

package levilamina

// The slots whose callbacks the generator leaves to hand-written code: each hands the host
// one of the exported functions of exports.go and gathers what it receives.

/*
#include "sdk/abi.h"
#include "slots_gen.h"
#include "glue.h"
*/
import "C"

import (
	"encoding/json"
	"errors"
	"fmt"
	"runtime/cgo"
	"sync"
)

// CommandOutput is one line a command printed.
type CommandOutput struct {
	Success bool
	Text    string
}

type outputCollector struct{ lines []CommandOutput }

// ExecuteCommand runs cmd as the console and returns what it printed. Server thread only. An
// error means the level is not ready, or the host cannot run commands.
func ExecuteCommand(cmd string) ([]CommandOutput, error) {
	if api == nil || !bool(C.piergo_has_execute_command(api)) {
		return nil, notProvided("execute_command")
	}
	col := &outputCollector{}
	h := cgo.NewHandle(col)
	defer h.Delete()
	if !bool(C.piergo_call_execute_command(api, str(cmd), handle(uintptr(h)), C.piergo_cmd_output_sink())) {
		return col.lines, errors.New("the level is not ready, so the command was not run")
	}
	return col.lines, nil
}

// BlockInfo is one block: its cell, its type name and its block state as SNBT.
type BlockInfo struct {
	X, Y, Z int32
	Name    string
	SNBT    string
}

type blockCollector struct{ blocks []BlockInfo }

func (c *blockCollector) addBlock(b BlockInfo) { c.blocks = append(c.blocks, b) }

// GetBlock reads the block at a cell. Server thread only.
func GetBlock(dim, x, y, z int32) (BlockInfo, error) {
	if api == nil || !bool(C.piergo_has_get_block(api)) {
		return BlockInfo{}, notProvided("get_block")
	}
	col := &blockCollector{}
	h := cgo.NewHandle(col)
	defer h.Delete()
	ok := bool(C.piergo_call_get_block(api, C.int32_t(dim), C.int32_t(x), C.int32_t(y), C.int32_t(z),
		handle(uintptr(h)), C.piergo_block_sink()))
	if !ok || len(col.blocks) == 0 {
		return BlockInfo{}, fmt.Errorf("the block at %d %d %d in dimension %d could not be read", x, y, z, dim)
	}
	return col.blocks[0], nil
}

// ActorInfo is one actor of a dimension: its unique id and its type name.
type ActorInfo struct {
	ID   ActorID
	Type string
}

type actorCollector struct{ actors []ActorInfo }

// ListActors lists every actor in a dimension. Server thread only.
func ListActors(dim int32) ([]ActorInfo, error) {
	if api == nil || !bool(C.piergo_has_list_actors(api)) {
		return nil, notProvided("list_actors")
	}
	col := &actorCollector{}
	h := cgo.NewHandle(col)
	defer h.Delete()
	C.piergo_call_list_actors(api, C.int32_t(dim), handle(uintptr(h)), C.piergo_actor_sink())
	return col.actors, nil
}

// SlotItem is one occupied slot of a container and its item as SNBT.
type SlotItem struct {
	Slot int32
	SNBT string
}

type slotCollector struct{ slots []SlotItem }

// ContainerItems lists the occupied slots of a container. Server thread only.
func ContainerItems(ref ContainerRef) ([]SlotItem, error) {
	if api == nil || !bool(C.piergo_has_container_get_items(api)) {
		return nil, notProvided("container_get_items")
	}
	col := &slotCollector{}
	h := cgo.NewHandle(col)
	defer h.Delete()
	if !bool(C.piergo_call_container_get_items(api, ref.c(), handle(uintptr(h)), C.piergo_slot_sink())) {
		return nil, errors.New("the container could not be read")
	}
	return col.slots, nil
}

// KeyValue is one entry of a key-value store.
type KeyValue struct {
	Key   string
	Value string
}

type kvCollector struct{ pairs []KeyValue }

func kvdbIter(db KvDbHandle) ([]KeyValue, error) {
	if api == nil || !bool(C.piergo_has_kvdb_iter(api)) {
		return nil, notProvided("kvdb_iter")
	}
	col := &kvCollector{}
	h := cgo.NewHandle(col)
	defer h.Delete()
	C.piergo_call_kvdb_iter(api, C.PierKvDbHandle(db.p), handle(uintptr(h)), C.piergo_kv_sink())
	return col.pairs, nil
}

type bytesCollector struct{ data []byte }

// SnbtToBinary encodes SNBT as binary NBT in format 0, the little-endian disk layout, or
// format 1, the network layout.
func SnbtToBinary(snbt string, format int32) ([]byte, error) {
	if api == nil || !bool(C.piergo_has_nbt_snbt_to_binary(api)) {
		return nil, notProvided("nbt_snbt_to_binary")
	}
	col := &bytesCollector{}
	h := cgo.NewHandle(col)
	defer h.Delete()
	if !bool(C.piergo_call_nbt_snbt_to_binary(api, str(snbt), C.int32_t(format), handle(uintptr(h)), C.piergo_bytes_sink())) {
		return nil, errors.New("the SNBT could not be encoded")
	}
	return col.data, nil
}

// SendForm shows a form to a player. fn runs once, on the server thread, with the result
// SNBT, including when the player closes the form; abi.h documents the result's shape.
func SendForm(player PlayerSel, kind int32, formSNBT string, fn func(result string)) error {
	if api == nil || !bool(C.piergo_has_form_send(api)) {
		return notProvided("form_send")
	}
	h := cgo.NewHandle(fn)
	if !bool(C.piergo_call_form_send(api, self, player.c(), C.int32_t(kind), str(formSNBT), C.piergo_form_cb(), handle(uintptr(h)))) {
		h.Delete()
		return fmt.Errorf("the form could not be sent to %s", player.Value)
	}
	return nil
}

// Subscription is one bus subscription, kept to end it.
type Subscription struct {
	id    uint64
	topic string
	fn    cgo.Handle
}

// ID is the host's id of the subscription.
func (s *Subscription) ID() uint64 { return s.id }

// Topic is the topic subscribed to.
func (s *Subscription) Topic() string { return s.topic }

// Unsubscribe ends the subscription. Calling it twice is harmless.
func (s *Subscription) Unsubscribe() error {
	if s == nil || s.id == 0 {
		return nil
	}
	known, err := Raw.BusUnsubscribe(s.id)
	if err != nil {
		return err
	}
	s.id = 0
	s.fn.Delete()
	if !known {
		return errors.New("the host did not know this subscription")
	}
	return nil
}

// BusSubscribe calls fn for every message published on topic, on the publisher's thread,
// whichever language the publisher is written in. fn returns a veto: true refuses a vetoable
// publish and false has no opinion, and a plain publish ignores it. A subscriber can only
// refuse, never overturn another's refusal. Namespace topics, as "plot:enter".
func BusSubscribe(topic string, fn func(topic, payload string) (veto bool)) (*Subscription, error) {
	if api == nil || !bool(C.piergo_has_bus_subscribe(api)) {
		return nil, notProvided("bus_subscribe")
	}
	h := cgo.NewHandle(fn)
	id := uint64(C.piergo_call_bus_subscribe(api, self, str(topic), C.piergo_bus_cb(), handle(uintptr(h))))
	if id == 0 {
		h.Delete()
		return nil, fmt.Errorf("subscribing to topic %q failed: the topic is empty or too long, or the mod is not adopted yet", topic)
	}
	return &Subscription{id: id, topic: topic, fn: h}, nil
}

// BusPublish sends payload to every subscriber of topic and returns how many ran; 0 is a
// normal answer, meaning nobody is listening. The payload is opaque to the host, so the two
// mods agree on its format, JSON or SNBT, out of band.
func BusPublish(topic, payload string) (uint32, error) { return Raw.BusPublish(topic, payload) }

// Vetoable is the result of one vetoable publish.
type Vetoable struct {
	// Vetoed is true when any subscriber refused.
	Vetoed bool
	// Delivered is how many subscribers ran. There is no short circuit: after a veto the
	// later subscribers still receive the message, so observers see a consistent stream.
	Delivered uint32
}

// BusPublishVetoable sends payload and collects the subscribers' vetoes.
func BusPublishVetoable(topic, payload string) (Vetoable, error) {
	delivered, vetoed, err := Raw.BusPublishVetoable(topic, payload)
	return Vetoable{Vetoed: vetoed, Delivered: delivered}, err
}

// BusSubscriberCount is how many subscribers topic has now.
func BusSubscriberCount(topic string) (uint32, error) { return Raw.BusSubscriberCount(topic) }

// ServiceRegistration is one service this mod provides, kept to withdraw it.
type ServiceRegistration struct {
	id   uint64
	name string
	fn   cgo.Handle
}

// ID is the host's id of the registration.
func (r *ServiceRegistration) ID() uint64 { return r.id }

// Name is the service name.
func (r *ServiceRegistration) Name() string { return r.name }

// Unregister withdraws the service. Calling it twice is harmless.
func (r *ServiceRegistration) Unregister() error {
	if r == nil || r.id == 0 {
		return nil
	}
	known, err := Raw.ServiceUnregister(r.id)
	if err != nil {
		return err
	}
	r.id = 0
	r.fn.Delete()
	if !known {
		return errors.New("the host did not know this service registration")
	}
	return nil
}

// RegisterService answers calls to name from mods of any language, and from native plugins
// through the bridge. fn receives the request and returns the reply, or an error whose
// message reaches the caller unchanged. It runs on the caller's thread, and also while this
// mod is disabled but loaded, since consumers resolve services in their own on_load.
func RegisterService(name string, fn func(request string) (string, error)) (*ServiceRegistration, error) {
	if api == nil || !bool(C.piergo_has_service_register(api)) {
		return nil, notProvided("service_register")
	}
	h := cgo.NewHandle(fn)
	id := uint64(C.piergo_call_service_register(api, self, str(name), C.piergo_service_cb(), handle(uintptr(h))))
	if id == 0 {
		h.Delete()
		return nil, fmt.Errorf("registering service %q failed: the name is invalid or another mod provides it", name)
	}
	return &ServiceRegistration{id: id, name: name, fn: h}, nil
}

// callProvider runs a provider and turns a panic into an error the caller reads.
func callProvider(fn func(string) (string, error), request string) (answer string, err error) {
	defer func() {
		if r := recover(); r != nil {
			err = fmt.Errorf("the provider failed: %v", r)
		}
	}()
	return fn(request)
}

// CallErrorKind says why a service call failed.
type CallErrorKind int

// The kinds of CallError, matching the PIER_SERVICE_* results.
const (
	CallNotFound CallErrorKind = iota + 1
	CallProvider
	CallRefused
	CallUnavailable
)

// CallError is why a service call failed: nobody provides the name, the provider said no
// and Message is what it said, the host refused the call (a bad name, a call to itself or a
// cycle), or the host has no service capability.
type CallError struct {
	Kind    CallErrorKind
	Name    string
	Message string
}

func (e *CallError) Error() string {
	switch e.Kind {
	case CallNotFound:
		return fmt.Sprintf("no mod provides service %q: it is not installed, not enabled, or misspelled", e.Name)
	case CallProvider:
		return fmt.Sprintf("service %q refused: %s", e.Name, e.Message)
	case CallRefused:
		return fmt.Sprintf("the host refused the call to %q: a bad name, a call to itself, or a cycle", e.Name)
	default:
		return "this Pier host has no service capability"
	}
}

// CallService calls another mod's service, whatever language it is written in, and returns
// its reply or a *CallError.
func CallService(name, request string) (string, error) {
	out, code, err := Raw.ServiceCall(name, request)
	if err != nil {
		if IsNotProvided(err) {
			return "", &CallError{Kind: CallUnavailable, Name: name}
		}
		return "", err
	}
	body := ""
	if len(out) > 0 {
		body = out[len(out)-1]
	}
	switch code {
	case 0:
		return body, nil
	case 1:
		return "", &CallError{Kind: CallNotFound, Name: name}
	case 2:
		return "", &CallError{Kind: CallProvider, Name: name, Message: body}
	default:
		return "", &CallError{Kind: CallRefused, Name: name}
	}
}

// CallServiceOptional is CallService with nobody providing the name as found false, for an
// optional integration that works without the other mod.
func CallServiceOptional(name, request string) (reply string, found bool, err error) {
	reply, err = CallService(name, request)
	var ce *CallError
	if errors.As(err, &ce) && ce.Kind == CallNotFound {
		return "", false, nil
	}
	return reply, err == nil, err
}

// ServiceInfo is one registered service and the mod providing it.
type ServiceInfo struct {
	Name string `json:"name"`
	Mod  string `json:"mod"`
}

// ListServices lists every registered service.
func ListServices() ([]ServiceInfo, error) {
	out, err := Raw.ServiceList()
	if err != nil {
		return nil, err
	}
	var list []ServiceInfo
	for _, text := range out {
		var part []ServiceInfo
		if err := json.Unmarshal([]byte(text), &part); err != nil {
			return nil, fmt.Errorf("the service list did not parse: %w", err)
		}
		list = append(list, part...)
	}
	return list, nil
}

// ServiceExists reports whether some mod provides name now.
func ServiceExists(name string) bool {
	list, err := ListServices()
	if err != nil {
		return false
	}
	for _, s := range list {
		if s.Name == name {
			return true
		}
	}
	return false
}

// ServiceCaller is the mod whose call is running, asked inside a provider. It is false
// outside a provider and for a caller with no mod, such as a native plugin through the
// bridge: a provider then knows it cannot attribute the request.
func ServiceCaller() (string, bool) {
	out, err := Raw.ServiceCaller()
	if err != nil || len(out) == 0 {
		return "", false
	}
	return out[len(out)-1], true
}

// MoneyKind is what an economy event does, one of the Money* kinds. A value outside them is
// one a newer economy backend sent; a veto listener should refuse it rather than guess.
type MoneyKind int32

// The economy event kinds.
const (
	MoneySet    MoneyKind = 0
	MoneyAdd    MoneyKind = 1
	MoneyReduce MoneyKind = 2
	MoneyTrans  MoneyKind = 3
)

// MoneyEvent is one economy transaction.
type MoneyEvent struct {
	Kind  MoneyKind
	From  string
	To    string
	Value int64
}

var (
	moneyMu     sync.Mutex
	moneyBefore []func(MoneyEvent) bool
	moneyAfter  []func(MoneyEvent)
	moneyHooked [2]bool
)

func moneyListeners(before bool) []func(MoneyEvent) bool {
	moneyMu.Lock()
	defer moneyMu.Unlock()
	if before {
		return append([]func(MoneyEvent) bool(nil), moneyBefore...)
	}
	out := make([]func(MoneyEvent) bool, 0, len(moneyAfter))
	for _, fn := range moneyAfter {
		fn := fn
		out = append(out, func(e MoneyEvent) bool { fn(e); return true })
	}
	return out
}

// OnMoneyBefore calls fn before every economy transaction; returning false vetoes it.
func OnMoneyBefore(fn func(MoneyEvent) bool) error {
	moneyMu.Lock()
	defer moneyMu.Unlock()
	if !moneyHooked[0] {
		if api == nil || !bool(C.piergo_has_money_listen_before_event(api)) {
			return notProvided("money_listen_before_event")
		}
		C.piergo_call_money_listen_before_event(api, C.piergo_money_before_cb())
		moneyHooked[0] = true
	}
	moneyBefore = append(moneyBefore, fn)
	return nil
}

// OnMoneyAfter calls fn after every economy transaction.
func OnMoneyAfter(fn func(MoneyEvent)) error {
	moneyMu.Lock()
	defer moneyMu.Unlock()
	if !moneyHooked[1] {
		if api == nil || !bool(C.piergo_has_money_listen_after_event(api)) {
			return notProvided("money_listen_after_event")
		}
		C.piergo_call_money_listen_after_event(api, C.piergo_money_after_cb())
		moneyHooked[1] = true
	}
	moneyAfter = append(moneyAfter, fn)
	return nil
}

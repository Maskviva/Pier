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
)

// ParamType is the type of one command parameter, the kind words of register_command_ex.
type ParamType string

// The parameter types. Enum and SoftEnum also name their enum, through RequiredEnum or
// OptionalEnum.
const (
	ParamInt           ParamType = "int"
	ParamBool          ParamType = "bool"
	ParamFloat         ParamType = "float"
	ParamString        ParamType = "string"
	ParamEnum          ParamType = "enum"
	ParamSoftEnum      ParamType = "soft_enum"
	ParamActor         ParamType = "actor"
	ParamPlayer        ParamType = "player"
	ParamBlockPos      ParamType = "block_pos"
	ParamVec3          ParamType = "vec3"
	ParamRawText       ParamType = "raw_text"
	ParamMessage       ParamType = "message"
	ParamJSON          ParamType = "json"
	ParamItem          ParamType = "item"
	ParamBlockName     ParamType = "block_name"
	ParamEffect        ParamType = "effect"
	ParamActorType     ParamType = "actor_type"
	ParamCommand       ParamType = "command"
	ParamRelativeFloat ParamType = "relative_float"
	ParamFilePath      ParamType = "file_path"
)

// Overload is one way a command parses: its parameters in order.
type Overload struct {
	params []*Nbt
}

// NewOverload begins an overload.
func NewOverload() *Overload { return &Overload{} }

func (o *Overload) push(name string, t ParamType, enum string, optional bool) *Overload {
	p := NewCompound()
	p.Set("name", NbtStringOf(name))
	p.Set("kind", NbtStringOf(string(t)))
	if optional {
		p.Set("optional", NbtByteOf(1))
	} else {
		p.Set("optional", NbtByteOf(0))
	}
	if enum != "" {
		p.Set("enum", NbtStringOf(enum))
	}
	o.params = append(o.params, p)
	return o
}

// Required adds a parameter that must be given.
func (o *Overload) Required(name string, t ParamType) *Overload { return o.push(name, t, "", false) }

// Optional adds a parameter that may be left out; Arg answers false for it then.
func (o *Overload) Optional(name string, t ParamType) *Overload { return o.push(name, t, "", true) }

// RequiredEnum adds an enum parameter, which RegisterCommandEnum or RegisterSoftEnum
// registered first.
func (o *Overload) RequiredEnum(name string, t ParamType, enum string) *Overload {
	return o.push(name, t, enum, false)
}

// OptionalEnum adds an enum parameter that may be left out.
func (o *Overload) OptionalEnum(name string, t ParamType, enum string) *Overload {
	return o.push(name, t, enum, true)
}

// Text adds a literal word, the shape /scoreboard objectives add is made of. The word comes
// back among the arguments under name, so a handler reads it like any other parameter. Sibling
// words taking the same arguments are one enum instead (contract section 6.1).
func (o *Overload) Text(name, literal string) *Overload {
	p := NewCompound()
	p.Set("name", NbtStringOf(name))
	p.Set("kind", NbtStringOf("text"))
	p.Set("text", NbtStringOf(literal))
	o.params = append(o.params, p)
	return o
}

// CommandBuilder declares a command with typed overloads. Each overload must parse a
// different input: two that accept the same words leave the engine to pick one.
type CommandBuilder struct {
	name, description string
	permission        Permission
	overloads         []*Overload
}

// NewCommand begins declaring /name.
func NewCommand(name, description string, permission Permission) *CommandBuilder {
	return &CommandBuilder{name: name, description: description, permission: permission}
}

// Overload adds one way the command parses.
func (b *CommandBuilder) Overload(o *Overload) *CommandBuilder {
	b.overloads = append(b.overloads, o)
	return b
}

// CommandOrigin is who ran a command and where.
type CommandOrigin struct {
	Name string
	// Kind is the CommandOriginType: 0 a player, 7 the server console, -1 not said.
	Kind    int32
	Dim     int32
	X, Y, Z float64
	HasPos  bool
}

// CommandCall is one run of a command declared with CommandBuilder.
type CommandCall struct {
	// Overload is the index of the overload that parsed.
	Overload int
	Args     *Nbt
	Origin   CommandOrigin
	inv      *Invocation
}

// Arg is a named argument, and false for an optional one left out.
func (c *CommandCall) Arg(name string) (*Nbt, bool) {
	if c.Args == nil {
		return nil, false
	}
	v := c.Args.Get(name)
	return v, v != nil
}

// ArgString is a named argument as text.
func (c *CommandCall) ArgString(name string) (string, bool) {
	if c.Args == nil {
		return "", false
	}
	return c.Args.OptString(name)
}

// ArgInt is a named integer argument.
func (c *CommandCall) ArgInt(name string) (int64, bool) {
	if c.Args == nil {
		return 0, false
	}
	return c.Args.OptInt(name)
}

// ArgFloat is a named number argument.
func (c *CommandCall) ArgFloat(name string) (float64, bool) {
	if c.Args == nil {
		return 0, false
	}
	return c.Args.OptFloat(name)
}

// IsConsole reports whether the dedicated server console ran the command.
func (c *CommandCall) IsConsole() bool { return c.Origin.Kind == 7 }

// Success sends a line of output.
func (c *CommandCall) Success(msg string) { c.inv.Success(msg) }

// Error sends a line of error output.
func (c *CommandCall) Error(msg string) { c.inv.Error(msg) }

// Register registers the command. fn runs on the server thread for every run; a payload the
// binding cannot parse is reported to whoever ran the command and fn does not run.
func (b *CommandBuilder) Register(fn func(*CommandCall)) error {
	if len(b.overloads) == 0 {
		return fmt.Errorf("/%s has no overload; give it at least one", b.name)
	}
	if api == nil || !bool(C.piergo_has_register_command_ex(api)) {
		return notProvided("register_command_ex")
	}
	list := &Nbt{Kind: NbtList}
	for _, o := range b.overloads {
		list.List = append(list.List, &Nbt{Kind: NbtList, List: o.params})
	}
	spec := NewCompound()
	spec.Set("overloads", list)
	text, err := spec.SNBT()
	if err != nil {
		return err
	}
	dispatch := func(inv *Invocation) {
		call, perr := parseCall(inv)
		if perr != nil {
			inv.Error("the command's arguments could not be read: " + perr.Error())
			return
		}
		fn(call)
	}
	h := cgo.NewHandle(dispatch)
	if !bool(C.piergo_call_register_command_ex(api, self, str(b.name), str(b.description), C.int32_t(b.permission),
		str(text), C.piergo_command_cb(), handle(uintptr(h)))) {
		h.Delete()
		return fmt.Errorf("registering /%s failed: another mod may own the name, or an overload is malformed", b.name)
	}
	return nil
}

func parseCall(inv *Invocation) (*CommandCall, error) {
	payload, err := ParseSNBT(inv.Args)
	if err != nil {
		return nil, err
	}
	call := &CommandCall{inv: inv, Args: payload.Get("args")}
	if n, ok := payload.OptInt("overload"); ok {
		call.Overload = int(n)
	}
	call.Origin = CommandOrigin{Name: inv.Origin, Kind: -1}
	// A command from this builder carries origin SNBT; a bare name is valid SNBT too, so
	// only a compound with a name is taken as the structured form.
	if o, oerr := ParseSNBT(inv.Origin); oerr == nil && o.Kind == NbtCompound {
		if name, ok := o.OptString("name"); ok {
			call.Origin.Name = name
			if k, ok := o.OptInt("type"); ok {
				call.Origin.Kind = int32(k)
			}
			d, okd := o.OptInt("dim")
			x, okx := o.OptFloat("x")
			y, oky := o.OptFloat("y")
			z, okz := o.OptFloat("z")
			if okd && okx && oky && okz {
				call.Origin.Dim, call.Origin.X, call.Origin.Y, call.Origin.Z, call.Origin.HasPos = int32(d), x, y, z, true
			}
		}
	}
	return call, nil
}

// EnumValue is one value of a command enum.
type EnumValue struct {
	Name  string
	Value int64
}

// RegisterCommandEnum registers a fixed enum for RequiredEnum and OptionalEnum.
func RegisterCommandEnum(name string, values []EnumValue) error {
	list := &Nbt{Kind: NbtList}
	for _, v := range values {
		list.List = append(list.List, &Nbt{Kind: NbtList, List: []*Nbt{NbtStringOf(v.Name), NbtLongOf(v.Value)}})
	}
	spec := NewCompound()
	spec.Set("values", list)
	text, err := spec.SNBT()
	if err != nil {
		return err
	}
	ok, err := Raw.RegisterCommandEnum(name, text)
	return okOr(ok, err, "command enum "+name)
}

func softValues(values []string) (string, error) {
	list := &Nbt{Kind: NbtList}
	for _, v := range values {
		list.List = append(list.List, NbtStringOf(v))
	}
	spec := NewCompound()
	spec.Set("values", list)
	return spec.SNBT()
}

// RegisterSoftEnum registers an enum whose values can change while the server runs.
func RegisterSoftEnum(name string, values []string) error {
	text, err := softValues(values)
	if err != nil {
		return err
	}
	ok, err := Raw.RegisterCommandSoftEnum(name, text)
	return okOr(ok, err, "soft enum "+name)
}

// SoftEnumOp is how UpdateSoftEnum changes the values.
type SoftEnumOp int32

// The soft enum operations.
const (
	SoftEnumSet    SoftEnumOp = 0
	SoftEnumAdd    SoftEnumOp = 1
	SoftEnumRemove SoftEnumOp = 2
)

// UpdateSoftEnum replaces, adds to or removes from the values of a soft enum.
func UpdateSoftEnum(name string, op SoftEnumOp, values []string) error {
	text, err := softValues(values)
	if err != nil {
		return err
	}
	ok, err := Raw.UpdateCommandSoftEnum(name, int32(op), text)
	return okOr(ok, err, "soft enum "+name)
}


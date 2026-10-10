package levilamina

import (
	"fmt"
	"math"
	"strconv"
	"strings"
)

// NbtKind is the type of one NBT value.
type NbtKind uint8

// The NBT kinds.
const (
	NbtByte NbtKind = iota + 1
	NbtShort
	NbtInt
	NbtLong
	NbtFloat
	NbtDouble
	NbtString
	NbtList
	NbtCompound
	NbtByteArray
	NbtIntArray
	NbtLongArray
)

// Nbt is one SNBT value. Integers of every width are in Int and both floating kinds in
// Float; a compound keeps its keys in order in Keys and its values in Map.
type Nbt struct {
	Kind  NbtKind
	Int   int64
	Float float64
	Str   string
	List  []*Nbt
	Keys  []string
	Map   map[string]*Nbt
	Ints  []int64
}

// nbtMaxDepth is the deepest nesting accepted, the cap Minecraft puts on NBT. The parser
// recurses once per level, and a payload nested deeper is refused instead of exhausting the
// stack of a host thread.
const nbtMaxDepth = 512

// ParseSNBT parses SNBT text, with the same rules as the Rust binding: a bare number is an
// int, then a long, then a double; a suffix b, s, l, f or d fixes the type; true and false
// are bytes; any other bare word is a string.
func ParseSNBT(text string) (*Nbt, error) {
	p := &snbtParser{s: text}
	v, err := p.value()
	if err != nil {
		return nil, err
	}
	p.ws()
	if p.i != len(p.s) {
		return nil, p.err("there are leftover characters after the value")
	}
	return v, nil
}

type snbtParser struct {
	s     string
	i     int
	depth int
}

func (p *snbtParser) err(what string) error {
	return fmt.Errorf("SNBT at byte %d: %s", p.i, what)
}

func (p *snbtParser) ws() {
	for p.i < len(p.s) && strings.IndexByte(" \t\r\n", p.s[p.i]) >= 0 {
		p.i++
	}
}

func (p *snbtParser) peek() (byte, bool) {
	if p.i < len(p.s) {
		return p.s[p.i], true
	}
	return 0, false
}

func (p *snbtParser) expect(c byte) error {
	p.ws()
	if p.i < len(p.s) && p.s[p.i] == c {
		p.i++
		return nil
	}
	return p.err(fmt.Sprintf("a %q was expected here", c))
}

func (p *snbtParser) value() (*Nbt, error) {
	p.ws()
	c, ok := p.peek()
	if !ok {
		return nil, p.err("the input ended in the middle of a value")
	}
	switch c {
	case '{':
		return p.nested(p.compound)
	case '[':
		return p.nested(p.listOrArray)
	case '"', '\'':
		s, err := p.quoted()
		if err != nil {
			return nil, err
		}
		return &Nbt{Kind: NbtString, Str: s}, nil
	}
	return p.scalar()
}

func (p *snbtParser) nested(parse func() (*Nbt, error)) (*Nbt, error) {
	if p.depth >= nbtMaxDepth {
		return nil, p.err(fmt.Sprintf("nested deeper than %d levels", nbtMaxDepth))
	}
	p.depth++
	defer func() { p.depth-- }()
	return parse()
}

func (p *snbtParser) compound() (*Nbt, error) {
	if err := p.expect('{'); err != nil {
		return nil, err
	}
	out := &Nbt{Kind: NbtCompound, Map: map[string]*Nbt{}}
	p.ws()
	if c, _ := p.peek(); c == '}' {
		p.i++
		return out, nil
	}
	for {
		p.ws()
		key, err := p.key()
		if err != nil {
			return nil, err
		}
		if err := p.expect(':'); err != nil {
			return nil, err
		}
		v, err := p.value()
		if err != nil {
			return nil, err
		}
		out.Set(key, v)
		p.ws()
		c, ok := p.peek()
		if !ok {
			return nil, p.err("the compound tag never closes")
		}
		p.i++
		if c == '}' {
			return out, nil
		}
		if c != ',' {
			return nil, p.err("a ',' or a '}' was expected inside the compound tag")
		}
	}
}

func (p *snbtParser) key() (string, error) {
	c, ok := p.peek()
	if !ok {
		return "", p.err("a key was expected here")
	}
	if c == '"' || c == '\'' {
		return p.quoted()
	}
	start := p.i
	for p.i < len(p.s) && isBareByte(p.s[p.i]) {
		p.i++
	}
	if p.i == start {
		return "", p.err("a key was expected here")
	}
	return p.s[start:p.i], nil
}

func (p *snbtParser) listOrArray() (*Nbt, error) {
	if err := p.expect('['); err != nil {
		return nil, err
	}
	if p.i+1 < len(p.s) && p.s[p.i+1] == ';' {
		kinds := map[byte]NbtKind{'B': NbtByteArray, 'I': NbtIntArray, 'L': NbtLongArray}
		if kind, typed := kinds[p.s[p.i]]; typed {
			p.i += 2
			return p.typedArray(kind)
		}
	}
	out := &Nbt{Kind: NbtList}
	p.ws()
	if c, _ := p.peek(); c == ']' {
		p.i++
		return out, nil
	}
	for {
		v, err := p.value()
		if err != nil {
			return nil, err
		}
		out.List = append(out.List, v)
		p.ws()
		c, ok := p.peek()
		if !ok {
			return nil, p.err("the list never closes")
		}
		p.i++
		if c == ']' {
			return out, nil
		}
		if c != ',' {
			return nil, p.err("a ',' or a ']' was expected inside the list")
		}
	}
}

func (p *snbtParser) typedArray(kind NbtKind) (*Nbt, error) {
	out := &Nbt{Kind: kind}
	p.ws()
	if c, _ := p.peek(); c == ']' {
		p.i++
		return out, nil
	}
	for {
		v, err := p.value()
		if err != nil {
			return nil, err
		}
		switch v.Kind {
		case NbtByte, NbtShort, NbtInt, NbtLong:
			out.Ints = append(out.Ints, v.Int)
		default:
			return nil, p.err("a typed array holds integers only")
		}
		p.ws()
		c, ok := p.peek()
		if !ok {
			return nil, p.err("the array never closes")
		}
		p.i++
		if c == ']' {
			return out, nil
		}
		if c != ',' {
			return nil, p.err("a ',' or a ']' was expected inside the array")
		}
	}
}

func (p *snbtParser) quoted() (string, error) {
	quote := p.s[p.i]
	p.i++
	var b strings.Builder
	for {
		if p.i >= len(p.s) {
			return "", p.err("the string has no closing quote")
		}
		c := p.s[p.i]
		p.i++
		if c == quote {
			return b.String(), nil
		}
		if c != '\\' {
			b.WriteByte(c)
			continue
		}
		if p.i >= len(p.s) {
			return "", p.err("there is nothing after the backslash")
		}
		e := p.s[p.i]
		p.i++
		switch e {
		case 'n':
			b.WriteByte('\n')
		case 'r':
			b.WriteByte('\r')
		case 't':
			b.WriteByte('\t')
		case 'b':
			b.WriteByte('\b')
		case 'f':
			b.WriteByte('\f')
		case '0':
			b.WriteByte(0)
		case 'u':
			if p.i+4 > len(p.s) {
				return "", p.err("there are fewer than four digits after \\u")
			}
			n, err := strconv.ParseUint(p.s[p.i:p.i+4], 16, 32)
			if err != nil {
				return "", p.err("\\u is not followed by four hex digits")
			}
			p.i += 4
			b.WriteRune(rune(n))
		default:
			b.WriteByte(e)
		}
	}
}

func (p *snbtParser) scalar() (*Nbt, error) {
	start := p.i
	for p.i < len(p.s) && isBareByte(p.s[p.i]) {
		p.i++
	}
	if p.i == start {
		return nil, p.err("a value was expected here")
	}
	raw := p.s[start:p.i]
	switch raw {
	case "true":
		return &Nbt{Kind: NbtByte, Int: 1}, nil
	case "false":
		return &Nbt{Kind: NbtByte, Int: 0}, nil
	}
	body, suffix := raw, byte(0)
	if last := raw[len(raw)-1]; strings.IndexByte("bBsSlLfFdD", last) >= 0 {
		body, suffix = raw[:len(raw)-1], last|0x20
	}
	if isNumeric(body) {
		switch suffix {
		case 'b':
			if v, err := strconv.ParseInt(body, 10, 64); err == nil {
				return &Nbt{Kind: NbtByte, Int: int64(int8(v))}, nil
			}
		case 's':
			if v, err := strconv.ParseInt(body, 10, 64); err == nil {
				return &Nbt{Kind: NbtShort, Int: int64(int16(v))}, nil
			}
		case 'l':
			if v, err := strconv.ParseInt(body, 10, 64); err == nil {
				return &Nbt{Kind: NbtLong, Int: v}, nil
			}
		case 'f':
			if v, err := strconv.ParseFloat(body, 32); err == nil {
				return &Nbt{Kind: NbtFloat, Float: v}, nil
			}
		case 'd':
			if v, err := strconv.ParseFloat(body, 64); err == nil {
				return &Nbt{Kind: NbtDouble, Float: v}, nil
			}
		default:
			if v, err := strconv.ParseInt(raw, 10, 32); err == nil {
				return &Nbt{Kind: NbtInt, Int: v}, nil
			}
			if v, err := strconv.ParseInt(raw, 10, 64); err == nil {
				return &Nbt{Kind: NbtLong, Int: v}, nil
			}
			if v, err := strconv.ParseFloat(raw, 64); err == nil {
				return &Nbt{Kind: NbtDouble, Float: v}, nil
			}
		}
	}
	return &Nbt{Kind: NbtString, Str: raw}, nil
}

func isBareByte(c byte) bool {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		c == '_' || c == '-' || c == '+' || c == '.'
}

func isNumeric(s string) bool {
	if s == "" {
		return false
	}
	for i := 0; i < len(s); i++ {
		c := s[i]
		if !(c >= '0' && c <= '9') && strings.IndexByte("-+.eE", c) < 0 {
			return false
		}
	}
	return true
}

// NewCompound is an empty compound.
func NewCompound() *Nbt { return &Nbt{Kind: NbtCompound, Map: map[string]*Nbt{}} }

// NbtByteOf is a byte value.
func NbtByteOf(v int8) *Nbt { return &Nbt{Kind: NbtByte, Int: int64(v)} }

// NbtIntOf is an int value.
func NbtIntOf(v int32) *Nbt { return &Nbt{Kind: NbtInt, Int: int64(v)} }

// NbtLongOf is a long value.
func NbtLongOf(v int64) *Nbt { return &Nbt{Kind: NbtLong, Int: v} }

// NbtDoubleOf is a double value.
func NbtDoubleOf(v float64) *Nbt { return &Nbt{Kind: NbtDouble, Float: v} }

// NbtStringOf is a string value.
func NbtStringOf(v string) *Nbt { return &Nbt{Kind: NbtString, Str: v} }

// Set puts value under key of a compound, keeping the key's first position.
func (v *Nbt) Set(key string, value *Nbt) {
	if v.Map == nil {
		v.Map = map[string]*Nbt{}
	}
	if _, had := v.Map[key]; !had {
		v.Keys = append(v.Keys, key)
	}
	v.Map[key] = value
}

// Get follows a dotted path through compounds, and is nil when a step is missing.
func (v *Nbt) Get(path string) *Nbt {
	cur := v
	for _, part := range strings.Split(path, ".") {
		if cur == nil || cur.Kind != NbtCompound {
			return nil
		}
		cur = cur.Map[part]
	}
	return cur
}

// OptString is the string at path, and false when it is absent or not a string.
func (v *Nbt) OptString(path string) (string, bool) {
	if n := v.Get(path); n != nil && n.Kind == NbtString {
		return n.Str, true
	}
	return "", false
}

// OptInt is the integer at path of any width, and false when it is absent or not one.
func (v *Nbt) OptInt(path string) (int64, bool) {
	if n := v.Get(path); n != nil {
		switch n.Kind {
		case NbtByte, NbtShort, NbtInt, NbtLong:
			return n.Int, true
		}
	}
	return 0, false
}

// OptFloat is the number at path, integer or floating, and false when it is absent.
func (v *Nbt) OptFloat(path string) (float64, bool) {
	if n := v.Get(path); n != nil {
		switch n.Kind {
		case NbtFloat, NbtDouble:
			return n.Float, true
		case NbtByte, NbtShort, NbtInt, NbtLong:
			return float64(n.Int), true
		}
	}
	return 0, false
}

// OptBool is the byte at path read as a boolean, and false in the second result when it is
// absent or not a 0 or 1 byte.
func (v *Nbt) OptBool(path string) (bool, bool) {
	if n := v.Get(path); n != nil && n.Kind == NbtByte && (n.Int == 0 || n.Int == 1) {
		return n.Int == 1, true
	}
	return false, false
}

// SNBT writes the value as SNBT. A floating value that is not finite is an error: SNBT has
// no spelling for it, and the host's parser refuses every field after one.
func (v *Nbt) SNBT() (string, error) {
	var b strings.Builder
	if err := v.write(&b); err != nil {
		return "", err
	}
	return b.String(), nil
}

func (v *Nbt) write(b *strings.Builder) error {
	switch v.Kind {
	case NbtByte:
		b.WriteString(strconv.FormatInt(v.Int, 10) + "b")
	case NbtShort:
		b.WriteString(strconv.FormatInt(v.Int, 10) + "s")
	case NbtInt:
		b.WriteString(strconv.FormatInt(v.Int, 10))
	case NbtLong:
		b.WriteString(strconv.FormatInt(v.Int, 10) + "L")
	case NbtFloat, NbtDouble:
		if math.IsNaN(v.Float) || math.IsInf(v.Float, 0) {
			return fmt.Errorf("SNBT has no spelling for %v", v.Float)
		}
		suffix := "d"
		if v.Kind == NbtFloat {
			suffix = "f"
		}
		b.WriteString(strconv.FormatFloat(v.Float, 'f', -1, 64) + suffix)
	case NbtString:
		b.WriteString(snbtQuote(v.Str))
	case NbtList:
		b.WriteByte('[')
		for i, e := range v.List {
			if i > 0 {
				b.WriteByte(',')
			}
			if err := e.write(b); err != nil {
				return err
			}
		}
		b.WriteByte(']')
	case NbtCompound:
		b.WriteByte('{')
		for i, k := range v.Keys {
			if i > 0 {
				b.WriteByte(',')
			}
			b.WriteString(snbtQuote(k) + ":")
			if err := v.Map[k].write(b); err != nil {
				return err
			}
		}
		b.WriteByte('}')
	case NbtByteArray, NbtIntArray, NbtLongArray:
		prefix, suffix := "[B;", "b"
		if v.Kind == NbtIntArray {
			prefix, suffix = "[I;", ""
		} else if v.Kind == NbtLongArray {
			prefix, suffix = "[L;", "L"
		}
		b.WriteString(prefix)
		for i, n := range v.Ints {
			if i > 0 {
				b.WriteByte(',')
			}
			b.WriteString(strconv.FormatInt(n, 10) + suffix)
		}
		b.WriteByte(']')
	default:
		return fmt.Errorf("an NBT value of unknown kind %d", v.Kind)
	}
	return nil
}

// snbtQuote quotes with only the escapes an SNBT parser reads back: strconv.Quote would
// write \x00 or \a, which no SNBT parser knows and which would come back as other text.
func snbtQuote(s string) string {
	var b strings.Builder
	b.WriteByte('"')
	for i := 0; i < len(s); i++ {
		c := s[i]
		switch c {
		case '"':
			b.WriteString("\\\"")
		case '\\':
			b.WriteString("\\\\")
		case '\n':
			b.WriteString("\\n")
		case '\r':
			b.WriteString("\\r")
		case '\t':
			b.WriteString("\\t")
		default:
			if c < 0x20 {
				fmt.Fprintf(&b, "\\u%04x", c)
			} else {
				b.WriteByte(c)
			}
		}
	}
	b.WriteByte('"')
	return b.String()
}

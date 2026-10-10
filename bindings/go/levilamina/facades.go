package levilamina

import "fmt"

// Player is one player, named by a selector. Most of its methods are generated from the
// constant tables of abi.h, one per property and verb; the rest are below.
type Player struct {
	Sel PlayerSel
}

// PlayerByName is the player with this name. A decision about permissions, money or
// ownership uses PlayerByXuid: a name can change hands.
func PlayerByName(name string) Player { return Player{Sel: ByName(name)} }

// PlayerByXuid is the player with this xuid.
func PlayerByXuid(xuid string) Player { return Player{Sel: ByXuid(xuid)} }

// PlayerByUuid is the player with this uuid.
func PlayerByUuid(uuid string) Player { return Player{Sel: ByUuid(uuid)} }

// Entity is one actor, named by its unique id.
type Entity struct {
	ID ActorID
}

// EntityByID is the actor with this unique id, the uid of event payloads.
func EntityByID(id ActorID) Entity { return Entity{ID: id} }

// BlockAt is one block cell of a dimension.
type BlockAt struct {
	Dim     int32
	X, Y, Z int32
}

// Block is the cell at x, y, z of dimension dim.
func Block(dim, x, y, z int32) BlockAt { return BlockAt{Dim: dim, X: x, Y: y, Z: z} }

// Item is an item stack as SNBT, a value: reading it asks the host about that SNBT, and
// changing it produces a new Item.
type Item struct {
	SNBT string
}

// ItemOf is the item stack these SNBT describe.
func ItemOf(snbt string) Item { return Item{SNBT: snbt} }

// noAnswer is the error of a read the host could not answer, kept apart from a zero or a
// false so that a caller never mistakes "unknown" for an answer.
func noAnswer(what string) error {
	return fmt.Errorf("the host has no answer for %s", what)
}

// refused is the error of a write or a verb the host refused.
func refused(what string) error {
	return fmt.Errorf("the host refused %s", what)
}

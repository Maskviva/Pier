package levilamina

import "fmt"

// okOr turns a slot's success flag into an error naming what was refused.
func okOr(ok bool, err error, what string) error {
	if err != nil {
		return err
	}
	if !ok {
		return refused(what)
	}
	return nil
}

// lastOr is the last string a slot sank, or the error of a read with no answer.
func lastOr(out []string, ok bool, err error, what string) (string, error) {
	if err != nil {
		return "", err
	}
	if !ok || len(out) == 0 {
		return "", noAnswer(what)
	}
	return out[len(out)-1], nil
}

// Server and world

// CurrentTick is the server's tick counter.
func CurrentTick() (uint64, error) { return Raw.GetCurrentTick() }

// TickDeltaTime is the length of the last tick, in seconds.
func TickDeltaTime() (float64, error) { return Raw.GetTickDeltaTime() }

// PlayerCount is how many players are online.
func PlayerCount() (int32, error) { return Raw.GetPlayerCount() }

// SimPaused reports whether the simulation is paused.
func SimPaused() (bool, error) { return Raw.GetSimPaused() }

// Tps is the ticks per second over the last windowSeconds. A negative answer, which the
// host gives before it has measured anything, is an error and not a rate.
func Tps(windowSeconds int32) (float64, error) {
	v, err := Raw.GetTps(windowSeconds)
	if err == nil && v < 0 {
		return 0, noAnswer("tps: nothing has been measured yet")
	}
	return v, err
}

// Mspt is the milliseconds per tick over the last windowSeconds, with Tps's rule.
func Mspt(windowSeconds int32) (float64, error) {
	v, err := Raw.GetMspt(windowSeconds)
	if err == nil && v < 0 {
		return 0, noAnswer("mspt: nothing has been measured yet")
	}
	return v, err
}

// Seed is the level seed.
func Seed() (int64, error) {
	v, ok, err := Raw.GetSeed()
	return v, okOr(ok, err, "the level seed")
}

// Difficulty is the level difficulty, 0 peaceful to 3 hard.
func Difficulty() (int32, error) {
	v, ok, err := Raw.GetDifficulty()
	return v, okOr(ok, err, "the difficulty")
}

// SetDifficulty sets the level difficulty.
func SetDifficulty(d int32) error {
	ok, err := Raw.SetDifficulty(d)
	return okOr(ok, err, "setting the difficulty")
}

// GameRule reads a game rule as text.
func GameRule(name string) (string, error) {
	out, ok, err := Raw.GameRuleGet(name)
	return lastOr(out, ok, err, "game rule "+name)
}

// SetGameRule writes a game rule from text.
func SetGameRule(name, value string) error {
	ok, err := Raw.GameRuleSet(name, value)
	return okOr(ok, err, "setting game rule "+name)
}

// Time is the time of day in ticks.
func Time() (int64, error) {
	v, ok, err := Raw.GetTime()
	return v, okOr(ok, err, "the time of day")
}

// SetTime sets the time of day in ticks.
func SetTime(t int64) error {
	ok, err := Raw.SetTime(t)
	return okOr(ok, err, "setting the time")
}

// SetWeather sets the weather: 0 clear, 1 rain, 2 thunder.
func SetWeather(weather int32) error {
	ok, err := Raw.SetWeather(weather)
	return okOr(ok, err, "setting the weather")
}

// SaveLevel saves the level now.
func SaveLevel() error {
	ok, err := Raw.LevelSave()
	return okOr(ok, err, "saving the level")
}

// Broadcast sends a message to every online player.
func Broadcast(msg string) error { return Raw.BroadcastMessage(msg) }

// Env reads an environment variable of the server process.
func Env(name string) (string, error) {
	out, ok, err := Raw.SysGetEnv(name)
	return lastOr(out, ok, err, "environment variable "+name)
}

// SetEnv sets an environment variable of the server process.
func SetEnv(name, value string) error {
	ok, err := Raw.SysSetEnv(name, value)
	return okOr(ok, err, "setting environment variable "+name)
}

// SpawnMob spawns a mob and returns it.
func SpawnMob(dim int32, typeName string, x, y, z float64) (Entity, error) {
	id, ok, err := Raw.SpawnMob(dim, typeName, x, y, z)
	return Entity{ID: id}, okOr(ok, err, "spawning "+typeName)
}

// SpawnParticle shows a particle effect to everyone near it.
func SpawnParticle(dim int32, effect string, x, y, z float64) error {
	ok, err := Raw.SpawnParticle(dim, effect, x, y, z)
	return okOr(ok, err, "spawning particle "+effect)
}

// ExplodeOptions are the optional parts of an explosion.
type ExplodeOptions struct {
	MaxResistance   float32
	Source          ActorID
	Fire            bool
	BreaksBlocks    bool
	AllowUnderwater bool
}

// Explode makes an explosion of the given radius.
func Explode(dim int32, x, y, z float64, radius float32, o ExplodeOptions) error {
	ok, err := Raw.Explode(dim, x, y, z, radius, o.MaxResistance, o.Source, o.Fire, o.BreaksBlocks, o.AllowUnderwater)
	return okOr(ok, err, "the explosion")
}

// DefaultSpawn is the level's world spawn.
func DefaultSpawn() (x, y, z int32, err error) {
	x, y, z, ok, err := Raw.LevelGetDefaultSpawn()
	return x, y, z, okOr(ok, err, "the world spawn")
}

// SetDefaultSpawn moves the level's world spawn.
func SetDefaultSpawn(x, y, z int32) error {
	ok, err := Raw.LevelSetDefaultSpawn(x, y, z)
	return okOr(ok, err, "setting the world spawn")
}

// SetBiome sets the biome of every column in a rectangle and returns how many changed.
func SetBiome(dim, minX, minZ, maxX, maxZ int32, biome string) (int32, error) {
	n, err := Raw.LevelSetBiome(dim, minX, minZ, maxX, maxZ, biome)
	if err == nil && n < 0 {
		return 0, refused("setting biome " + biome)
	}
	return n, err
}

// FillRegion fills a box with one block and returns how many cells changed.
func FillRegion(dim, x1, y1, z1, x2, y2, z2 int32, blockSpec string, updateFlags int32) (int64, error) {
	n, err := Raw.EditFillRegion(dim, x1, y1, z1, x2, y2, z2, blockSpec, updateFlags)
	if err == nil && n < 0 {
		return 0, refused("filling the region with " + blockSpec)
	}
	return n, err
}

// RegistryList lists one of the engine's registries, one of the PIER_REGISTRY_* kinds.
func RegistryList(kind int32) ([]string, error) {
	out, ok, err := Raw.RegistryList(kind)
	return out, okOr(ok, err, "the registry list")
}

// PlayerInfo is one online player as ListPlayers reports it. Dim and the position are
// read only when present, and HasDim and HasPos say whether they were.
type PlayerInfo struct {
	Name, Xuid, Uuid string
	Dim              int32
	HasDim           bool
	X, Y, Z          float64
	HasPos           bool
}

// ListPlayers lists the online players. An entry that does not parse is skipped with a
// warning, so one bad entry does not make who is online unanswerable.
func ListPlayers() ([]PlayerInfo, error) {
	raw, err := Raw.ListPlayers()
	if err != nil {
		return nil, err
	}
	out := make([]PlayerInfo, 0, len(raw))
	for _, text := range raw {
		v, perr := ParseSNBT(text)
		if perr != nil {
			logAt(levelWarn, "one entry of list_players could not be parsed and was skipped: "+perr.Error())
			continue
		}
		info := PlayerInfo{}
		info.Name, _ = v.OptString("name")
		info.Xuid, _ = v.OptString("xuid")
		info.Uuid, _ = v.OptString("uuid")
		if d, ok := v.OptInt("dim"); ok {
			info.Dim, info.HasDim = int32(d), true
		}
		x, okx := v.OptFloat("x")
		y, oky := v.OptFloat("y")
		z, okz := v.OptFloat("z")
		if okx && oky && okz {
			info.X, info.Y, info.Z, info.HasPos = x, y, z, true
		}
		out = append(out, info)
	}
	return out, nil
}

// Players

// Resolve is the player's actor. An error means nobody matches the selector.
func (p Player) Resolve() (Entity, error) {
	id, ok, err := Raw.PlayerResolve(p.Sel)
	if err != nil {
		return Entity{}, err
	}
	if !ok {
		return Entity{}, fmt.Errorf("player %s is offline, or the selector matches nobody", p.Sel.Value)
	}
	return Entity{ID: id}, nil
}

// IsOnline reports whether the selector matches an online player. Every ABI v2 host has the
// slot it asks, so false means nobody matches.
func (p Player) IsOnline() bool {
	_, err := p.Resolve()
	return err == nil
}

// SendMessage sends a chat line to the player.
func (p Player) SendMessage(msg string) error {
	ok, err := Raw.PlayerSendMessage(p.Sel, msg)
	return okOr(ok, err, "the message to "+p.Sel.Value)
}

// SendMessageTyped sends a message of a given TextPacket type to the player.
func (p Player) SendMessageTyped(msg string, kind int32) error {
	ok, err := Raw.PlayerSendMessageTyped(p.Sel, msg, kind)
	return okOr(ok, err, "the message to "+p.Sel.Value)
}

// SendTitle shows a title: slot 0 title, 1 subtitle, 2 action bar; the times are in ticks.
func (p Player) SendTitle(slot int32, text string, fadeIn, stay, fadeOut int32) error {
	ok, err := Raw.PlayerSendTitle(p.Sel, slot, text, fadeIn, stay, fadeOut)
	return okOr(ok, err, "the title for "+p.Sel.Value)
}

// Disconnect removes the player from the server with a reason.
func (p Player) Disconnect(reason string) error {
	ok, err := Raw.PlayerDisconnect(p.Sel, reason)
	return okOr(ok, err, "disconnecting "+p.Sel.Value)
}

// SetGameType sets the player's game mode, the value GameType reads.
func (p Player) SetGameType(mode int32) error {
	ok, err := Raw.PlayerSetGamemode(p.Sel, mode)
	return okOr(ok, err, "the game mode of "+p.Sel.Value)
}

// Teleport moves the player to a position of any registered dimension.
func (p Player) Teleport(dim int32, x, y, z float64) error {
	ok, err := Raw.PlayerTeleport(p.Sel, dim, x, y, z)
	return okOr(ok, err, "teleporting "+p.Sel.Value)
}

// CarriedItem is the item in the player's hand.
func (p Player) CarriedItem() (Item, error) {
	out, ok, err := Raw.PlayerGetCarriedItem(p.Sel)
	s, err := lastOr(out, ok, err, "the carried item of "+p.Sel.Value)
	return Item{SNBT: s}, err
}

// InventoryItem is the item in one inventory slot.
func (p Player) InventoryItem(slot int32) (Item, error) {
	out, ok, err := Raw.PlayerGetItem(p.Sel, slot)
	s, err := lastOr(out, ok, err, fmt.Sprintf("slot %d of %s", slot, p.Sel.Value))
	return Item{SNBT: s}, err
}

// SetInventoryItem puts an item into one inventory slot.
func (p Player) SetInventoryItem(slot int32, item Item) error {
	ok, err := Raw.PlayerSetItem(p.Sel, slot, item.SNBT)
	return okOr(ok, err, fmt.Sprintf("slot %d of %s", slot, p.Sel.Value))
}

// ConnID is the player's network connection id.
func (p Player) ConnID() (uint64, error) { return Raw.PlayerConnId(p.Sel) }

// Inventory is the player's inventory as a container.
func (p Player) Inventory() Container {
	return Container{Ref: ContainerRef{Which: ContainerInventory, Player: p.Sel}}
}

// EnderChest is the player's ender chest as a container.
func (p Player) EnderChest() Container {
	return Container{Ref: ContainerRef{Which: ContainerEnderChest, Player: p.Sel}}
}

// Armor is the player's armor slots as a container.
func (p Player) Armor() Container {
	return Container{Ref: ContainerRef{Which: ContainerArmor, Player: p.Sel}}
}

// Offhand is the player's offhand slot as a container.
func (p Player) Offhand() Container {
	return Container{Ref: ContainerRef{Which: ContainerOffhand, Player: p.Sel}}
}

// Entities

// Snapshot is the actor's full NBT as SNBT.
func (e Entity) Snapshot() (string, error) {
	out, ok, err := Raw.ActorSnapshot(e.ID)
	return lastOr(out, ok, err, fmt.Sprintf("the snapshot of actor %d", e.ID))
}

// Vehicle is what the actor rides.
func (e Entity) Vehicle() (Entity, error) {
	id, ok, err := Raw.ActorGetVehicle(e.ID)
	return Entity{ID: id}, okOr(ok, err, fmt.Sprintf("the vehicle of actor %d", e.ID))
}

// Owner is the actor's owner, for a tamed or summoned one.
func (e Entity) Owner() (Entity, error) {
	id, ok, err := Raw.ActorGetOwner(e.ID)
	return Entity{ID: id}, okOr(ok, err, fmt.Sprintf("the owner of actor %d", e.ID))
}

// Target is the actor's current target.
func (e Entity) Target() (Entity, error) {
	id, ok, err := Raw.ActorGetTarget(e.ID)
	return Entity{ID: id}, okOr(ok, err, fmt.Sprintf("the target of actor %d", e.ID))
}

// DistanceTo is the distance to another actor.
func (e Entity) DistanceTo(other Entity) (float64, error) {
	d, ok, err := Raw.ActorDistanceTo(e.ID, other.ID)
	return d, okOr(ok, err, "the distance between two actors")
}

// Clone copies the actor to a position and returns the copy.
func (e Entity) Clone(dim int32, x, y, z float64) (Entity, error) {
	id, ok, err := Raw.ActorClone(e.ID, dim, x, y, z)
	return Entity{ID: id}, okOr(ok, err, fmt.Sprintf("cloning actor %d", e.ID))
}

// Blocks

// Info reads the block: its type name and state.
func (b BlockAt) Info() (BlockInfo, error) { return GetBlock(b.Dim, b.X, b.Y, b.Z) }

// Set places a block from a block spec, as /setblock reads one.
func (b BlockAt) Set(blockSpec string) error {
	ok, err := Raw.SetBlock(b.Dim, b.X, b.Y, b.Z, blockSpec)
	return okOr(ok, err, "placing "+blockSpec)
}

// State reads one block state by name.
func (b BlockAt) State(name string) (string, error) {
	out, ok, err := Raw.BlockGetState(b.Dim, b.X, b.Y, b.Z, name)
	return lastOr(out, ok, err, "block state "+name)
}

// SetState writes one block state by name.
func (b BlockAt) SetState(name, value string) error {
	ok, err := Raw.BlockSetState(b.Dim, b.X, b.Y, b.Z, name, value)
	return okOr(ok, err, "block state "+name)
}

// EntitySNBT is the block entity at the cell, such as a chest's contents, as SNBT.
func (b BlockAt) EntitySNBT() (string, error) {
	out, ok, err := Raw.BlockEntitySnbt(b.Dim, b.X, b.Y, b.Z)
	return lastOr(out, ok, err, "the block entity")
}

// Biome is the biome at the cell.
func (b BlockAt) Biome() (string, error) {
	out, ok, err := Raw.LevelGetBiome(b.Dim, b.X, b.Y, b.Z)
	return lastOr(out, ok, err, "the biome")
}

// Container is the cell's block container, such as a chest.
func (b BlockAt) Container() Container {
	return Container{Ref: ContainerRef{Which: ContainerBlock, Dim: b.Dim, X: b.X, Y: b.Y, Z: b.Z}}
}

// Items

// Payload is the item's SNBT parsed.
func (i Item) Payload() (*Nbt, error) { return ParseSNBT(i.SNBT) }

// Enchants lists the item's enchantments as SNBT.
func (i Item) Enchants() (string, error) {
	out, ok, err := Raw.ItemGetEnchants(i.SNBT)
	return lastOr(out, ok, err, "the enchantments")
}

// Matches reports whether two items stack: the same item, data and user data.
func (i Item) Matches(other Item) (bool, error) { return Raw.ItemMatches(i.SNBT, other.SNBT) }

// Transform applies one of the PIER_IOP_* operations of abi.h and returns the new item.
func (i Item) Transform(op int32, sarg string, narg float64) (Item, error) {
	out, ok, err := Raw.ItemTransform(i.SNBT, op, sarg, narg)
	s, err := lastOr(out, ok, err, "the item transform")
	return Item{SNBT: s}, err
}

// Containers

// Container is a player's or a block's container.
type Container struct {
	Ref ContainerRef
}

// Size is the number of slots.
func (c Container) Size() (int32, error) {
	n, ok, err := Raw.ContainerSize(c.Ref)
	return n, okOr(ok, err, "the container size")
}

// Item is the item in one slot.
func (c Container) Item(slot int32) (Item, error) {
	out, ok, err := Raw.ContainerGetItem(c.Ref, slot)
	s, err := lastOr(out, ok, err, fmt.Sprintf("container slot %d", slot))
	return Item{SNBT: s}, err
}

// SetItem puts an item into one slot.
func (c Container) SetItem(slot int32, item Item) error {
	ok, err := Raw.ContainerSetItem(c.Ref, slot, item.SNBT)
	return okOr(ok, err, fmt.Sprintf("container slot %d", slot))
}

// AddItem adds an item where it fits.
func (c Container) AddItem(item Item) error {
	ok, err := Raw.ContainerAddItem(c.Ref, item.SNBT)
	return okOr(ok, err, "adding to the container")
}

// RemoveItem removes count items from one slot.
func (c Container) RemoveItem(slot, count int32) error {
	ok, err := Raw.ContainerRemoveItem(c.Ref, slot, count)
	return okOr(ok, err, fmt.Sprintf("removing from container slot %d", slot))
}

// Clear empties the container.
func (c Container) Clear() error {
	ok, err := Raw.ContainerClear(c.Ref)
	return okOr(ok, err, "clearing the container")
}

// Items lists the occupied slots.
func (c Container) Items() ([]SlotItem, error) { return ContainerItems(c.Ref) }

// Key-value stores

// KvDb is an open key-value store, kept by the host under the mod's data directory.
type KvDb struct {
	h    KvDbHandle
	path string
}

// OpenKvDb opens the store at path, creating it when createIfMissing is set.
func OpenKvDb(path string, createIfMissing bool) (*KvDb, error) {
	h, err := Raw.KvdbOpen(path, createIfMissing)
	if err != nil {
		return nil, err
	}
	if h.IsZero() {
		return nil, fmt.Errorf("the key-value store %s could not be opened", path)
	}
	return &KvDb{h: h, path: path}, nil
}

// Path is where the store was opened.
func (db *KvDb) Path() string { return db.path }

// Get reads one key; found is false for a key that does not exist. A store the host closed
// on its own, at an unload, also reads as not found: the ABI answers both alike.
func (db *KvDb) Get(key string) (value string, found bool, err error) {
	out, ok, err := Raw.KvdbGet(db.h, key)
	if err != nil || !ok || len(out) == 0 {
		return "", false, err
	}
	return out[len(out)-1], true, nil
}

// Set writes one key.
func (db *KvDb) Set(key, value string) error {
	ok, err := Raw.KvdbSet(db.h, key, value)
	return okOr(ok, err, "writing key "+key)
}

// Delete removes one key.
func (db *KvDb) Delete(key string) error {
	ok, err := Raw.KvdbDel(db.h, key)
	return okOr(ok, err, "deleting key "+key)
}

// Has reports whether a key exists.
func (db *KvDb) Has(key string) (bool, error) { return Raw.KvdbHas(db.h, key) }

// IsEmpty reports whether the store holds no key.
func (db *KvDb) IsEmpty() (bool, error) { return Raw.KvdbIsEmpty(db.h) }

// Iter lists every entry.
func (db *KvDb) Iter() ([]KeyValue, error) { return kvdbIter(db.h) }

// Close closes the store; using it afterwards fails.
func (db *KvDb) Close() error {
	err := Raw.KvdbClose(db.h)
	db.h = KvDbHandle{}
	return err
}

// Economy

// Money is a balance. A negative answer means it cannot be read, an empty xuid or no
// economy backend, and is an error: a real balance is never negative.
func Money(xuid string) (int64, error) {
	v, err := Raw.GetMoney(xuid)
	if err == nil && v < 0 {
		return 0, noAnswer("the balance of " + xuid)
	}
	return v, err
}

// SetMoney sets a balance.
func SetMoney(xuid string, amount int64) error {
	ok, err := Raw.SetMoney(xuid, amount)
	return okOr(ok, err, "setting the balance of "+xuid)
}

// AddMoney adds to a balance.
func AddMoney(xuid string, delta int64) error {
	ok, err := Raw.AddMoney(xuid, delta)
	return okOr(ok, err, "adding to the balance of "+xuid)
}

// ReduceMoney takes from a balance.
func ReduceMoney(xuid string, delta int64) error {
	ok, err := Raw.ReduceMoney(xuid, delta)
	return okOr(ok, err, "reducing the balance of "+xuid)
}

// TransferMoney moves value between two balances; the recipient receives it taxed.
func TransferMoney(from, to string, value int64, note string) error {
	ok, err := Raw.TransMoney(from, to, value, note)
	return okOr(ok, err, fmt.Sprintf("transferring %d from %s to %s", value, from, to))
}

// Simulated players

// SimSpawn spawns a simulated player.
func SimSpawn(name string, dim int32, x, y, z float64) error {
	ok, err := Raw.SimSpawn(name, dim, x, y, z)
	return okOr(ok, err, "spawning simulated player "+name)
}

// SimDo runs one verb on a simulated player; args is SNBT, "{}" when there is none.
func SimDo(name, verb, args string) error {
	ok, err := Raw.SimDo(ByName(name), verb, args)
	return okOr(ok, err, "verb "+verb+" of "+name)
}

// IsSimulated reports whether name is a live simulated player.
func IsSimulated(name string) (bool, error) { return Raw.SimIs(ByName(name)) }

// SimList lists the live simulated players.
func SimList() ([]string, error) { return Raw.SimList() }

// Custom dimensions

// DimensionsAvailable reports whether this host can register custom dimensions.
func DimensionsAvailable() (bool, error) { return Raw.MdIsAvailable() }

// AddDimension registers a dimension from spec SNBT and returns its id.
func AddDimension(name, spec string) (int32, error) {
	id, err := Raw.MdAddDimension(name, spec)
	if err == nil && id < 0 {
		return 0, refused("dimension " + name)
	}
	return id, err
}

// DimensionID is the id of a registered dimension.
func DimensionID(name string) (int32, error) {
	id, err := Raw.MdGetDimensionId(name)
	if err == nil && id < 0 {
		return 0, fmt.Errorf("no dimension is named %s", name)
	}
	return id, err
}

// ListDimensions lists the registered custom dimensions.
func ListDimensions() ([]string, error) { return Raw.MdListDimensions() }

// SetDimensionRule sets one rule, a PIER_DIMRULE_* value, for one dimension.
func SetDimensionRule(dim, rule int32, allow bool) error { return Raw.MdSetDimensionRule(dim, rule, allow) }

// DimensionRule reads one rule; set is false when the dimension follows vanilla for it,
// which a host older than the rule also answers.
func DimensionRule(dim, rule int32) (allow, set bool, err error) {
	allow, set, err = Raw.MdGetDimensionRule(dim, rule)
	return allow, set, err
}

// Scoreboards

// Scoreboard runs one PIER_SB_* operation and returns its output, when it has one.
func Scoreboard(op int32, a, b string, n int64) (string, error) {
	out, ok, err := Raw.ScoreboardOp(op, a, b, n)
	if err != nil {
		return "", err
	}
	if !ok {
		return "", refused("the scoreboard operation")
	}
	if len(out) == 0 {
		return "", nil
	}
	return out[len(out)-1], nil
}

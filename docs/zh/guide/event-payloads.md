# 事件载荷

不管模组用什么语言写，每个事件到达模组时都是一段 SNBT 文本。这一页是这段文本里有什么的参考。
怎么订阅看各语言的指南；字段在哪门语言里都一样。

## 两类事件

**LeviLamina 事件**是 LeviLamina 自己事件注册表里的事件，用完整 id 订阅，比如 `ll::event::MobDieEvent`。
载荷是 LeviLamina 对事件的序列化，Pier 在上面补充字段（见下文），因为那份序列化只写出了引擎对象的名字，没有描述它们。

**合成事件**是 Pier 从自己的 hook 里发出的事件，比如 `PlayerAttackTargetEvent`。
载荷由 Pier 写出，每个事件的字段在各绑定的事件名列表里逐一说明。

## Pier 给 LeviLamina 事件补了什么

载荷里的引擎对象会在旁边被描述出来，键名是同一个键前面加下划线。
出现的字段都是宿主读到的。读不到的字段，要么不出现，要么被列进 `_unresolved`，Pier 不会替它填一个默认值。

| 载荷里有 | Pier 补充 | 形状 |
|---|---|---|
| 事件所关于的玩家（`self`） | `_player` | `{name, xuid, uuid, pos:{x,y,z}}` |
| 其他任何实体，或不是玩家的 `self` | `_<键名>` | `{uid, type, name, isPlayer, dim}`，玩家还多 `xuid` 和 `realName` |
| 伤害来源 | `_<键名>` | `{cause, attackerUid?, attacker?, projectileUid?, projectile?}` |
| 实体类型 | `_identifier` | `{full, namespace, name}` |
| 方块或物品 | `_<键名>` | `{name}` |
| BlockSource，或 `self` 实体 | `dim` | 维度 id，事件本身没带时补上 |

`uid` 是实体的唯一 id，和所有实体槽位接收的是同一个数，所以模组可以对载荷里点名的实体直接操作。
`name` 是名牌，可能被插件改过；`realName` 是玩家的账号名。

### 谁杀了谁

`MobDieEvent`、`PlayerDieEvent` 和 `ActorHurtEvent` 都带伤害来源。伤害来自实体时，来源里有：

- `attackerUid`：负责的那个实体。投射物伤害时是发射它的实体，不是箭本身。
- `attacker`：那个实体的描述，它还存在时才有。
- `projectileUid` 和 `projectile`：箭、三叉戟或其他投射物，只在投射物伤害时有。

即使实体已经不在了，uid 也会写出来，所以射手下线后被箭射死，仍然知道是谁干的。
因此没有 `attacker` 表示"已经不在了"；没有 `attackerUid` 才表示伤害根本不是实体造成的：火、摔落、虚空。
两种情况下 `cause` 都是引擎的伤害原因编号。

### 有东西解析不出来的时候

既不是在线玩家、也不在运行时实体表里的实体无法描述。它的键名会被列进 `_unresolved`，事件照常到达。
基于这种载荷做保护决定的模组应当拒绝，而不是猜；Rust SDK 的 `check_complete()` 和 `dim()` 已经替你这样做了。

## 合成事件的约定

合成事件的载荷遵守三条规则，让模组能分清"答案"和"失败"：

- **`partial`**：`"partial":1` 表示至少有一个字段没读到、只是默认值。出现在 `ExplosionEvent`、
  `FarmlandDecayEvent`、`SpawnItemActorEvent`、`ChestPairEvent` 和 `PistonPushEvent` 里，全部读到时不出现。
- **`targetKnown`**：在 `PlayerAttackTargetEvent` 和 `PlayerInteractEntityEvent` 里，0 表示目标没读到。
  这时 `targetIsPlayer` 为 1，所以 PvP 规则会拒绝。
- **取消**：回调会拿到一个写回接口，可取消的事件通过写回时带上取消标记来取消；各绑定都包装好了，Rust 里是 `ev.cancel()`。
  标为只观察的事件不能取消，它的文档会指出该去拦哪个事件。

## 事件没到的时候

- 事件可能被关掉了：`config.json` 的 `hooks.disabled` 里列出的合成事件会拒绝订阅，并记一行说明。见[配置](configuration.md)。
- 引擎 hook 点变了的 LeviLamina 事件，在新引擎版本上可能不再到达；怎么判断见[排错](troubleshooting.md)。

## c4fee72d52

>  `levilamina`: writing LeviLamina mods in safe Rust.
>
>  ```ignore
>  struct Hello;
>  impl LeviMod for Hello {
>      fn on_load(ctx: &ModContext) -> Result<Self> { Ok(Hello) }
>  }
>  levilamina::register_mod!(Hello);
>  ```
>
>  On top of `levilamina_sys`, the cell-for-cell mirror of `sdk/abi.h`, it does four things
>  and no more: the two slot gates, string handling, the panic fence, and copying inside a
>  sink (contract §3).
>  It caches no host state, feigns no synchronization and covers for the host in nothing: a
>  failure is an `Err` that says why and never quietly becomes a default (§5.1).

`levilamina`：用安全的 Rust 写 LeviLamina 模组。

```rust
struct Hello;
impl LeviMod for Hello {
    fn on_load(ctx: &ModContext) -> Result<Self> { Ok(Hello) }
}
levilamina::register_mod!(Hello);
```

它建立在 `levilamina_sys`（`sdk/abi.h` 的逐格镜像）之上，只做四件事：两道槽位关卡、字符串处理、panic 围栏，以及在输出回调里复制数据（契约 §3）。它不缓存宿主的状态，不假装做同步，也不替宿主掩盖任何问题：失败就是一个说明原因的 `Err`，永远不会悄悄变成默认值（§5.1）。

## 915f1c464a

> One `use levilamina::prelude::*;` brings in what writing a mod needs most.

一句 `use levilamina::prelude::*;` 就能引入写模组最常用的东西。

## 41e708231b

>  The context handed to a mod's lifecycle callbacks.
>
>  It carries no state of its own, since the real state lives in `RUNTIME`. It exists to
>  give the facades one common entry point, and to make a signature such as
>  `on_load(ctx)` read sensibly.

交给模组生命周期回调的上下文。

它自己不带任何状态，真正的状态在 `RUNTIME` 里。它的存在是为了给各个门面一个共同的入口，也让 `on_load(ctx)` 这样的签名读起来自然。

## daee979f98

>  Capabilities at the host and system level: the run stage, scheduling, executing
>  commands and the protocol version.

宿主和系统层面的能力：运行阶段、调度、执行命令和协议版本。

## 83324e7f7c

>  The packet facade.

数据包门面。

## d046aa325c

>  The world facade.

世界门面。

## 5ed0dac114

>  Server runtime control: freezing and warping ticks, and performance sampling.

服务器运行时的控制：冻结和加速刻，以及性能采样。

## d7c050b430

>  Whether the host was built for the client target.
>
>  Rarely needed, since a mod loaded onto the wrong target is refused by the host during
>  the handshake. It exists so that one source can make a small behavioral distinction
>  between the two targets without a compile-time feature.

宿主是不是为客户端目标构建的。

很少需要用到：模组被加载到错误的目标上时，宿主会在握手时拒绝它。它的用处是让同一份源码不靠编译期特性，就能在两个目标之间做一点行为上的区分。

## 3768d270b5

>  The ABI version and table length of the host. For diagnostics: reporting that a pier is
>  too old for a feature only tells someone how far to upgrade when these two numbers come
>  with it.

宿主的 ABI 版本和表长。用于诊断：报告「pier 太旧，不支持某个功能」时，附上这两个数，看的人才知道要升级到什么程度。

## 1181cc65f3

>  One Pier mod.
>
>  `Send` is what makes `ModSlot<T>`, a `Mutex<Option<T>>`, genuinely `Sync`: a lifecycle
>  callback may be entered on a different thread, since the host allows `unload` and a
>  cross-mod service call to come from another thread.

一个 Pier 模组。

`Send` 让 `ModSlot<T>`（一个 `Mutex<Option<T>>`）真正满足 `Sync`：生命周期回调可能在另一个线程上进入，因为宿主允许 `unload` 和跨模组的服务调用来自别的线程。

## ed543cab54

>  Load. An `Err` makes the host treat loading as failed and roll back, with the teardown
>  steps running as usual.

加载。返回 `Err` 时，宿主把这次加载视为失败并回滚，拆除的步骤照常运行。

## 350fd54a15

>  The ticket of a scheduled task.

一个定时任务的票据。

## d0c9380d10

>  Why one ABI call failed. The message is meant for a human and can go straight into a
>  log.

一次 ABI 调用失败的原因。消息是写给人看的，可以直接记进日志。

## 91522ed55c

>  Mirrors `ll::io::LogLevel`. The values are part of the ABI.

对应 `ll::io::LogLevel`。取值属于 ABI。

## 10e28036db

>  Generates the `pier_main` entry point. Written once per mod.
>
>  ```ignore
>  struct MyMod;
>  impl LeviMod for MyMod { /* ... */ }
>  levilamina::register_mod!(MyMod);
>  ```

生成 `pier_main` 入口点。每个模组写一次。

```rust
struct MyMod;
impl LeviMod for MyMod { /* ... */ }
levilamina::register_mod!(MyMod);
```

## 43c7d19f9c

>  Both gates. Missing either returns an `Err` that says what is missing.
>
>  Usage: at the top of a function body,
>  `require_slot!(md_add_dimension, "creating a dimension");`
>
>  The message carries no historical product name (contract §7). An earlier one read
>  "...Update levilamina-rust-loader" while no mod of that name exists any more, so anyone
>  following it finds nothing, which is exactly the shape §5.3 opposes when it says a log
>  line has to answer what to do about it.

两道关卡都检查。缺任何一道，都返回一个说明缺了什么的 `Err`。

用法：写在函数体开头，`require_slot!(md_add_dimension, "creating a dimension");`

消息里不带任何历史产品名（契约 §7）。以前有一版写着「...Update levilamina-rust-loader」，而这个名字的模组早就不存在了，照着去找的人什么都找不到；§5.3 要求日志回答「该怎么办」，反对的正是这种写法。

## 1ec9aaaf9c

>  Only asks whether it exists and returns no `Err`. For code that uses it when present
>  and degrades otherwise.
>
>  Both gates again: long enough and non-null counts as present.

只问槽位在不在，不返回 `Err`。用于有就用、没有就降级的代码。

同样检查两道关卡：表够长并且不为空，才算存在。

## 7c39d7f4df

>  Players: addressed by selector and resolved again on every call.
>
>  # Only an xuid may be used as a key
>
>  When `PlayerSel::Name` matches no account name on the host side it falls back to the display
>  name, which another mod can change. A player setting their display name to the account name of
>  an offline player redirects every by-name call onto themselves. Permission, economy and
>  ownership decisions all use [`Player::by_xuid`]; see the module documentation of [`crate::sel`].
>
>  # A player is an actor too
>
>  [`Player::as_entity`] goes through `player_resolve` for an `ActorUniqueID`, after which the
>  whole of [`crate::entity::Entity`] applies. The two APIs are complementary: player-specific
>  things such as inventory, ability bits, titles and kicking are here, and actor-general ones such
>  as health, teleporting and tags are there.

玩家：用选择器指定，每次调用都重新解析。

**只有 xuid 能当作键**

`PlayerSel::Name` 在宿主那边对不上任何账号名时，会退回去匹配显示名，而显示名是别的模组可以改的。一名玩家把自己的显示名改成某个离线玩家的账号名，所有按名字的调用就都落到了他身上。权限、经济和归属的判断都要用 [`Player::by_xuid`]；见 [`crate::sel`] 的模块文档。

**玩家也是实体**

[`Player::as_entity`] 经 `player_resolve` 取得 `ActorUniqueID`，之后 [`crate::entity::Entity`] 的全部能力都可以用。两套接口互相补充：物品栏、能力位、标题、踢出这类玩家特有的在这里，生命值、传送、标签这类实体通用的在那里。

## 5728b33325

>  One player. It holds a selector and no pointer.

一名玩家。它只存着选择器，不存指针。

## 1f7d9ccb52

>  By name. This goes through the display-name fallback, so identity uses
>  [`Player::by_xuid`].

按名字指定。这会经过显示名的回退，所以认人要用 [`Player::by_xuid`]。

## f932bf9597

>  By xuid: unique, unforgeable and unchangeable by the player.

按 xuid 指定：唯一，伪造不了，玩家自己也改不了。

## a99c1587cd

>  The list of online players.
>
>  The host sinks one SNBT per player. An unparsable entry is skipped with a warning
>  rather than emptying the whole table: one bad entry should not make who is on the
>  server unanswerable. Every ABI v2 host fills `list_players`, server and client alike,
>  so an empty list means that nobody is online.

在线玩家的列表。

宿主每个玩家输出一段 SNBT。解析不了的条目会被跳过并记一条警告，不会让整张表变空：一条坏数据不应该让「服务器上有谁」变得答不上来。每个 ABI v2 宿主都提供 `list_players`，服务器和客户端都一样，所以列表为空就表示没有人在线。

## 9f16382f4c

>  Sends one message to every online player.

给每个在线玩家发送一条消息。

## 2622b1305b

>  Whether this selector currently resolves to anyone. Every ABI v2 host fills
>  `player_resolve`, server and client alike, so false here means nobody matches, never
>  a host that cannot tell.

这个选择器当前能不能解析到某个人。每个 ABI v2 宿主都提供 `player_resolve`，服务器和客户端都一样，所以这里的 false 表示没有人对得上，不会是宿主分辨不出来。

## 80678c1a30

>  Uses it as an actor, giving the full set of [`Entity`] capabilities.

把玩家当作实体使用，获得 [`Entity`] 的全部能力。

## 03d7657c1a

>  Reads a `PIER_PPROP_*` numeric property.

读取一个 `PIER_PPROP_*` 数值属性。

## 3da275aabf

>  Writes a `PIER_PPROP_*` numeric property. Only the ones marked (S) are writable.

写入一个 `PIER_PPROP_*` 数值属性。只有标着 (S) 的才能写。

## fd067f62d7

>  Reads a `PIER_PSTR_*` string property.

读取一个 `PIER_PSTR_*` 字符串属性。

## 9d9abb9b16

>  The position of the last death. Never having died gives `Ok(None)`, since the host
>  sends an empty string.

上一次死亡的位置。从来没死过时返回 `Ok(None)`，因为宿主给的是空字符串。

## c736695361

>  The experience bar progress, from 0 to 1.

经验条的进度，从 0 到 1。

## 80c9b9f9cc

>  The position. It goes through a dedicated slot rather than a property number, because
>  one call gives all three axes plus the dimension, while a player may have moved between
>  three separate property calls.

位置。它走的是专用的槽位，没有用属性编号：一次调用就拿到三个坐标和维度，分三次读属性的话，玩家可能在两次调用之间移动了。

## 837ff10ef4

>  The network status in detail.

详细的网络状态。

## 3fed960b81

>  The connection id of this player, the same number a packet interceptor sees.
>
>  A 0 means offline or no network identity available. On the ABI 0 is not a valid
>  connection id, so this reports it truthfully as an `Err` rather than handing over a 0.

这名玩家的连接 id，和数据包拦截器看到的是同一个数。

0 表示不在线，或者拿不到网络标识。在 ABI 上 0 不是有效的连接 id，所以这里如实地把它报告成 `Err`，不交出一个 0。

## e2fc461deb

>  Teleports. A custom dimension, with an id of 3 or above, goes through here too, and
>  when the dimension bridge cannot build a matching instance the host fails rather than
>  dropping the person into a mismatched dimension.

传送。id 为 3 及以上的自定义维度也走这里；维度桥接构造不出对得上的实例时，宿主会让调用失败，不会把人丢进一个对不上的维度。

## 554cf2320a

>  Runs a `PIER_PACT_*` action and returns its output.

执行一个 `PIER_PACT_*` 动作，返回它的输出。

## 3f43e74ab8

>  Sets one ability bit.
>
>  Passing a boolean ability where a floating-point one belongs raises no error and is
>  simply written under the other interpretation, so [`Ability::is_float`] stops it first.

设置一个能力位。

在浮点类的能力上传布尔值不会报错，只会按另一种解释写进去，所以 [`Ability::is_float`] 会先把它拦下来。

## b72a3079f6

>  Sets an ability bit by index, for when the host is newer than this layer and has extra
>  bits.

按下标设置能力位，用于宿主比这一层新、多出了能力位的情况。

## 03f7ab5f36

>  A per-player sidebar. `lines` runs from top to bottom.

只对这名玩家显示的侧边栏。`lines` 从上到下排列。

## 59f7c9f3bc

>  Sends one with a given `TextPacketType`.

按指定的 `TextPacketType` 发送一条消息。

## 1a73114681

>  Sends one title.
>
>  It goes through a real `SetTitlePacket` and not an assembled `/title` command, which
>  would paste the text into a command line verbatim and turn a plot whose name contains a
>  quote or an `@e` into a command injection.
>
>  A given `times` sends a Times packet first so the timing is definite, and omitting it
>  reuses the durations the client stored last. The three durations cannot be given in
>  part, a combination the host refuses outright.

发送一个标题。

它通过真正的 `SetTitlePacket` 发送，没有拼 `/title` 命令：拼命令会把文本原样塞进命令行，名字里带引号或者 `@e` 的地皮就成了一次命令注入。

给了 `times` 时会先发一个 Times 包，计时是确定的；省略它则沿用客户端上一次存下的时长。三个时长不能只给一部分，那样的组合宿主会直接拒绝。

## 71b6425b73

>  Spawns a particle for this one player only.
>
>  Unlike `World::spawn_particle`, nobody else sees it, since that one broadcasts across
>  the whole dimension. Something like a selection highlight has to use this one, otherwise
>  the whole server sees it.

只为这一名玩家生成粒子。

`World::spawn_particle` 会向整个维度广播，这个则只有这名玩家看得到。选区高亮这类东西必须用这个，否则全服务器都看得到。

## 8cb670e647

>  Pushes a raw packet onto the connection of this player.
>
>  An escape hatch: `body` is the wire format of the current game version and has to
>  follow every version change, which is the caller's responsibility. A named entry point
>  is preferred wherever one exists.

把一个原始数据包推到这名玩家的连接上。

这是一个逃生口：`body` 是当前游戏版本的线上格式，每次版本变化都要跟着改，这由调用方负责。有命名的接口时，请优先用那些。

## abeae426ae

>  The item in the off hand. An empty hand gives the air item and is not an error.

副手里的物品。空手给出的是空气物品，不算错误。

## bc742ab839

>  Writes the off hand. Remember [`Container::refresh`] afterwards, otherwise the client
>  keeps showing the old item.

写入副手。之后记得调用 [`Container::refresh`]，否则客户端会一直显示旧的物品。

## 3af217a10d

>  The item in hand.

手里拿着的物品。

## 29edb71f32

>  One inventory slot.

物品栏的某一格。

## 02113d5349

>  The full equipment set. For the `slot` numbering see [`crate::types::EquipSlot`].

全部装备。`slot` 的编号见 [`crate::types::EquipSlot`]。

## be99440f5c

>  How many ticks of cooldown one item has left.
>
>  On the ABI a -1 means both not on cooldown and the player being offline. It is handed
>  over unchanged and stated rather than guessed at on the caller's behalf
>  (contract §5.2). Telling them apart starts with
>  [`Player::is_online`].

某个物品的冷却还剩多少刻。

在 ABI 上，-1 同时表示不在冷却中和玩家不在线。这里原样交出，并把这一点写明，不替调用方去猜（契约 §5.2）。要区分这两种情况，先调用 [`Player::is_online`]。

## ff8155b615

>  The account name.

账号名。

## 9343f3295c

>  `address:port`. IPv6 has the same shape, so it must not be split on the last colon.

`地址:端口`。IPv6 也是这个形状，所以不能按最后一个冒号去切分。

## 1c50bd42ea

>  `{x,y,z,dim}` as SNBT: where this player would respawn. Never empty -- a player
>  with no bed reports the world spawn, and the two cannot be told apart.

以 SNBT `{x,y,z,dim}` 给出这名玩家会在哪里重生。不会为空：没有床的玩家报告的是世界出生点，这两种情况区分不开。

## 5af55fbce2

>  The name shown above the head, which can be changed. An identity decision uses
>  [`Player::xuid`].

显示在头顶上的名字，可以被修改。认人要用 [`Player::xuid`]。

## 60268ba6ec

>  The experience level.

经验等级。

## c724d24084

>  The progress of the experience bar, from 0 to 1. It is not the accumulated experience.

经验条的进度，从 0 到 1。它表示的并非累计的经验值。

## eb3d492f7a

>  The saturation; hunger only starts dropping once it is spent.

饱和度；它耗尽以后饥饿值才开始下降。

## acdf7a0ca2

>  The exhaustion; filling one unit costs a point of saturation.

消耗度；每攒满一个单位，扣掉一点饱和度。

## 52bfa53b9d

>  How much experience remains to the next level.

距离下一级还差多少经验。

## 6a14e2bfe7

>  The `score` pseudo-objective of the scoreboard, not any custom objective.

计分板上的 `score` 伪计分项，和任何自定义的计分项无关。

## 25afff51f0

>  The view distance the client requested, in chunks.

客户端请求的视距，单位是区块。

## 6c6b0f5d70

>  The value of `BuildPlatform`, not an operating system name.

`BuildPlatform` 的值，并非操作系统的名字。

## 18633b6064

>  The facing: 0 is south, 1 west, 2 north and 3 east.

朝向：0 南，1 西，2 北，3 东。

## 19b37d7656

>  The round-trip latency in milliseconds. For the detail see [`Player::network_status`].

往返延迟，单位毫秒。详细情况见 [`Player::network_status`]。

## e5594d4356

>  Listed as an operator in `permissions.json`. Not the same thing as
>  [`Player::permission_level`], which can change at runtime.

在 `permissions.json` 里被列为管理员。它和 [`Player::permission_level`] 是两回事，后者在运行时可能改变。

## d2491bc7d4

>  Currently inside the invulnerability frames after taking damage.

正处在受伤后的无敌帧里。

## d2f55c91d7

>  Has died at least once in this save.

在这个存档里至少死过一次。

## d38f8f9a3e

>  How to point at a player.

怎样指定一名玩家。

## 49dec52213

>  The underlying `kind` value, 0, 1 or 2, aligned with `abi.h`.

底层的 `kind` 值，0、1 或 2，和 `abi.h` 对齐。

## 03084bdd8d

>  Whether this selector is a reliable identity, meaning an xuid or a uuid.
>
>  A permission, economy or ownership decision receiving `false` should take care; see
>  the module documentation.

这个选择器是不是可靠的身份，也就是 xuid 或 uuid。

权限、经济或归属的判断拿到 `false` 时要当心；见模块文档。

## 602a69fd92

>  An emptiness check: an empty selector resolves to nobody, and finding that early beats
>  seeing an inexplicable `false` at the call site.

检查是否为空：空的选择器解析不到任何人，早点发现，好过在调用处看到一个莫名其妙的 `false`。

## 68ab8a37cf

>  One entry `list_players` reports.
>
>  `dimension` and `pos` are `Option` and not bare values. The host omits those keys when
>  the level is not ready, or while that player is midway through a dimension change, and
>  filling them with 0 would say they are at the origin of the overworld. The land
>  protection bypass contract §5.1 records has exactly that shape: an event in a custom
>  dimension could not read `dim`, the consumer wrote `unwrap_or(0)`, and everything was
>  allowed as the overworld.
>
>  A caller that wants a fallback writes `.unwrap_or(0)` itself, and is then the one
>  answerable for that default.

`list_players` 报告的一项。

`dimension` 和 `pos` 是 `Option`，没有用裸值。世界还没就绪，或者这名玩家正在换维度时，宿主会省略这两个键，而填 0 就等于说他在主世界的原点。契约 §5.1 记录的领地保护绕过就是这个形状：自定义维度里的一个事件读不到 `dim`，使用方写了 `unwrap_or(0)`，结果一切都被当成主世界放行了。

想要默认值的调用方自己写 `.unwrap_or(0)`，这个默认值的后果也由它自己负责。

## 461fc9902f

>  Builds a stable selector from an xuid. An empty xuid, on an offline-mode server, falls
>  back to the name and says so, so a caller knows the key is unreliable.

用 xuid 构造一个稳定的选择器。离线模式的服务器上 xuid 为空，这时退回到名字，并且会表明这一点，调用方由此知道这个键靠不住。

## 83f2cca6bb

>  The network status of a player, from `PIER_PSTR_NETWORK_STATUS`.

一名玩家的网络状态，取自 `PIER_PSTR_NETWORK_STATUS`。

## 29379ae316

>  Actors: everything addressed by an `ActorUniqueID`, players included.
>
>  # An id is an identity and not a pointer
>
>  An [`Entity`] holds one `i64`. The host looks the live table up again on every call, so
>  an `Entity` value can be kept across ticks: once the actor dies a call returns `Err`
>  rather than jumping into freed memory. The cost is one lookup per call, so a hot path
>  caches the result itself.
>
>  # A player passes through here to use actor capabilities
>
>  `Player::as_entity()` goes through `player_resolve` for the id. The reverse does not
>  hold: an actor id is not necessarily a player, and there is no slot resolving an id back
>  into a selector.

实体：所有用 `ActorUniqueID` 指定的东西，玩家也包括在内。

**id 是身份，不是指针**

一个 [`Entity`] 只存一个 `i64`。宿主每次调用都重新查一遍活动实体表，所以 `Entity` 值可以跨刻保存：实体死掉以后，调用返回 `Err`，不会跳进已经释放的内存。代价是每次调用一次查找，所以热路径要自己缓存结果。

**玩家要经过这里才能用实体的能力**

`Player::as_entity()` 经 `player_resolve` 取得 id。反过来不成立：一个实体 id 不一定是玩家，也没有槽位能把 id 解析回选择器。

## 28f3dcf33c

>  One actor. A zero-cost wrapper around an `ActorUniqueID`.

一个实体。`ActorUniqueID` 的零开销包装。

## 8290d4e735

>  Enumerates live actors. A `dim` of `None` spans every dimension.
>
>  This slot has no failure bit and reports nothing while the level is not ready, so an
>  empty table means either that the dimension holds no actor or that the level has not
>  come up, and a caller tells them apart with `Host::gaming_status()`.

列出活着的实体。`dim` 为 `None` 时覆盖所有维度。

这个槽位没有失败位，世界没就绪时什么都不报告，所以表为空，既可能是这个维度里没有实体，也可能是世界还没起来；调用方用 `Host::gaming_status()` 区分两者。

## f863b3fbff

>  Whether this id still points at a live actor.
>
>  The criterion is whether the type name can be read: every actor that resolves has one,
>  and one that does not makes `actor_get_str` return false.

这个 id 是否仍然指向一个活着的实体。

判断依据是能不能读到类型名：每个能解析到的实体都有类型名，解析不到的会让 `actor_get_str` 返回 false。

## 969e331364

>  Reads a `PIER_APROP_*` numeric property.

读取一个 `PIER_APROP_*` 数值属性。

## 867d460fa8

>  Reads a `PIER_ASTR_*` string property.

读取一个 `PIER_ASTR_*` 字符串属性。

## ac96f429b5

>  The position, from `Actor::getPosition`. For the feet coordinate of a player see
>  [`Entity::feet_pos`].

位置，取自 `Actor::getPosition`。玩家的脚下坐标见 [`Entity::feet_pos`]。

## 0fb0a93df1

>  The unit vector of the line of sight.

视线方向的单位向量。

## 4404769c8f

>  `(pitch, yaw)`.

`(pitch, yaw)`，即俯仰角和偏航角。

## 6d32ce26e4

>  Runs one `PIER_AACT_*` action and returns its output, which is an empty string for most
>  actions.

执行一个 `PIER_AACT_*` 动作，返回它的输出；大多数动作的输出是空字符串。

## 86181fc8a2

>  Teleports elsewhere within the same dimension.

在同一个维度内传送到别处。

## bf047c778d

>  Teleports into a given dimension. A custom dimension, with an id of 3 or above, goes
>  through here too.

传送到指定的维度。id 为 3 及以上的自定义维度也走这里。

## 59a7d09731

>  Adds one status effect.

添加一个状态效果。

## 17dd0ee35a

>  Reads the current value of one attribute, named as `minecraft:health` is.

读取一个属性的当前值，属性名的写法和 `minecraft:health` 一样。

## a4bbace764

>  The name shown above the head.

显示在头顶上的名字。

## 207db3f81e

>  The line below the name.

名字下方的那一行。

## 03fe650716

>  The name after profanity filtering.

经过脏话过滤以后的名字。

## ccd98129de

>  The current movement speed, in blocks per tick.

当前的移动速度，单位是方块每刻。

## c8704e0dfe

>  How many blocks the current fall has covered, used to compute fall damage on landing.

这一次下落已经落了多少格，着地时用来计算摔落伤害。

## 3122862ce6

>  The size scale, where 1.0 is the original size.

体型缩放，1.0 是原始大小。

## 131f635f4f

>  The `variant` data value, whose meaning differs per actor kind.

`variant` 数据值，含义随实体种类而不同。

## 02c20c8180

>  The `mark_variant` data value, a different numbering from [`Entity::variant`].

`mark_variant` 数据值，编号和 [`Entity::variant`] 是两套。

## e34b057b78

>  How many ticks of the death animation have played.

死亡动画已经播放了多少刻。

## 70c517d88b

>  It is not despawned for being too far from a player.

不会因为离玩家太远而被清除。

## fd994a1d8f

>  In the breeding state.

处在繁殖状态。

## 20ec535f95

>  Holding a totem of undying in a hand or the off hand.

主手或副手拿着不死图腾。

## fc9e030fb6

>  The vehicle being ridden. Riding nothing gives `Ok(None)` and only a missing slot is
>  an `Err`.

正在骑乘的载具。什么都没骑时返回 `Ok(None)`，只有缺少槽位才是 `Err`。

## 59a35ee35b

>  The distance between two actors. The host returns a failure across dimensions.

两个实体之间的距离。跨维度时宿主返回失败。

## e2ee1d15b2

>  The bounding box.

碰撞箱。

## 6438121ec1

>  Clones one to a given position.

在指定的位置复制出一个。

## 37fad71a78

>  Every status effect on it.

身上的全部状态效果。

## c78e18f7fe

>  Reads one bit of `ActorFlags`.
>
>  On the ABI this slot collapses the actor being gone and the bit being false into the
>  same `false`, the shape contract §5.2 opposes. That signature is already released, so
>  this only states it truthfully.
>  Call [`Entity::exists`] first when the two must be told apart.

读取 `ActorFlags` 的一位。

在 ABI 上，这个槽位把「实体不在了」和「这一位为 false」合成了同一个 `false`，这正是契约 §5.2 反对的形状。这个签名已经发布，所以这里只是如实写明。需要区分两者时，先调用 [`Entity::exists`]。

## 14efb9309d

>  Casts a ray along the line of sight of this actor, reporting the hit as an exact
>  coordinate.

沿着这个实体的视线发射一条射线，以精确坐标报告命中点。

## 01b432aeef

>  As above, reporting the hit as a block cell and carrying the face hit.
>
>  Both slots are kept because they answer different questions: placing a block needs a
>  cell coordinate and a face while drawing a particle needs an exact coordinate, and
>  flooring the latter into the former is off by one cell at a block boundary.

同上，但以方块格报告命中点，并带上命中的面。

两个槽位都保留，因为它们回答的问题不同：放方块需要格子坐标和面，画粒子需要精确坐标，而把后者向下取整当作前者，在方块边界上会差一格。

## 5272cad18d

>  One status effect.

一个状态效果。

## 70c44c5bb3

>  The axis-aligned bounding box of an actor.

实体的轴对齐碰撞箱。

## 8506c3ca44

>  One entry `list_actors` reports.

`list_actors` 报告的一项。

## 98ada888df

>  The world: reads and writes at the level layer, covering time, weather, difficulty,
>  game rules, biomes and chunks.
>
>  The boundary with [`crate::host`] is whether something speaks about the host or the
>  world. The server stage, scheduling and executing a command belong to the host and hold
>  for another game; time, weather and chunks belong to the world.
>
>  The switches of the level itself are here, changing things inside the world is in
>  `edit`, and assembling commands is in `commands`.

世界：关卡层面的读写，包括时间、天气、难度、游戏规则、生物群系和区块。

和 [`crate::host`] 的分界在于说的是宿主还是世界。服务器阶段、调度、执行命令属于宿主，换一个游戏也成立；时间、天气、区块属于世界。

关卡本身的开关在这里，改变世界里的东西在 `edit`，拼装命令在 `commands`。

## 558279aaa8

>  Cuts a cuboid along y into slices, each within [`MAX_FILL_VOLUME`].
>
>  Along y rather than along the longest edge: the cost of a `/fill` lies mostly in how
>  many chunks it spans, and the cells of one y column are necessarily in the same chunk.

把一个长方体沿 y 切成若干片，每一片都在 [`MAX_FILL_VOLUME`] 以内。

沿 y 切，不沿最长的边切：`/fill` 的开销主要在于跨了多少个区块，而同一个 y 列上的格子一定在同一个区块里。

## 29e3b186bf

>  Whether the name of a ticking area is valid.
>
>  The engine accepts `A-Z a-z 0-9 _` only. A name containing a space or a non-ASCII
>  character makes `/tickingarea add` parse the name as the next argument, and the error
>  it reports has nothing to do with the name.

常加载区域的名字是否合法。

引擎只接受 `A-Z a-z 0-9 _`。名字里有空格或非 ASCII 字符时，`/tickingarea add` 会把名字解析成下一个参数，报出来的错误和名字毫无关系。

## f4f9991062

>  The level facade. Zero sized.

关卡门面。零大小。

## 8d001ce445

>  Sets the level and the remaining duration, in ticks, of rain and of lightning
>  individually.
>
>  Finer than [`World::set_weather`], which has three settings, this can do light rain for
>  three minutes.

分别设置降雨和闪电的强度，以及各自剩余的持续时间（单位刻）。

比只有三档的 [`World::set_weather`] 更细，可以做到「下三分钟的小雨」。

## 3753e5b719

>  Reads one game rule. An unrecognized rule name is an `Err` and not some default value.

读取一条游戏规则。规则名认不出来时返回 `Err`，不会给某个默认值。

## dacfbe73b6

>  Saves to disk immediately.

立即保存到磁盘。

## 8675caac51

>  Sets the biome of a region, column by column.
>
>  It takes no y. `setBiome3d` works per y while Bedrock stores a biome per column, and
>  taking a y would suggest layers can be set separately. It returns how many columns were
>  set, and 0 means none were, because the chunks are not loaded or the biome name is
>  unrecognized, rather than being set with nothing changing.

按列设置一片区域的生物群系。

不接收 y。`setBiome3d` 按 y 设置，而基岩版按列存储生物群系，接收 y 会让人以为可以分层设置。返回设置了多少列；返回 0 表示一列都没设置，原因是区块没加载或者生物群系名认不出来，不会出现设置了却没有变化的情况。

## 1a1e9e8b3e

>  The villages in one dimension, and `Err` for a host without the villages slot, which
>  an empty list would hide.

一个维度里的村庄；宿主没有 villages 槽位时返回 `Err`，空列表会把这一点掩盖掉。

## 02a206d77a

> use try_villages: this answers an empty list when the host has no villages slot

请用 `try_villages`：宿主没有 villages 槽位时，这个函数回答的是空列表

## 5c3cd24e7f

>  The villages in one dimension, and an empty list when the host cannot list them.

一个维度里的村庄；宿主列不出来时返回空列表。

## f1f230144e

>  The hardcoded generation areas in the loaded chunks within a radius.
>
>  Only loaded chunks are examined, since a read-only query should not force chunks to
>  load. An empty result therefore means either that there are none nearby or that the
>  nearby chunks are not loaded; a host without the slot is an `Err`.

一个半径内已加载区块里的硬编码生成区。

只检查已加载的区块，因为只读查询不应该强制加载区块。所以结果为空，可能是附近没有，也可能是附近的区块没加载；宿主没有这个槽位时返回 `Err`。

## a664a0e1aa

> use try_structures_near: this answers an empty list when the host has no structures slot

请用 `try_structures_near`：宿主没有 structures 槽位时，这个函数回答的是空列表

## cabd3afd53

>  As [`Self::try_structures_near`], with an empty list when the host cannot answer.

和 [`Self::try_structures_near`] 一样，但宿主答不上来时返回空列表。

## 966dc6166a

>  Whether every chunk covered by `[min..max]` is in memory.
>
>  This has to be asked before deleting a save key: a loaded chunk has a copy in memory
>  and writes the key just deleted straight back on unload, while the deletion itself
>  succeeded and reported a positive number.

覆盖 `[min..max]` 的每个区块是否都在内存里。

删除存档键之前必须先问这个：已加载的区块在内存里有一份副本，卸载时会把刚删掉的键直接写回去，而删除本身显示成功，还报告了一个正数。

## 7ed8fdaff0

>  Deletes every save key of a chunk, so the engine regenerates it from the generator on
>  the next load.
>
>  The chunk must be unloaded first; see [`World::chunks_loaded`]. Getting a chunk
>  unloaded is the caller's business, since who is nearby and when unloading is possible
>  needs domain knowledge this layer should not have.
>
>  It returns how many keys were deleted. A 0 is a normal result and means that chunk was
>  never generated.

删除一个区块的所有存档键，下次加载时引擎会用生成器重新生成它。

区块必须先卸载；见 [`World::chunks_loaded`]。让区块卸载是调用方的事，因为谁在附近、什么时候能卸载，需要这一层不该有的领域知识。

返回删除了多少个键。返回 0 是正常结果，表示这个区块从来没有生成过。

## 3db39ece36

>  Lists every save key of a chunk.
>
>  A key is binary and contains zero bytes, so it is a `Vec<u8>` and not a `String`: a
>  UTF-8 conversion would corrupt it into a key that cannot be deleted.

列出一个区块的所有存档键。

键是二进制的，里面有零字节，所以类型是 `Vec<u8>`，没有用 `String`：转成 UTF-8 会把它损坏成一个删不掉的键。

## fc2fd60fc2

>  Deletes one save key byte for byte. The content is not interpreted and what is passed
>  is what is deleted.

逐字节删除一个存档键。内容不会被解读，传进来什么就删除什么。

## 5753bde507

>  Fills a region with `/fill`, cut automatically into slices within the volume cap.
>
>  It returns how many commands ran. A failure partway stops there and returns `Err`
>  rather than continuing, since continuing gives a half-filled region and the return
>  value does not say how far it got.

用 `/fill` 填充一片区域，自动切成不超过体积上限的若干片。

返回运行了多少条命令。中途失败就在那里停下并返回 `Err`，不会继续：继续下去会得到一片填了一半的区域，返回值也说不出填到了哪里。

## 80a6be2721

>  Creates a ticking area.
>
>  A ticking area belongs to the save, survives a restart and belongs to no mod, so it is
>  not removed automatically when a mod unloads and needs
>  [`World::remove_ticking_area`].

创建一个常加载区域。

常加载区域属于存档，重启后仍然存在，也不属于任何模组，所以模组卸载时不会自动移除，需要调用 [`World::remove_ticking_area`]。

## 8b2e4535a1

>  Lists the ticking area names of one dimension.
>
>  The engine output is prose meant for a human, split here on commas and whitespace. The
>  format follows the version, so failing to split returns an empty table rather than an
>  error, and the raw output is in the log.

列出一个维度里的常加载区域名。

引擎的输出是写给人看的文字，这里按逗号和空白切分。格式随版本变化，所以切分失败时返回空表、不报错，原始输出会写进日志。

## 2ccda415a1

>  Scans a region and collects every block and actor into memory.
>
>  A large region uses [`World::scan_with`] instead, since the memory this function uses
>  is proportional to the cell count.

扫描一片区域，把每个方块和实体都收集到内存里。

大区域请改用 [`World::scan_with`]，因为这个函数用的内存和格子数成正比。

## a93f4148d7

>  A streaming scan: the callback runs once per entry the host sinks and nothing is
>  accumulated.
>
>  Both callbacks run synchronously during this call, the pointers the host passes become
>  invalid the moment it returns, and a callback therefore receives an already copied
>  `String` (contract §3).

流式扫描：宿主每输出一项，回调就运行一次，什么都不积累。

两个回调都在这次调用期间同步运行，宿主传来的指针在调用返回的那一刻就失效了，所以回调收到的是已经复制好的 `String`（契约 §3）。

## 4bf4a86e19

>  Scans the blocks of a region into a palette plus cells.
>
>  The host serializes each distinct block state once and reports every cell as an
>  index, so a large region costs a few strings instead of two per cell; this is the
>  form for copying, saving and diffing regions. Entities are not included; use
>  [`World::scan`] with the entity half for those. The same 2^24-cell limit applies.

把一片区域的方块扫描成调色板加格子。

宿主把每种不同的方块状态只序列化一次，每一格只报告一个索引，所以大区域只需要几个字符串，用不着每格两个；复制、保存和比较区域都用这种形式。不包括实体，实体请用 [`World::scan`] 的实体那一半。同样有 2^24 格的上限。

## 7af49dc18e

>  Fills a box with one block. `spec` is a bare name such as `minecraft:stone` or full
>  SNBT, resolved once for the whole box. `update_flags` is as in
>  [`crate::Block::set_nbt`]: `BlockUpdate::NONE` is the fastest and leaves the client to
>  catch up on the next chunk send. Returns the number of cells written.
>
>  One call replaces a loop over `set_block`, which cost an FFI call, a dimension lookup
>  and a spec parse per cell.

用一种方块填满一个长方体。`spec` 可以是 `minecraft:stone` 这样的方块名，也可以是完整的 SNBT，整个长方体只解析一次。`update_flags` 和 [`crate::Block::set_nbt`] 的一样：`BlockUpdate::NONE` 最快，客户端要等下一次发送区块时才更新。返回写入的格子数。

一次调用就代替了对 `set_block` 的循环，后者每一格都要一次 FFI 调用、一次维度查找和一次描述解析。

## c2672f9137

>  Writes many cells in one call. Each entry of `palette` is a block spec resolved once,
>  and each cell names one by index. Returns the number of cells written; a cell whose
>  index is out of range or whose spec did not resolve is skipped, so a result below
>  `cells.len()` means some were.
>
>  Pasting an [`IndexedScan`] is `set_blocks(dim, &scan.palette_specs(), &scan.cells, BlockUpdate::NONE)`.

一次调用写入很多格。`palette` 的每一项是一个只解析一次的方块描述，每一格用索引指定其中一项。返回写入的格子数；索引越界或者描述解析不出来的格子会被跳过，所以结果小于 `cells.len()` 就表示有格子被跳过了。

粘贴一个 [`IndexedScan`] 就是 `set_blocks(dim, &scan.palette_specs(), &scan.cells, BlockUpdate::NONE)`。

## 75f0085c39

>  Spawns an actor from full NBT, the inverse of [`Entity::snapshot`].
>
>  A given `pos` overrides the `Pos` inside the NBT. The engine allocates a new UniqueID,
>  so the id from the save is not reused.

用完整的 NBT 生成一个实体，是 [`Entity::snapshot`] 的反向操作。

给了 `pos` 时会覆盖 NBT 里的 `Pos`。引擎会分配新的 UniqueID，不会沿用存档里的 id。

## 9e3af47869

>  Detonates. A `source` of `None` means there is no source actor.

引爆。`source` 为 `None` 表示没有来源实体。

## c84a37a808

>  A particle visible across the whole dimension. Showing it to one person only uses
>  [`crate::player::Player::spawn_particle`].

整个维度都看得到的粒子。只显示给一个人看，请用 [`crate::player::Player::spawn_particle`]。

## 966f7f5f58

>  Computes a path for an actor to a target cell.

为一个实体计算到目标格子的路径。

## 28a5b533fe

>  The result of one [`World::scan_indexed`]: every distinct block state once, and one
>  small cell per position. A region of a million stone cells holds one `String` pair here
>  where [`Scan`] holds two million.

一次 [`World::scan_indexed`] 的结果：每种不同的方块状态一份，每个位置一个小格子。一百万格石头的区域，在这里只有一对 `String`，在 [`Scan`] 里则有两百万个。

## b524530613

>  The palette entry of a cell.

一个格子对应的调色板条目。

## ea21c911ae

>  The palette as block specs, the shape [`World::set_blocks`] takes.

把调色板转成方块描述，也就是 [`World::set_blocks`] 接收的形状。

## 2bc85daad2

>  Cells that are not air, by palette name.

按调色板里的名字，统计不是空气的格子数。

## a2470e4c4b

>  The result of one scan.

一次扫描的结果。

## 0ac51783f1

>  Indexes a block by coordinate.
>
>  It rebuilds a table on every call, so it does not belong in a loop, where it is O(n^2).
>  Repeated lookups keep the returned value. The ABI does not guarantee the traversal
>  order of the sink, so a position cannot be computed from an index.

按坐标索引方块。

每次调用都会重建一张表，所以不该放在循环里，那样是 O(n^2)。要反复查询，就保存返回的值。ABI 不保证输出回调的遍历顺序，所以不能从下标推算位置。

## 467728f862

>  How many actors fell inside this region.

有多少实体落在这片区域里。

## 186a40c3a6

>  One actor that fell inside the region during a scan.

扫描时落在区域里的一个实体。

## 7b09e97f8b

>  One distinct block state met by [`World::scan_indexed`].

[`World::scan_indexed`] 遇到的一种不同的方块状态。

## 12cf0cfd3d

>  One cell of [`World::scan_indexed`] or [`World::set_blocks`]: a position and an index
>  into the palette that travels with it.

[`World::scan_indexed`] 或 [`World::set_blocks`] 的一格：一个位置，加上随它一起传递的调色板里的索引。

## e1e2821c4b

>  One village.

一个村庄。

## 1294856559

>  One hardcoded generation area: a stronghold, a witch hut, an ocean monument or a
>  pillager outpost.

一个硬编码生成区：要塞、女巫小屋、海底神殿或掠夺者前哨站。

## 9262e49ce7

>  The value of one game rule.

一条游戏规则的值。

## ec0cd9eb9f

>  The sleep status, from `level_get_sleep_status`.

睡眠状态，取自 `level_get_sleep_status`。

## 2f6bcb50d2

>  A cuboid in whole cells, as `(min, max)`.

以整格表示的长方体，写作 `(min, max)`。

## c2379ed863

>  The volume cap of one `/fill` command.
>
>  The engine's own cap is 32768 cells and exceeding it fails the whole command: not a few
>  cells short, but not one cell filled.

一条 `/fill` 命令的体积上限。

引擎自己的上限是 32768 格，超过时整条命令都会失败，一格都不会填。

## ba9c108726

>  Blocks: a cell addressed by a dimension plus a coordinate.
>
>  # There are two write paths, and the native one is the default
>
>  [`Block::set`] goes through `set_block`, meaning `BlockSource::setBlock`, and takes a name or
>  full SNBT. [`Block::set_states`] and [`Block::set_nbt`] go through `edit_*` and add a
>  [`BlockUpdate`] letting the caller decide whether to notify neighbors and synchronize the
>  client. Turning both off during a bulk fill is an order of magnitude faster, at the cost of
>  resynchronizing afterwards.
>
>  # A waterlogged block needs the liquid layer
>
>  Waterlogging in Bedrock is not a block state but a second block in the same cell: the main layer
>  is the stair and the liquid layer is the water. [`Block::name`] sees only the main layer, so
>  copying and pasting a waterlogged stair loses the water entirely: the main layer is exact and
>  the water is gone. Moving the water with it means reading and writing [`Block::extra`].

方块：用维度加坐标指定的一格。

**有两条写入路径，默认用原生的那条**

[`Block::set`] 走 `set_block`，也就是 `BlockSource::setBlock`，接收方块名或完整的 SNBT。[`Block::set_states`] 和 [`Block::set_nbt`] 走 `edit_*`，多一个 [`BlockUpdate`]，让调用方决定要不要通知相邻方块、要不要同步给客户端。批量填充时两者都关掉，速度能快一个数量级，代价是之后要重新同步。

**含水方块要读写液体层**

基岩版的含水用的是同一格里的第二个方块：主层是楼梯，液体层是水。[`Block::name`] 只看得到主层，所以复制、粘贴含水楼梯会把水全部丢掉：主层完全正确，水没了。要把水一起搬过去，就要读写 [`Block::extra`]。

## 1064ab1e91

>  One cell in the world.

世界里的一格。

## a78e26ebe0

>  The type name and the full SNBT, both from one call.

类型名和完整的 SNBT，一次调用拿到两者。

## b50025f85b

>  Parses the full serialization into an NBT tree. Writing it back unchanged uses
>  [`Block::set_nbt`], a path that does not go through the parser of this layer.

把完整的序列化解析成 NBT 树。要原样写回，用 [`Block::set_nbt`]，那条路径不经过这一层的解析器。

## 640c7c52fe

>  Reads a `PIER_BPROP_*` numeric property.

读取一个 `PIER_BPROP_*` 数值属性。

## 187c82a8bf

>  Reads a `PIER_BSTR_*` string property.

读取一个 `PIER_BSTR_*` 字符串属性。

## f396c3ae16

>  The block tags.

方块的标签。

## d807a858ec

>  Runs a `PIER_BACT_*` action.

执行一个 `PIER_BACT_*` 动作。

## 2a42d22e83

>  Treats this cell as an item, through `Block::asItemInstance`.

把这一格当作物品，经 `Block::asItemInstance` 转换。

## d73d238a38

>  Drops one item at this cell.

在这一格掉落一个物品。

## 150731534b

>  Places a block. `spec` is a name such as `"minecraft:stone"`, or full SNBT.
>
>  An unrecognized name fails and no placeholder block is put down, whose symptom would
>  be a patch of purple-and-black in the world with no visible origin.

放置一个方块。`spec` 是 `"minecraft:stone"` 这样的名字，或者完整的 SNBT。

名字认不出来时调用失败，不会放一个占位方块；放了占位方块的症状，是世界里出现一块来历不明的紫黑格子。

## 5fbb1976a8

>  Places a block from full NBT, deciding the update flags yourself.

用完整的 NBT 放置一个方块，更新标志由你自己决定。

## 1e35466831

>  Places a block by name plus a subset of its states.
>
>  A `states` of `None` means every state at its default. The host takes the version
>  number from the default states and a caller must not fill it in: a wrong version number
>  lands the block under a different set of state meanings.

用方块名加上部分状态放置一个方块。

`states` 为 `None` 表示所有状态都用默认值。宿主从默认状态里取版本号，调用方不能自己填：版本号错了，方块会按另一套状态含义落地。

## 09665a7c9c

>  Reads the liquid layer. An empty one reads back as `"minecraft:air"` and is not an
>  error.

读取液体层。空的液体层读出来是 `"minecraft:air"`，不算错误。

## ee6dfcd966

>  Writes the liquid layer. Writing `"minecraft:air"` clears it.

写入液体层。写入 `"minecraft:air"` 就是清空。

## 3732d381f1

>  The container at this cell, a chest or a hopper. It does not check whether one is
>  really there, since checking would cross the ABI once, and the returned
>  [`crate::container::Container`] reports it naturally the first time it is used.

这一格的容器，比如箱子或漏斗。它不检查那里是不是真的有容器，因为检查要跨一次 ABI；返回的 [`crate::container::Container`] 第一次使用时自然会报告。

## 6e22a38471

>  The localization key, not the text that is displayed.

本地化用的键，并非显示出来的文字。

## 9c13c15d12

>  The localized display name.

本地化以后的显示名。

## 3a71e53806

>  The engine's own debug string. Its format follows the version, so no decision rests on
>  it.

引擎自己的调试字符串。格式随版本变化，所以不要拿它做任何判断。

## b00ffc9977

>  It can produce a redstone signal itself, as a lever or a button does.

自己能产生红石信号，像拉杆或按钮那样。

## 92195e07ed

>  It drops nothing unless mined with the right tool.

不用正确的工具挖就什么都不掉。

## 742ef4cd9d

>  The legacy data value. A newer block uses [`Block::states`].

旧版的数据值。较新的方块请用 [`Block::states`]。

## 0572deaa4f

>  The numeric id of the matching item.

对应物品的数字 id。

## bde129212b

>  The actual brightness of this cell, skylight included.

这一格的实际亮度，包括天空光。

## f4937f694e

>  How much light this block emits itself.

这个方块自己发出多少光。

## b73f9366f1

>  The mining hardness; larger is slower.

挖掘硬度；越大越慢。

## be392ad2b0

>  The blast resistance.

爆炸抗性。

## ca77300e72

>  The friction coefficient; ice is one of the low ones.

摩擦系数；冰是摩擦很低的一种。

## 79ea479e9e

>  The bounciness; a slime block is non-zero.

弹性；黏液块的弹性不为零。

## 2528453402

>  The probability weight of catching fire.

着火的概率权重。

## 1f0cf9396c

>  The probability weight of spreading fire to a neighbor.

把火蔓延给相邻方块的概率权重。

## 0b1a0e46b6

>  The redstone signal strength this cell outputs, from 0 to 15.

这一格输出的红石信号强度，0 到 15。

## 47cc44b656

>  The strength a comparator reads from this cell; a container computes it from how full
>  it is.

比较器从这一格读到的强度；容器按装满的程度计算它。

## dd55305b66

>  Reads the value of one block state.

读取一个方块状态的值。

## 5b42381794

>  Every block state.

所有方块状态。

## 96829e197e

>  The collision box.

碰撞箱。

## e12d561c60

>  The NBT of the block entity. A cell with no block entity gives `Ok(None)`.

方块实体的 NBT。这一格没有方块实体时返回 `Ok(None)`。

## 5b1b56397c

>  Writes the NBT of the block entity back, through `BlockActor::load`.
>  The cell already has to hold the matching kind of block.

把方块实体的 NBT 写回去，经 `BlockActor::load`。这一格里必须已经是对应种类的方块。

## ba778faaa2

>  What one block cell reads back as.

一个方块格读回来的内容。

## dd300249b7

>  Items: a value object and not a handle.
>
>  On the ABI an item is a string of SNBT throughout. Reading a property means asking a
>  property with that SNBT, and changing one means exchanging that SNBT for a new one
>  through `item_transform`. This layer copies that shape and hides no pointer in between.
>
>  # The consequence: an `ItemStack` has no connection to the item in the world
>
>  `container.item(0)` returns a snapshot. Changing it does not move the one in the
>  container, and writing it back takes an explicit `container.set_item(0, &stack)`. This
>  is the other side of a handle being an identity and not a pointer: with no implicit
>  synchronization in between, there is no I changed it and nothing happened.

物品：一个值对象，不是句柄。

在 ABI 上，物品自始至终是一段 SNBT 字符串。读一个属性，就是拿这段 SNBT 去问；改一个属性，就是通过 `item_transform` 把这段 SNBT 换成一段新的。这一层照搬这个形状，中间不藏任何指针。

**结果：`ItemStack` 和世界里的物品没有联系**

`container.item(0)` 返回的是一份快照。修改它不会影响容器里的那个，要写回去需要明确调用 `container.set_item(0, &stack)`。这是「句柄是身份，不是指针」的另一面：中间没有隐式的同步，也就不会出现「我改了却什么都没变」。

## 079b6106dd

>  An SNBT snapshot of one item.
>
>  The fields the snapshot itself carries, `Name`, `Count`, `Damage` and the `tag`
>  compound, are read locally from a parse cached on first use; only properties that need
>  the item registry, such as the stack limit or the attack damage, cross the ABI, where the
>  host parses the SNBT and builds an engine ItemStack for every call.

一个物品的 SNBT 快照。

快照本身带有的字段，即 `Name`、`Count`、`Damage` 和 `tag` 复合标签，从第一次使用时缓存的解析结果里在本地读取；只有需要物品注册表的属性，比如堆叠上限或攻击伤害，才会跨过 ABI，这时宿主每次调用都要解析 SNBT，并构造一个引擎的 ItemStack。

## 1f18dcf618

>  Uses a string of SNBT directly, without validation, since validating would cross the
>  ABI once and this constructor sits on a hot path.
>  A wrong shape reports an error at the first call that really uses it.

直接使用一段 SNBT 字符串，不做校验：校验要跨一次 ABI，而这个构造函数在热路径上。形状不对时，会在第一次真正使用它的调用处报错。

## 4364d91ee4

>  Builds one from a type name and a count.
>
>  It assembles the minimal shape `{Name:"...",Count:Nb}` and the engine fills in the rest
>  inside `ItemStack::fromTag`. A name that does not exist fails at the moment the item is
>  used and not here.

用类型名和数量构造一个物品。

它拼出最小的形状 `{Name:"...",Count:Nb}`，其余部分由引擎在 `ItemStack::fromTag` 里补全。名字不存在时，会在物品被使用的时候失败，不在这里失败。

## 9d4d04fa20

>  Air. An empty slot in a container reads back as this.

空气。容器里的空格子读出来就是它。

## 93377dc22f

>  The underlying SNBT.

底层的 SNBT。

## 4ab5cfc074

>  Parses into an NBT tree, for reading a field the ABI gives no named accessor for.

解析成 NBT 树，用来读取 ABI 没有提供命名访问方式的字段。

## d18d8c3190

>  Reads a `PIER_IPROP_*` numeric property.
>
>  A property number the host does not recognize returns `Err` and not 0:
>  cannot-be-determined and an answer of 0 must stay apart (contract §5.2).

读取一个 `PIER_IPROP_*` 数值属性。

宿主不认识的属性编号返回 `Err`，不返回 0：「无法确定」和「答案是 0」必须分开（契约 §5.2）。

## 72c2537fea

>  Reads a `PIER_ISTR_*` string property.

读取一个 `PIER_ISTR_*` 字符串属性。

## 4678bcae0a

>  The custom lore. The host gives an SNBT string list, parsed here into a `Vec<String>`.

自定义的描述文字。宿主给的是 SNBT 字符串列表，这里解析成 `Vec<String>`。

## ccc9ed6723

>  The custom NBT of the item, the `tag` section. It goes through a dedicated slot rather
>  than `PIER_ISTR_USER_DATA`: the content is the same and the dedicated slot saves one
>  property-number dispatch on the host side.

物品的自定义 NBT，也就是 `tag` 段。它走专用的槽位，没有用 `PIER_ISTR_USER_DATA`：内容一样，专用槽位让宿主少做一次属性编号的分派。

## 28424076d1

>  The color as `{r,g,b}`. Only a dyeable item has one.

颜色，形如 `{r,g,b}`。只有能染色的物品才有。

## 18ad3a9084

>  Runs one `PIER_IOP_*` transform and replaces itself with the result.
>
>  On failure it stays unchanged: with no new SNBT from the host there is nothing to write
>  back, and writing half a result in is harder to diagnose than doing nothing.

执行一个 `PIER_IOP_*` 变换，用结果替换自己。

失败时保持不变：宿主没有给出新的 SNBT，也就没有东西可以写回；写进去半个结果，比什么都不做更难排查。

## 150b78198c

>  As above, returning a new item and leaving this one unchanged.

同上，但返回一个新物品，这个物品保持不变。

## 6c73330c6d

>  Adds an enchantment. A level of 0 removes it, as far as the engine is concerned.

添加一个附魔。等级为 0 时，在引擎看来就是移除它。

## d14f379818

>  Replaces the whole enchantment set and returns a new item.

替换整组附魔，返回一个新物品。

## 5d58c56afa

>  Whether two items are the same kind of thing.
>
>  The criterion comes from the engine, `ItemStack::matches`, and is not string equality:
>  fields such as count and durability take no part, while comparing SNBT text would
>  include them.
>  A missing slot returns `Err` and does not fall back to a text comparison, which would
>  make the criterion differ between hosts.

两个物品是不是同一种东西。

判断标准来自引擎的 `ItemStack::matches`，和字符串相等不同：数量、耐久这些字段不参与比较，而比较 SNBT 文本会把它们算进去。缺少槽位时返回 `Err`，不会退回到文本比较，否则判断标准会因宿主而异。

## 7e172c53ea

>  One enchantment.

一个附魔。

## 190538423e

>  Containers: the four on a player, plus the one at a coordinate in the world.
>
>  On the ABI a container is an owner plus which one, a `PierContainerRef`, and not a
>  pointer. A `Container` value can therefore be kept indefinitely: it resolves again on
>  every call and still points at the right thing after a player leaves and rejoins.
>
>  # Call [`Container::refresh`] after writing
>
>  `set_item`, `add_item` and `clear` go through `Container::setItem`, which changes only
>  the server copy and sends no packet. The client keeps rendering what it last received
>  until the player clicks a slot and it resynchronizes passively. One `refresh` after a
>  bulk change pushes the whole container across. It must not be called per slot in a
>  loop, since it pushes the whole container and doing so per slot is a packet storm.

容器：玩家身上的四个，加上世界里某个坐标处的那一个。

在 ABI 上，容器是「所有者 + 哪一个」，即 `PierContainerRef`，不是指针。所以 `Container` 值可以一直保存：它每次调用都重新解析，玩家离开又回来以后，仍然指向正确的东西。

**写完以后调用 [`Container::refresh`]**

`set_item`、`add_item` 和 `clear` 都经 `Container::setItem` 写入，它只改服务器上的副本，不发送任何数据包。客户端继续显示它最后收到的内容，直到玩家点一下某个格子，被动地重新同步。批量修改之后调用一次 `refresh`，就会把整个容器推过去。不能在循环里每改一格调用一次：它推送的是整个容器，每格一次会发出大量数据包。

## c337949d02

>  A reference to one container.

对一个容器的引用。

## 34d86f31a9

>  One container on a player. Rarely called directly; the `Player::inventory()` family is
>  the usual route.

玩家身上的某个容器。很少直接调用，通常走 `Player::inventory()` 这一族方法。

## 2595cf1187

>  A block container at a coordinate in the world.

世界里某个坐标处的方块容器。

## 23173159b5

>  The coordinate of a block container. A player container gives `None`, since it is not
>  on a cell.

方块容器的坐标。玩家的容器返回 `None`，因为它不在某个格子上。

## e3eafd8342

>  The slot count.
>
>  Failing to resolve, because the player left or that cell holds no container, returns
>  `Err` and not 0: a 0 would make a caller's `for i in 0..size` run zero times in
>  silence (contract §5.2).

格子数量。

解析失败（玩家离开了，或者那一格没有容器）时返回 `Err`，不返回 0：返回 0 会让调用方的 `for i in 0..size` 悄无声息地一次都不执行（契约 §5.2）。

## 228caff180

>  What is in one slot. An empty slot gives the SNBT of the air item and is not an error.

某一格里的东西。空格子给出空气物品的 SNBT，不算错误。

## bd5ec42bd3

>  Every slot.
>
>  One slot failing to read fails the whole thing rather than being skipped: a listing
>  missing a few items gives the wrong answer to what is in this chest.

每一格。

有一格读不出来就整体失败，不会跳过：少了几个物品的清单，对「这个箱子里有什么」给出的是错误的答案。

## 9a51ca8b62

>  Writes one slot. Remember [`Container::refresh`] afterwards.

写入某一格。之后记得调用 [`Container::refresh`]。

## a9326b68ed

>  Puts one in and lets the engine pick the slot.
>
>  A full container is an `Err` and not a silent discard, whose symptom is a player's
>  items vanishing.

放进去一个，由引擎挑选格子。

容器满了时返回 `Err`，不会悄悄丢弃；悄悄丢弃的症状是玩家的物品凭空消失。

## 5a3054837d

>  Takes `count` items out of one slot.

从某一格取出 `count` 个物品。

## bd4da148ef

>  Resends the whole container to its owner.
>
>  A block container returns `Err`: a chest has no single owner to send to and its
>  viewers are refreshed by the engine's own container transaction path. That is a host
>  rule and not a choice of this layer.

把整个容器重新发给它的主人。

方块容器返回 `Err`：箱子没有唯一的主人可以发送，正在看它的玩家由引擎自己的容器事务流程刷新。这是宿主的规则，这一层只是照着做。

## b28f917280

>  The kind of a container. The values align with `PierContainerRef::which`.

容器的种类。取值和 `PierContainerRef::which` 对齐。

## c976a46574

>  A block container has no single owner and therefore cannot be resynchronized; see
>  [`Container::refresh`].

方块容器没有唯一的主人，因此不能重新同步；见 [`Container::refresh`]。

## 3eeab165ab

>  Events: subscribing, reading a payload, editing it, cancelling.
>
>  # A missing key and a value of 0 must stay apart
>
>  Contract §5.1 records a land-protection bypass: an event in a custom dimension could
>  not read `dim`, the consumer wrote `unwrap_or(0)` and treated it as the overworld, and
>  it was allowed with nothing logged. [`Event`] therefore offers typed access, where a
>  missing key and a type mismatch are two different errors.
>
>  It also recognizes `_unresolved`, a marker the host injects when it cannot resolve the
>  source of an event. Methods such as [`Event::dim`] return `Err` on it, so a protection
>  decision fails closed instead of continuing with an invented 0.
>
>  [`Wiring`] does chained batch subscription and holds the handles together.

事件：订阅、读取载荷、修改载荷、取消。

**缺少的键和值为 0 必须分开**

契约 §5.1 记录了一次领地保护绕过：自定义维度里的一个事件读不到 `dim`，使用方写了 `unwrap_or(0)`，把它当成主世界，结果放行了，日志里什么都没有。所以 [`Event`] 提供带类型的访问方式，缺少键和类型不符是两种不同的错误。

它还识别 `_unresolved`，这是宿主解析不出事件来源时注入的标记。[`Event::dim`] 这类方法遇到它会返回 `Err`，保护判断因此会以拒绝收场，不会拿一个编出来的 0 继续往下走。

[`Wiring`] 用来链式批量订阅，并把句柄放在一起保管。

## 3ea8b89185

>  Subscribes to an event.
>
>  ```ignore
>  let l = event::subscribe(names::PLAYER_CHAT, |ev| {
>      let Ok(msg) = ev.str_at("message") else { return };
>      if !msg.contains("badword") { return; }
>      // cancel() returns a Result: an event that cannot be blocked says why and where.
>      if let Err(e) = ev.cancel() {
>          Logger::get().error(&format!("the chat filter did not take effect: {e}"));
>      }
>  })?;
>  ```

订阅一个事件。

```rust
let l = event::subscribe(names::PLAYER_CHAT, |ev| {
    let Ok(msg) = ev.str_at("message") else { return };
    if !msg.contains("badword") { return; }
    // cancel() 返回 Result：拦不住的事件会说明原因，以及该去哪里拦。
    if let Err(e) = ev.cancel() {
        Logger::get().error(&format!("the chat filter did not take effect: {e}"));
    }
})?;
```

## ad1f947782

>  Subscribes with a priority.

按指定的优先级订阅。

## 7c38c1330e

>  Every event id the host knows, from the registry plus every synthetic event.
>
>  Reading it beats guessing when an event name is misspelled.

宿主知道的所有事件 id，来自注册表，加上所有合成事件。

事件名拼错时，读一读它比去猜强。

## 9127a4ef35

>  Whether this host knows a given event id.

这个宿主是否知道某个事件 id。

## 3b336aad8c

>  One event dispatch. This is what a callback receives.
>
>  It lives only for the duration of the callback and must not be stored, which the
>  lifetime parameter also prevents.

一次事件分发，也就是回调收到的东西。

它只在回调期间存在，不能保存下来，生命周期参数也会阻止这样做。

## 04f6544bc0

>  The event id, such as `"ll::event::player::PlayerChatEvent"` or the synthetic
>  `"BlockDestroyEvent"`.

事件 id，例如 `"ll::event::player::PlayerChatEvent"`，或者合成事件 `"BlockDestroyEvent"`。

## 9fc8ab2e54

>  The raw payload SNBT. The most direct thing while debugging.

原始的载荷 SNBT。调试时最直接。

## 7ba1de333f

>  The parsed payload. Parsed on the first call and reused afterwards.

解析后的载荷。第一次调用时解析，之后复用。

## ecf718987d

>  Returns `None` rather than an error when the payload cannot be parsed, for an
>  observing listener that treats an unparsable payload as no event at all.

载荷解析不了时返回 `None`，不报错。给把解析不了的载荷当作没有事件的观察型监听器用。

## 93a6a0569f

>  Reads a string by path. A missing key and a type mismatch both name the key.

按路径读取一个字符串。缺少键和类型不符，都会在错误里写出键名。

## 149f249275

>  The lenient form: `None` when it cannot be read, with no explanation. Only for cases
>  where not reading it does not matter.

宽松的写法：读不出来时返回 `None`，不说明原因。只用在读不到也无所谓的地方。

## bf2a87a888

>  The list of fields the host could not resolve, `_unresolved`.
>
>  When an event carries an Actor stub that is neither an online player nor present in
>  the runtime actor table, the host records the field name here. A non-empty list means
>  the payload is incomplete, and a protection decision should refuse rather than guess.
>  The list is empty too when the payload could not be parsed at all, which
>  [`Self::check_complete`] does not count as complete.

宿主没能解析的字段列表，即 `_unresolved`。

事件里带着的实体既不是在线玩家、也不在运行时的实体表里时，宿主会把字段名记在这里。列表不为空表示载荷不完整，保护判断应当拒绝，不要去猜。载荷根本解析不了时，列表也是空的，[`Self::check_complete`] 不会把这种情况算作完整。

## e587b4f142

>  Checks whether the payload is complete: it parsed, and `_unresolved` is empty. A
>  payload that did not parse is not complete, since nothing in it could be resolved.

检查载荷是否完整：解析成功，并且 `_unresolved` 为空。解析不了的载荷不算完整，因为里面什么都没能解析出来。

## 80c1670311

>  Which dimension the event happened in.
>
>  An unreadable value is an `Err`. An
>  earlier design had callers write `payload.i32_at("dim").unwrap_or(0)`, so every event
>  in a custom dimension, whose id is 3 or above, was judged to be in the overworld, and
>  land protection refusing in the overworld and allowing elsewhere was bypassed with
>  nothing logged.

事件发生在哪个维度。

读不出来时返回 `Err`。以前的设计让调用方写 `payload.i32_at("dim").unwrap_or(0)`，于是自定义维度（id 为 3 及以上）里的每个事件都被判定在主世界，在主世界拒绝、在别处放行的领地保护就这样被绕过了，日志里什么都没有。

## b86e025df7

>  The player identity inside an event.
>
>  Handles three shapes: the `_player:{name,xuid,uuid}` of a synthetic event, the
>  `_player` the host enriched, and an older event carrying only a name. Callers used to
>  have to know the differences themselves.

事件里的玩家身份。

处理三种形状：合成事件的 `_player:{name,xuid,uuid}`，宿主补充的 `_player`，以及只带名字的旧事件。以前调用方得自己知道这些区别。

## 95ac95dba1

>  The block or position coordinates in an event, as the three flat fields `x`, `y` and
>  `z`, which is the shape of a synthetic event.

事件里的方块或位置坐标，即平铺的三个字段 `x`、`y`、`z`，这是合成事件的形状。

## ddb24dcd9e

>  As above but as floating point, for a player position and the like.

同上，但用浮点数，用于玩家位置之类。

## 3140d23b35

>  Whether this event can be cancelled.
>
>  * `Some(true)`: it can;
>  * `Some(false)`: it cannot, and [`Event::cancel`] returns `Err`;
>  * `None`: it is not in the tables, being an event a third-party mod emits itself or a
>    new upstream event the tables have not caught up with. `cancel()` then writes back as
>    usual and nobody can confirm for you that it took effect.

这个事件能不能取消。

- `Some(true)`：能；
- `Some(false)`：不能，[`Event::cancel`] 会返回 `Err`；
- `None`：不在表里，可能是第三方模组自己发出的事件，也可能是表还没跟上的上游新事件。这时 `cancel()` 照常写回，但没有人能替你确认它是否生效。

## 2b6cfaef2a

>  Cancels this event.
>
>  An event that cannot be cancelled returns `Err` and says which event to block instead,
>  such as `PlayerStartDestroyBlockEvent` pointing at `PlayerDestroyBlockEvent`. Returning
>  `()` would let a protection mod believe it had blocked something, and that belief is
>  more dangerous than a crash, since a crash is at least visible.
>
>  An event that is not in the tables, where `can_cancel()` is `None`, does not stand in
>  the way: it writes back as usual and returns `Ok`. The SDK does not pretend to know
>  what it does not know.
>
>  An `Ok` means only that the cancel bit was written back to the host and not that the
>  engine stopped: some hook points sit half updated and the host does not accept a cancel
>  there at all. That boundary rests on the event documentation alone.

取消这个事件。

不能取消的事件返回 `Err`，并说明应该去拦哪个事件，比如 `PlayerStartDestroyBlockEvent` 会指向 `PlayerDestroyBlockEvent`。如果返回 `()`，保护模组会以为自己拦住了，这种误会比崩溃更危险，因为崩溃至少看得见。

不在表里的事件（`can_cancel()` 为 `None`）不受阻拦：照常写回，返回 `Ok`。SDK 不假装知道自己不知道的事。

`Ok` 只表示取消位已经写回给宿主，并不表示引擎停下了：有些钩子点处于更新到一半的状态，宿主在那里根本不接受取消。这个边界只写在事件文档里。

## 1082c48b9b

>  Cancels without caring whether cancelling is possible.
>
>  There is one legitimate use: a generic forwarding or proxy component where the event id
>  arrives at runtime and failing to block simply has to be accepted. Business code uses
>  [`Event::cancel`] and handles the `Err`.

不管能不能取消，都去取消。

只有一种正当的用法：通用的转发或代理组件，事件 id 在运行时才知道，拦不住也只能接受。业务代码请用 [`Event::cancel`]，并处理 `Err`。

## 062ca423ec

>  Undoes an earlier cancel by writing `cancelled` back to 0.
>
>  For a two-stage decision that blocks first, decides, and then finds it may allow. Note
>  that it undoes only a cancel written by this callback itself: a cancel another mod made
>  at an earlier priority cannot be undone, and the host bus does not allow a veto to be
>  turned back into an approval either.

把 `cancelled` 写回 0，撤销之前的取消。

用于「先拦下、再判断、结果发现可以放行」的两段式决策。注意它只能撤销这个回调自己写的取消：别的模组在更早的优先级上做的取消撤销不了，宿主的事件总线也不允许把否决变回同意。

## 92734226bc

>  Edits one field of the payload.
>
>  The write-back is a difference: only the keys really touched go back to the host and
>  untouched ones stay as they are, so two mods on the same event do not erase each
>  other's edits.

修改载荷里的一个字段。

写回的是差异：只有真正碰过的键会交还给宿主，没碰过的保持原样，所以同一个事件上的两个模组不会抹掉彼此的修改。

## 34e2c2d558

>  An arbitrary rewrite. The closure receives a mutable copy of the payload.
>
>  This is the one path that copies the whole payload; [`Event::set`] and
>  [`Event::cancel`] write only the keys they touch.

任意改写。闭包收到载荷的一份可变副本。

这是唯一会复制整个载荷的路径；[`Event::set`] 和 [`Event::cancel`] 只写它们碰过的键。

## e1c38eaf04

>  Batch subscription, holding the handles together.
>
>  Two things over subscribing by hand: a failed subscription is not silent, since
>  failures are recorded in [`Wiring::failures`] and `arm()` can fail as a whole, and the
>  tag goes into the log so the failing entry can be located.
>
>  ```ignore
>  let wiring = Wiring::new("plots")
>      .on(names::PLAYER_DESTROY_BLOCK, "protect-break", |ev| { ... })
>      .at(names::PLAYER_DISCONNECT, Priority::Low, "forget", |ev| { ... })
>      .arm()?;                       // any failure fails the whole thing
>
>  // wiring going out of scope unsubscribes everything
>  ```

批量订阅，并把句柄放在一起保管。

比手动订阅多两点：订阅失败不会悄无声息，失败会记在 [`Wiring::failures`] 里，`arm()` 也可以整体失败；标签会写进日志，方便找到失败的那一项。

```rust
let wiring = Wiring::new("plots")
    .on(names::PLAYER_DESTROY_BLOCK, "protect-break", |ev| { ... })
    .at(names::PLAYER_DISCONNECT, Priority::Low, "forget", |ev| { ... })
    .arm()?;                       // 任何一项失败，整体都失败

// wiring 离开作用域时，全部订阅都会取消
```

## 5c18cde666

>  Adds a subscription at Normal priority. `tag` is used only for logging.

以 Normal 优先级添加一个订阅。`tag` 只用于日志。

## 95abe202b5

>  Adds a subscription at a given priority.

以指定的优先级添加一个订阅。

## cd299aa087

>  Performs the subscriptions. Any failure fails the whole thing and the ones that
>  succeeded are unsubscribed before returning, because half-attached protection is more
>  dangerous than none: some points block and some do not, and nobody knows which.

执行订阅。任何一项失败都让整体失败，已经成功的会在返回前取消订阅，因为挂了一半的保护比没有保护更危险：有的点拦得住，有的拦不住，没有人知道是哪些。

## e9461c563f

>  The lenient form: failures are recorded and successes attach as usual. Suited to an
>  optional feature that is nice to have.

宽松的写法：失败的记录下来，成功的照常挂上。适合有了更好、没有也行的可选功能。

## 74a470e661

>  The ones that did not attach, as (tag, reason).

没有挂上的那些，形如 (标签, 原因)。

## 6be8d65afa

>  How many attached.

挂上了多少个。

## 154f7e1e78

>  Switches everything to no automatic unsubscription, living until the mod unloads.

全部改为不自动取消订阅，一直存活到模组卸载。

## 33957d03ba

>  The player identity inside an event.
>
>  The three fields each have their own use and must not be mixed:
>  * `xuid` is unique and cannot be changed, and is the only key for permissions or
>    economy. It may be empty in offline mode.
>  * `uuid` is equally stable and suits a save key.
>  * `name` is for display. A player can change their display name, and host name
>    resolution falls back to the display name when the account name misses (see
>    `bridge::resolvePlayer`), so it must not be used as an identity.

事件里的玩家身份。

三个字段各有各的用处，不能混用：

- `xuid` 唯一且不可更改，是权限或经济判断唯一能用的键。离线模式下可能为空。
- `uuid` 同样稳定，适合用作存档的键。
- `name` 用于显示。玩家可以改显示名，宿主解析名字时，账号名对不上就会退回到显示名（见 `bridge::resolvePlayer`），所以不能拿它当身份。

## c2acfed5a7

>  Returns a selector usable for calling the API.
>
>  The xuid comes first, since it cannot be forged. An empty xuid, in offline mode, falls
>  back to the uuid and then to the name. On a fall back to `Name`,
>  [`PlayerSel::is_stable`] is false, which a permission or economy decision should treat
>  with care, since a name goes through the display-name fallback; see the `sel` module.

返回一个可以用来调用接口的选择器。

优先用 xuid，因为它伪造不了。xuid 为空（离线模式）时，依次退回到 uuid 和名字。退回到 `Name` 时，[`PlayerSel::is_stable`] 为 false，权限或经济判断遇到这种情况要当心，因为名字会经过显示名的回退；见 `sel` 模块。

## 47b70908b2

>  Whether there is a reliable identity, an xuid or a uuid. Ask this before using one as
>  a permission key.

有没有可靠的身份，即 xuid 或 uuid。拿它当权限的键之前，先问一下这个。

## bc9e6122d5

>  A subscription handle. Dropping it unsubscribes.
>
>  [`Listener::forget`] keeps it alive until the mod unloads. The host removes whatever
>  remains at unload, really calling `removeListener` and reporting a failure rather than
>  staying silent.

订阅句柄。丢弃它就会取消订阅。

[`Listener::forget`] 让它一直存活到模组卸载。卸载时宿主会移除剩下的订阅，真的调用 `removeListener`，失败了会报告，不会不声不响。

## bb05f0f0c0

>  The id of the subscribed event.

所订阅的事件的 id。

## 9c9dbaac0e

>  Gives up automatic unsubscription. The closure leaks with it and lives until the
>  process ends.

放弃自动取消订阅。闭包随之泄漏，一直存活到进程结束。

## e90ff7e1ba

>  Dispatch priority. A lower value runs first, aligned with 0..4 in the ABI.

分发的优先级。值越小越先运行，和 ABI 里的 0 到 4 对齐。

## 521fc782f7

>  An earlier generation called this `EventRef`. The name is kept and points at the same
>  type.

早先的一代管它叫 `EventRef`。这个名字保留着，指向同一个类型。

## a250a3799d

>  An earlier generation called the priority `EventPriority`.

早先的一代把优先级叫作 `EventPriority`。

## 31fbb0b68f

>  Event id constants.
>
>  An event id is a string, and the cost of a typo is a subscription that succeeds
>  silently while the callback never fires once. The host reports a failed resolution
>  and lists nearby ids (§5.3), and these constants move that to compile time.
>
>  Two kinds. A registry event always gets the full name `ll::event::<ClassName>`, with
>  no category segment in between. The host accepts a unique suffix too, but a suffix
>  becomes ambiguous once upstream adds an event of the same name. A synthetic event, a
>  bare name, is one Pier builds with a native detour to fill a point LL does not cover.
>
>  Each entry states whether it can be cancelled. Calling `Event::cancel()` on an event
>  that cannot be is a harmless no-op, and it leaves the impression of having blocked it.

事件 id 常量。

事件 id 是字符串，拼错的代价是订阅悄悄成功，回调却一次都不触发。宿主会报告解析失败，并列出相近的 id（§5.3），这些常量把这一步提前到了编译期。

分两类。注册表里的事件总是用完整的名字 `ll::event::<类名>`，中间没有分类段。宿主也接受唯一的后缀，但上游一旦加了同名的事件，后缀就会有歧义。合成事件的名字只有一个单词，是 Pier 用原生钩子构造的，用来补上 LL 没有覆盖的点。

每一项都写明了能不能取消。对不能取消的事件调用 `Event::cancel()`，不会出错，也不起作用，还会留下「已经拦住了」的印象。

## 306eee805c

>  Whether this event can be cancelled.
>
>  * `Some(true)`: it can, and `Event::cancel()` takes effect;
>  * `Some(false)`: it cannot, and `cancel()` returns `Err` saying which event to block;
>  * `None`: it is not in the tables. An event a third-party mod emits itself, or a new
>    upstream event these tables have not caught up with, both land here. `cancel()` writes
>    back as usual and blocks nothing, and it can confirm nothing on the caller's behalf.

这个事件能不能取消。

- `Some(true)`：能，`Event::cancel()` 会生效；
- `Some(false)`：不能，`cancel()` 会返回 `Err`，说明应该去拦哪个事件；
- `None`：不在表里。第三方模组自己发出的事件，或者这些表还没跟上的上游新事件，都会落在这里。`cancel()` 照常写回，什么都拦不住，也无法替调用方确认任何事情。

## b34f15340a

>  Why an observation-only event cannot be cancelled, and which event to block instead.

只能观察的事件为什么不能取消，以及应该改去拦哪个事件。

## 8f8b975694

>  Cancellable, as a `Cancellable<ServerPlayerEvent>`. Cancelling refuses the join.

可以取消，类型是 `Cancellable<ServerPlayerEvent>`。取消就是拒绝加入。

## cf508e8f45

>  Cancellable.

可以取消。

## d69c68f89a

>  Observation only; the player is already leaving and cannot be stopped.

只能观察；玩家已经在离开，拦不住。

## 7637154f67

>  Observation only.

只能观察。

## 92626f9a43

>  Cancellable. The payload carries `message`, and editing it rewrites what was said.

可以取消。载荷里有 `message`，修改它就改写了说的话。

## a1c215e706

>  Cancellable. A player mined a block.

可以取消。玩家挖掉了一个方块。

## 94e95767ed

>  Cancellable. `PlayerPlacingBlockEvent` is about to place while `Placed` is already done.

可以取消。`PlayerPlacingBlockEvent` 是即将放置，`Placed` 是已经放好。

## 57a74ff43d

>  Observation only; it is already done. Use [`PLAYER_PLACING_BLOCK`] to block it.

只能观察；已经放好了。要拦请用 [`PLAYER_PLACING_BLOCK`]。

## ed1ab63199

>  Cancellable. Right-clicking a block: opening a chest, pressing a button, using a tool.

可以取消。右键点方块：开箱子、按按钮、使用工具。

## 8b839aacd9

>  Cancellable. Banning a food or a potion is blocked here and not at
>  [`PLAYER_USE_ITEM_COMPLETE`].

可以取消。禁止某种食物或药水要在这里拦，不要在 [`PLAYER_USE_ITEM_COMPLETE`] 拦。

## 3adcb0b9e8

>  Cancellable. Note that it cannot tell attacking a player from attacking a mob; that
>  needs [`PLAYER_ATTACK_TARGET`], a synthetic event whose payload carries `targetIsPlayer`.

可以取消。注意它分不清攻击的是玩家还是生物；要区分得用 [`PLAYER_ATTACK_TARGET`]，那是一个合成事件，载荷里带着 `targetIsPlayer`。

## d446a30c31

>  Cancellable, since the base `PlayerSneakEvent` is a `Cancellable<>`.

可以取消，因为基类 `PlayerSneakEvent` 是 `Cancellable<>`。

## cc7660e600

>  Cancellable, as above.

可以取消，同上。

## 0cf51c6202

>  Observation only: `PlayerSprintEvent` is not Cancellable, unlike Sneak.

只能观察：和潜行不同，`PlayerSprintEvent` 不是 Cancellable。

## 647fcc2332

>  Observation only, as above.

只能观察，同上。

## a950fdaeb1

>  Observation only. It is the base of `PlayerAttackEvent` and
>  `PlayerDestroyBlockEvent`; subscribe to those two to block a specific action.

只能观察。它是 `PlayerAttackEvent` 和 `PlayerDestroyBlockEvent` 的基类；要拦具体的动作，请订阅那两个。

## b1b2bd4c99

>  Observation only; a base class, as above.

只能观察；同样是基类。

## 39ec225f69

>  Observation only: `MobEvent` is not Cancellable.

只能观察：`MobEvent` 不是 Cancellable。

## f5f554e22e

>  Observation only; it is already done.

只能观察；已经发生了。

## 4e2c64b145

>  Observation only. Block changes are blocked through [`PLAYER_PLACING_BLOCK`],
>  [`PLAYER_DESTROY_BLOCK`] or [`BLOCK_DESTROY`], the last of which covers non-player sources.

只能观察。方块的变化要通过 [`PLAYER_PLACING_BLOCK`]、[`PLAYER_DESTROY_BLOCK`] 或 [`BLOCK_DESTROY`] 来拦，最后一个覆盖非玩家的来源。

## 2a47066225

>  Observation only, once per tick, so the test has to be cheap or use `Host::schedule`.

只能观察，每刻一次，所以判断必须很快，或者改用 `Host::schedule`。

## ebb5bf1edd

>  Cancellable. A command allowlist or an audit hooks here.

可以取消。命令白名单或者审计就挂在这里。

## 74740b3273

>  Cancellable. Something removed this cell, without asking who.
>
>  It fills the largest gap: an enderman taking a grass block, a wither smashing a wall,
>  a creeper crater, a silverfish burrowing into stone, `/setblock ... destroy`, another
>  plugin calling destroyBlock. None of these fired any event before, and plot protection
>  could only watch blocks vanish.
>
>  Payload: `x` `y` `z` `dim` `dropResources` `block`.
>  Note there is no who: the engine already dropped the source at this layer, and
>  inventing one would only mislead.

可以取消。有东西移除了这一格，不问是谁。

它补上了最大的一块空缺：末影人搬走草方块、凋灵撞碎墙、苦力怕炸出的坑、蠹虫钻进石头、`/setblock ... destroy`、另一个插件调用 destroyBlock。以前这些都不触发任何事件，地皮保护只能看着方块消失。

载荷：`x` `y` `z` `dim` `dropResources` `block`。注意里面没有「是谁」：引擎在这一层已经丢掉了来源，编一个出来只会误导人。

## 3e1b8708ce

>  Cancellable. Cancelling means the explosion does not happen at all, damage and blocks
>  alike.
>  Payload: `x` `y` `z` `dim` `radius` `maxResistance` `fire` `breaksBlocks`
>  `underwater` `sourceIsPlayer` `sourceId` `source`.

可以取消。取消就是这次爆炸完全不发生，伤害和方块都不受影响。

载荷：`x` `y` `z` `dim` `radius` `maxResistance` `fire` `breaksBlocks` `underwater` `sourceIsPlayer` `sourceId` `source`。

## 4bf50867e3

>  Cancellable. Water or lava is about to spread into a cell. It blocks a neighbor pouring
>  water on their own ground and having it flow across: the pour is legitimate and the
>  spreading step is the crossing.
>  Payload: the target cell `x` `y` `z` `dim`, the source cell `fromX` `fromY` `fromZ`,
>  plus `direction` and `liquid`.
>
>  A hot path: liquid spreads every tick, so the test has to be cheap.

可以取消。水或岩浆即将流进一格。用来拦这种情况：邻居在自己的地上倒水，水却流过了边界；倒水本身是合法的，流过去的那一步才越界。

载荷：目标格 `x` `y` `z` `dim`，来源格 `fromX` `fromY` `fromZ`，以及 `direction` 和 `liquid`。

这是热路径：液体每刻都在扩散，所以判断必须很快。

## 327e3804b1

>  Cancellable. Something fell from a height and trampled farmland into dirt, needing no
>  permission and leaving no log.
>  Payload: `x` `y` `z` `dim` `fallDistance` `byPlayer` `actor`, plus `_player` for a player.

可以取消。有东西从高处落下，把耕地踩回了泥土，不需要任何权限，也不留任何日志。

载荷：`x` `y` `z` `dim` `fallDistance` `byPlayer` `actor`，是玩家时还有 `_player`。

## 5fc694f145

>  Cancellable. A piston is about to push or pull a set of blocks. It blocks a cross-plot
>  piston machine.
>  Payload: the piston `x` `y` `z` `dim`, `facing:[x,y,z]` and `attached:[[x,y,z],...]`.

可以取消。活塞即将推动或拉回一组方块。用来拦跨地皮的活塞机器。

载荷：活塞的 `x` `y` `z` `dim`，`facing:[x,y,z]` 和 `attached:[[x,y,z],...]`。

## 1a873b5dda

>  Cancellable. Two chests are about to pair into a double chest.
>  A chest placed against the boundary pairs with the neighbor's, and opening the near half
>  shows everything in theirs. Container protection decides on the cell you clicked, and
>  that cell really belongs to the placer.
>  Payload: `x` `y` `z` `dim` `otherX` `otherY` `otherZ`.

可以取消。两个箱子即将合成一个大箱子。

贴着边界放的箱子会和邻居的箱子合并，打开靠自己这边的一半，就能看到对方那边的所有东西。容器保护按你点的那一格做判断，而那一格确实属于放箱子的人。

载荷：`x` `y` `z` `dim` `otherX` `otherY` `otherZ`。

## b249d90484

>  Cancellable. Cancelling means the drop is not spawned and the item disappears rather
>  than lying on the ground.
>  For anti-duplication and drop ownership. Payload: `x` `y` `z` `dim` `item` `count`
>  `throwTime`
>  `sourceIsPlayer` `source`.

可以取消。取消就是不生成这个掉落物，物品直接消失，不会躺在地上。

用于防刷和掉落物归属。载荷：`x` `y` `z` `dim` `item` `count` `throwTime` `sourceIsPlayer` `source`。

## eb80998645

>  Observation only. A weather change. Payload: `rainLevel` `rainTime` `lightningLevel`
>  `lightningTime`.

只能观察。天气变化。载荷：`rainLevel` `rainTime` `lightningLevel` `lightningTime`。

## e3b54f18b5

>  Cancellable through the engine's own `NotPossibleHere`, so the client shows the vanilla
>  message.
>  For someone else's bed, a game mode where the night must not be skipped, and a dimension
>  where a bed is a bomb.

可以取消，取消时用引擎自己的 `NotPossibleHere`，所以客户端显示的是原版的提示。

用于别人的床、不能跳过夜晚的玩法，以及床会爆炸的维度。

## b5c4996fe5

>  Observation only: the return is a reference to the item in the new slot, and cancelling
>  would mean inventing one out of nothing.
>  Payload: `from` `to` `item` `dim` `_player`.

只能观察：返回值是新槽位里那个物品的引用，取消就意味着凭空编出一个物品。

载荷：`from` `to` `item` `dim` `_player`。

## de00a71fd2

>  Observation only: cancelling here leaves the player holding the item forever. Finishing
>  eating, drinking or lowering a spyglass.
>  Banning a food blocks the start of the use at [`PLAYER_USE_ITEM`].

只能观察：在这里取消，玩家会一直拿着这个物品放不下来。吃完、喝完，或者放下望远镜时触发。

禁止某种食物，要在 [`PLAYER_USE_ITEM`] 拦住使用的开始。

## e59d99f2db

>  Cancellable. A player swaps equipment with an armor stand, which is neither a container
>  nor a block, so neither protection sees it.

可以取消。玩家和盔甲架交换装备；盔甲架既不是容器也不是方块，两种保护都看不到它。

## c8279c4893

>  Cancellable. Left-clicking an item frame to take the item: not breaking a block, since
>  the frame remains, and not hitting an actor, since the frame is a block.

可以取消。左键点物品展示框取出物品：展示框还在，所以不算破坏方块；展示框是方块，所以也不算攻击实体。

## 74c3432406

>  Cancellable. Rotating the item in a frame, the other half of the frame pair: the
>  attack half is [`PLAYER_ATTACK_ITEM_FRAME`]. Neither is a block place nor a hit on
>  an actor, so nothing else sees it.

可以取消。旋转展示框里的物品，是展示框这一对事件的另一半，攻击的那一半是 [`PLAYER_ATTACK_ITEM_FRAME`]。两者都不是放方块，也不是攻击实体，所以别的事件都看不到。

## 86948b1930

>  Cancellable. Editing the text of a sign already placed. Placing a sign is a block
>  place and was already covered; changing its text was neither a place nor an interact.

可以取消。修改一块已经放好的告示牌上的文字。放告示牌属于放方块，早就覆盖了；改文字既不是放置也不是交互。

## 23acb3f180

>  Not raised by this host. The name is kept for mods that already subscribe to it, and
>  [`is_cancellable`] answers `Some(false)` for it so no mod mistakes it for a gate. It is
>  left out of [`ALL_SYNTHETIC`], which lists only what the host registers.

这个宿主不会触发它。保留这个名字，是为了已经订阅它的模组；[`is_cancellable`] 对它回答 `Some(false)`，免得有模组把它当成关卡。它没有列在 [`ALL_SYNTHETIC`] 里，那里只列宿主注册的事件。

## dc975b87ae

>  Cancellable. A player attacks a target. The payload carries `targetIsPlayer`, which is
>  how the pvp flag tells attacking a player from attacking a mob, and `targetKnown`: when
>  the target could not be read it is 0 and `targetIsPlayer` is 1, so a pvp rule refuses.

可以取消。玩家攻击一个目标。载荷里有 `targetIsPlayer`，pvp 开关靠它区分攻击玩家和攻击生物；还有 `targetKnown`：目标读不出来时它为 0，同时 `targetIsPlayer` 为 1，于是 pvp 规则会拒绝。

## 2549d51966

>  Cancellable. A player changes game mode, including through `/gamemode` and calls from
>  other plugins.

可以取消。玩家切换游戏模式，包括通过 `/gamemode` 和其他插件的调用。

## e6fa16a312

>  Cancellable. Dropping an item, covering both dropping by hand and dragging out of the
>  inventory UI.

可以取消。丢出物品，手动丢弃和从物品栏界面拖出去都算。

## 4720767e53

>  Cancellable. Right-clicking an actor: villager trading, feeding an animal, shearing.

可以取消。右键点实体：和村民交易、喂动物、剪羊毛。

## 53f112fb27

>  Cancellable. A player steps on a pressure plate or a tripwire. Throttled internally at
>  250 ms per (player, position).

可以取消。玩家踩上压力板或绊线。内部按（玩家，位置）节流，每 250 毫秒最多触发一次。

## 9b35d24bf2

>  Cancellable. As above, but for a non-player actor, using a separate throttle table.

可以取消。同上，但针对非玩家的实体，用的是另一张节流表。

## 7130073807

>  Cancellable. A player launches a projectile: a snowball, an ender pearl, an arrow, a
>  trident, a crossbow firework.

可以取消。玩家发射一个弹射物：雪球、末影珍珠、箭、三叉戟、弩射出的烟花。

## 7e2b0719f2

>  Cancellable. A player pushes an actor. Throttled internally.

可以取消。玩家推动一个实体。内部有节流。

## 88509e041d

>  Cancellable. A player mounts a vehicle.

可以取消。玩家骑上一个载具。

## 058bf452ff

>  Cancellable. A non-player actor mounts a vehicle, such as a villager in a boat or a pig
>  in a minecart.
>  The payload uses `passenger` and `passengerId` instead of `_player`.

可以取消。非玩家的实体骑上一个载具，比如村民坐船、猪坐矿车。

载荷用 `passenger` 和 `passengerId`，没有 `_player`。

## 3c0f0da1fc

>  Cancellable. A player picks up a projectile actor such as an arrow or a trident.

可以取消。玩家捡起一个弹射物实体，比如箭或三叉戟。

## 82b1284138

>  Cancellable. A player opens a container.

可以取消。玩家打开一个容器。

## cf2789af42

>  Observation only, emitted before origin, for recording who started mining which cell.

只能观察，在原函数之前发出，用于记录是谁开始挖哪一格。

## f270d1b73e

>  Cancellable, and the target can be rewritten. A player changes dimension, from any
>  cause: a portal, a teleport, `/execute in`, a respawn.
>
>  Payload: `from` `to` `to_x` `to_y` `to_z` `use_portal` `respawn` `_player`.
>  `use_portal` is what separates walking into a portal from being teleported, and a rule
>  that treats the two alike will surprise whoever wrote it.
>
>  Answering with `to`, or with all three of `to_x` `to_y` `to_z`, reroutes the transfer;
>  the engine then performs it itself. Teleporting from the callback instead re-enters
>  this event while the player still stands in the portal, and repeats every tick.

可以取消，目标也可以改写。玩家切换维度，任何原因都算：传送门、传送、`/execute in`、重生。

载荷：`from` `to` `to_x` `to_y` `to_z` `use_portal` `respawn` `_player`。`use_portal` 用来区分走进传送门和被传送，把两者一视同仁的规则，会让写它的人吃惊。

回答 `to`，或者同时回答 `to_x` `to_y` `to_z` 三个，就能给这次转移改道，之后由引擎自己完成。如果改为在回调里传送，玩家还站在传送门里时会再次进入这个事件，每刻都重复一遍。

## b373442ccc

>  Cancellable. A frame is about to become a nether portal.
>
>  It fires after the engine has measured the frame and found the fire, so a consumer
>  needs no geometry of its own. Payload: `dim` `x` `y` `z`.
>
>  There is no player: fire reaches a frame from a dispenser, from lightning and from
>  spreading, and a rule keyed on a player would miss those.

可以取消。一个框架即将变成下界传送门。

它在引擎量好框架、找到火之后触发，所以使用方不需要自己做任何几何计算。载荷：`dim` `x` `y` `z`。

里面没有玩家：火可以来自发射器、闪电和蔓延，按玩家判断的规则会漏掉这些情况。

## 427ac8ac61

>  Observation only. A hopper transfers an item. Payload: `x` `y` `z` `slot` `item` `count`
>  `old_item` `old_count`.

只能观察。漏斗转移了一个物品。载荷：`x` `y` `z` `slot` `item` `count` `old_item` `old_count`。

## e93caa82ee

>  Cancellable. A player uses an item on a block, placing it or right-clicking with it.

可以取消。玩家对一个方块使用物品，放置它，或者拿着它右键。

## 23412f2b2d

>  The ids of every synthetic event. Comparing it against [`super::list()`] at startup
>  shows which capability packages this host was built with.

所有合成事件的 id。启动时拿它和 [`super::list()`] 对比，就能看出这个宿主编入了哪些能力包。

## b490036b36

>  Custom commands.
>  # Command registration is one way
>  Bedrock offers no route to deregister a command, so a registered command lives until the server
>  stops. While a mod is disabled the host mutes the callback rather than removing it, and re-
>  enabling resumes it. There is therefore no `unregister` and no handle that deregisters on drop,
>  since such a handle would suggest it can be undone when it cannot.
>
>  It follows that registering a command inside `on_enable` has to survive being called several
>  times, as a hot reload does. A re-registration under the same name only swaps the callback and
>  does not rebuild the command.
>
>  # Two shapes
>
>  [`register`] takes the whole raw line, which suits parsing it yourself. [`CommandBuilder`]
>  declares typed overloads that the engine parses and completes, so a player sees argument hints
>  on the client.

自定义命令。

**命令注册是单向的**

基岩版没有注销命令的途径，所以注册过的命令会一直存在到服务器停止。模组被禁用时，宿主屏蔽它的回调，不移除命令；重新启用后恢复。所以这里没有 `unregister`，也没有在丢弃时注销的句柄，这样的句柄会让人以为可以撤销，而实际上撤销不了。

因此，在 `on_enable` 里注册命令必须能承受被调用多次，热重载就会这样调用。用同一个名字再次注册只会换掉回调，不会重建命令。

**两种形状**

[`register`] 接收整行原始文本，适合自己解析。[`CommandBuilder`] 声明带类型的重载，由引擎解析并补全，玩家在客户端能看到参数提示。

## ef0a4d0a19

>  Registers a command that takes the whole raw line.
>
>  [`Invocation::raw`] in the callback is everything after `/name`, parsed by you.

注册一条接收整行原始文本的命令。

回调里的 [`Invocation::raw`] 是 `/name` 之后的全部内容，由你自己解析。

## a0996c7638

>  Begins declaring a command with overloads.

开始声明一条带重载的命令。

## 519b895390

>  Registers a static enum for a [`ParamType::Enum`] parameter to reference.

注册一个静态枚举，供 [`ParamType::Enum`] 参数引用。

## cf518412e2

>  Registers a soft enum that can change at runtime, for a [`ParamType::SoftEnum`]
>  parameter to reference.

注册一个可以在运行时改变的软枚举，供 [`ParamType::SoftEnum`] 参数引用。

## ac9b2a1124

>  One command invocation.

一次命令调用。

## c46a05b5c8

>  The raw argument text. For a command registered through `CommandBuilder` this is the
>  argument SNBT.

原始的参数文本。对通过 `CommandBuilder` 注册的命令，这是参数的 SNBT。

## b0c2ae4570

>  Returns one success output.

返回一条成功输出。

## edeaf36db0

>  Returns one error output. It is a separate channel from success, the client colors it
>  differently, and a failed command must not use the success channel, which would make a
>  command block decide wrongly.

返回一条错误输出。它和成功输出是两个通道，客户端用不同的颜色显示；执行失败的命令不能用成功通道，否则命令方块会做出错误的判断。

## 7da3422876

>  The origin.
>
>  A command from [`CommandBuilder`] carries `{name,type,dim,x,y,z}`. One from
>  `register` carries only the name, so `kind` is -1 there and [`CommandOrigin::is_console`]
>  and [`CommandOrigin::player_name`] cannot tell who sent it; a command that needs to
>  know is registered through `CommandBuilder`. The two shapes are told apart by
>  structure: a bare word such as `Steve` is itself valid SNBT, so a successful parse
>  alone does not mean the structured form.

命令的来源。

来自 [`CommandBuilder`] 的命令带着 `{name,type,dim,x,y,z}`。来自 `register` 的命令只带名字，这时 `kind` 为 -1，[`CommandOrigin::is_console`] 和 [`CommandOrigin::player_name`] 都无法判断是谁发的；需要知道是谁的命令，要通过 `CommandBuilder` 注册。两种形状按结构区分：`Steve` 这样的单词本身就是合法的 SNBT，所以光是解析成功，并不说明是结构化的那种。

## d71439a074

>  Which overload matched. Only a command registered through `CommandBuilder` has one.

匹配上的是哪个重载。只有通过 `CommandBuilder` 注册的命令才有。

## 599a875725

>  Reads a named argument. An omitted optional argument gives `None`.

读取一个命名参数。省略了的可选参数返回 `None`。

## 4addc5a90c

>  The declaration of one overload.

一个重载的声明。

## 63c7406aa3

>  A **literal**: a fixed word the player types, and a node in the command tree.
>
>  # Why this is not an enum with one value
>
>  An `Enum` parameter carries `EnumAutocompleteExpansion`, so the client lists its
>  values, but its data type is `Enum` and not `ChainedSubcommand`: there is no tree
>  node, and the client has nothing to narrow as the player types. A literal is what
>  `/scoreboard objectives add` is made of, and it is the only form that filters.
>
>  The word comes back in the arguments under `name`, so the handler reads it the
>  same way as any other parameter, rather than keying on an overload index that
>  moves when an overload is inserted above it.
>
>  Sibling words that take the same arguments are one enum instead (CONTRACT.md 6.1).

一个**字面量**：玩家要输入的固定单词，也是命令树上的一个节点。

**为什么不用只有一个值的枚举**

`Enum` 参数带有 `EnumAutocompleteExpansion`，客户端会列出它的值，但它的数据类型是 `Enum`，并非 `ChainedSubcommand`：没有树节点，玩家输入时客户端没有东西可以缩小范围。`/scoreboard objectives add` 就是由字面量组成的，只有这种形式能随输入过滤。

这个单词会以 `name` 为名回到参数里，所以处理函数读它的方式和读其他参数一样，不用依赖重载的下标；在上面插入一个重载时，下标就会变。

接收相同参数的几个并列单词，应当合成一个枚举（CONTRACT.md 6.1）。

## e850a43af8

>  Where a command came from.

命令从哪里来。

## 7f5a8d02fe

>  The player name when the origin is a player.
>
>  Note this is a name and not an identity: a permission decision uses `kind` plus a
>  own player table, for the reason [`crate::sel`] gives.

来源是玩家时，玩家的名字。

注意这是名字，不是身份：权限判断要用 `kind` 加上自己的玩家表，原因见 [`crate::sel`]。

## 6892d5bdb4

>  A command with typed overloads.
>
>  Each overload parses a different input: two that accept the same words leave the engine
>  to pick one, and the handler cannot rely on which (contract §6.1). Sibling words with the
>  same arguments are one enum parameter; a word with arguments of its own is a literal.
>
>  ```ignore
>  command::register_enum("plot_simple", &[("menu", 0), ("help", 1), ("status", 2)])?;
>  command::builder("plot", "Plots", CommandPermission::Any)
>      .overload(|o| o.required_enum("simple", ParamType::Enum, "plot_simple"))
>      .overload(|o| o.text("verb", "rate").required("score", ParamType::Int))
>      .register(handler)?;
>  ```

带类型化重载的命令。

每个重载解析的输入必须不同：两个重载接受同样的单词时，由引擎从中挑一个，处理函数不能依赖挑的是哪一个（契约 §6.1）。接收相同参数的并列单词合成一个枚举参数；带有自己参数的单词用字面量。

```rust
command::register_enum("plot_simple", &[("menu", 0), ("help", 1), ("status", 2)])?;
command::builder("plot", "Plots", CommandPermission::Any)
    .overload(|o| o.required_enum("simple", ParamType::Enum, "plot_simple"))
    .overload(|o| o.text("verb", "rate").required("score", ParamType::Int))
    .register(handler)?;
```

## 00194c3e83

>  Registers it. At least one overload is required, since the host refuses outright with
>  none, so this stops it earlier and says why rather than leaving a bare registration
>  failure to be guessed at.

注册这条命令。至少要有一个重载：没有重载时宿主会直接拒绝，所以这里提前拦下并说明原因，免得只留下一个光秃秃的注册失败让人去猜。

## 11dd8cd4b7

>  The permission a command needs. The values mirror `CommandPermissionLevel`.

命令需要的权限。取值对应 `CommandPermissionLevel`。

## 4adcbe26ee

>  The type of one parameter inside one overload.
>
>  `Enum` and `SoftEnum` also need an enum name, declared through the
>  [`OverloadBuilder::required_enum`] pair of methods.

一个重载里一个参数的类型。

`Enum` 和 `SoftEnum` 还需要一个枚举名，通过 [`OverloadBuilder::required_enum`] 这一对方法声明。

## 94d6015e69

>  The spelling on the ABI. The host dispatches on this string and a typo drops the whole
>  overload.

ABI 上的写法。宿主按这个字符串分派，拼错会让整个重载被丢弃。

## cb2f99fe49

>  Changes the values of a soft enum.

修改软枚举的值。

## 0367127a18

>  Forms: the three screens sent to a player, whose callback arrives asynchronously.
>
>  # The callback runs at most once and may never run
>
>  When the player answers, or closes the form, the callback runs once on the server
>  thread. But if the mod is disabled before the player answers, the host mutes that
>  callback and it never arrives. Therefore:
>
>  * the callback is an `FnOnce` and is freed once it has run;
>  * on the muted path the memory holding the closure is leaked on purpose. The code able
>    to free it lives in a dynamic library that may already be unloaded, and freeing it is
>    the real use-after-free.
>
>  It follows that cleanup that has to happen does not belong in a form callback: a player
>  may never click.

表单：发给玩家的三种界面，回调是异步到达的。

**回调最多运行一次，也可能永远不运行**

玩家回答或者关掉表单时，回调在服务器线程上运行一次。但如果玩家回答之前模组被禁用了，宿主会屏蔽这个回调，它永远不会到达。因此：

- 回调是 `FnOnce`，运行一次以后就被释放；
- 在被屏蔽的那条路径上，存放闭包的内存是有意泄漏的。能释放它的代码在一个可能已经卸载的动态库里，去释放它才是真正的释放后使用。

所以，必须执行的清理工作不该放在表单回调里：玩家可能永远不点。

## 24433f368e

>  A custom form: input fields, toggles, dropdowns and sliders.

自定义表单：输入框、开关、下拉框和滑块。

## c9567eb78d

>  A slider.
>
>  An out-of-range default, or a `(max-min)` that is not a whole multiple of `step`,
>  makes the Bedrock client render nothing of the form at all, which the player sees as
>  it opening and disappearing. The host clamps and warns, and getting it right before
>  passing it in is better.

一个滑块。

默认值超出范围，或者 `(max-min)` 不是 `step` 的整数倍，基岩版客户端会把整个表单都画不出来，玩家看到的是表单一打开就消失。宿主会截断并发出警告，但传进来之前就弄对更好。

## 8df2b9c1e1

>  A simple form: a column of buttons.

简单表单：一列按钮。

## e3dcf22a34

>  A button with an icon. `image_type` is `"path"` or `"url"`.

一个带图标的按钮。`image_type` 是 `"path"` 或 `"url"`。

## 60aa658d0b

>  Sends it.
>
>  A form with no button at all cannot be pressed and can only be closed. It is stopped
>  here in advance, because it is almost always a list that assembled empty rather than
>  something intended.

发送这个表单。

一个按钮都没有的表单没法点，只能关掉。这里提前拦下，因为这种情况几乎总是一个拼装出来是空的列表，很少是有意为之。

## 23b71c0ac9

>  The value one control of a custom form returns.

自定义表单里一个控件返回的值。

## 5aa5db8534

>  A modal form: some text and two buttons.

模态表单：一段文字和两个按钮。

## f1265555c7

>  A player's answer to a form.

玩家对一个表单的回答。

## c1f1c477ac

>  Reads the value of one control of a custom form.

读取自定义表单里一个控件的值。

## 3d5277c5c0

>  Reads the text of the selected item of one control of a custom form.

读取自定义表单里一个控件所选那一项的文字。

## f9c0c95ca7

>  Scoreboards.
>
>  Everything goes through one multiplexed slot, `scoreboard_op`, where `op` decides what
>  happens. This layer wraps each op as a named method and separates a score that cannot be
>  read from a score of 0, which the bare slot reports as the same empty output
>  (contract §5.2).

计分板。

所有操作都经过一个复用的槽位 `scoreboard_op`，由 `op` 决定做什么。这一层把每个 op 包成一个具名方法，并把「读不到分数」和「分数为 0」分开；直接用这个槽位时，两者给出的是同样的空输出（契约 §5.2）。

## 74d56c4c4e

>  The scoreboard facade. Zero sized.

计分板门面。零大小。

## e5788f4d53

>  Creates a dummy objective. It fails when one of that name already exists.

创建一个 dummy 计分项。同名的计分项已经存在时失败。

## d2014576f5

>  Reads one score.
>
>  A person with no score on that objective gives `Ok(None)`, and only an objective that
>  does not exist is an `Err`. The bare slot collapses both of those and a score of 0 into
>  the same empty output, so this separates them by whether the output is empty and leaves
>  a nonexistent objective for the op itself to report.

读取一个分数。

这个人在这个计分项上没有分数时返回 `Ok(None)`，只有计分项不存在才是 `Err`。直接用这个槽位时，这两种情况和分数为 0 会合成同一个空输出，所以这里按输出是否为空把它们分开，计分项不存在的情况留给 op 本身去报告。

## a360ed1a8b

>  Sets it to `value` and returns the value after the write.

设为 `value`，返回写入以后的值。

## dc94e55258

>  Erases the score of this person on this objective, which is not setting it to 0.

抹掉这个人在这个计分项上的分数，这和设为 0 是两回事。

## a361ca5ad6

>  The display slot of a scoreboard.

计分板的显示位置。

## 736e487373

>  The host dispatches on this string.

宿主按这个字符串分派。

## 6b7649230c

>  One objective.

一个计分项。

## 52106ea6a9

>  Raw packet interception.
>  This layer handles closure ownership, the panic fence, and gathering `{ptr,len}` into a `&[u8]`.
>  It interprets not one byte of the body: version differences, field layout and codecs all live on
>  the caller's side. A loader usable across versions cannot understand the wire format of every
>  version at once.
>
>  # Threads: read this before writing any state
>
>  An inbound callback runs on the thread pumping that connection and an outbound one on the thread
>  that started the send. Usually that is the server thread, but an async flush means it is not
>  guaranteed, which is why the closure bound is `Send + Sync` and not `Send`: the same closure may
>  be entered by several threads at once.
>
>  With several subscribers, each sees the output of the previous one in registration order, and
>  the first Drop wins with everything after it skipped. The subscription table is snapshotted
>  before dispatch, so registering and deregistering inside a callback is safe.

原始数据包拦截。

这一层负责闭包的所有权、panic 围栏，以及把 `{ptr,len}` 收集成 `&[u8]`。包体它一个字节都不解读：版本差异、字段布局和编解码都在调用方那边。一个能跨版本使用的加载器，没办法同时理解每个版本的线上格式。

**线程：写任何状态之前请先读这一段**

收到的包，回调在泵送这个连接的线程上运行；发出的包，回调在发起发送的线程上运行。通常是服务器线程，但有异步刷新，所以并不保证。这就是闭包的约束是 `Send + Sync`、只有 `Send` 不够的原因：同一个闭包可能同时被几个线程进入。

有多个订阅者时，按注册顺序，每一个看到的是前一个的输出，第一个 Drop 生效，之后的全部跳过。分发前会给订阅表拍一个快照，所以在回调里注册和注销是安全的。

## a931b992ff

>  The packet a callback receives. Valid only during the callback.

回调收到的数据包。只在回调期间有效。

## ce4592af63

>  The id of this connection. Stable across packets and usable as the key of per-connection
>  state.

这个连接的 id。在多个数据包之间保持不变，可以用作按连接保存状态的键。

## b9e63da166

>  The peer address as `ip:port`. For diagnostics; use `conn_id` as a key.

对端地址，形如 `ip:port`。用于诊断；当作键请用 `conn_id`。

## c31c93cfe1

>  The body, without the header, since the packet id and sub id are already decoded.
>
>  That is deliberate: a rewriter only supplies a new body and the host re-encodes the
>  header from `edit`, so changing a packet id is a field assignment rather than varint
>  surgery.

包体，不含包头，因为数据包 id 和子 id 已经解码好了。

这是有意的：改写方只提供新的包体，宿主根据 `edit` 重新编码包头，所以改数据包 id 只是给一个字段赋值，不用去拆变长整数。

## 12819af4f8

>  Replaces the body. It may be called several times in one callback and the last one
>  counts.

替换包体。一次回调里可以调用多次，以最后一次为准。

## 6881fd1c3d

>  Rewrites the packet id, for remapping onto the numbering of another version.

改写数据包 id，用于映射到另一个版本的编号上。

## 40e2971d9a

>  Registers a packet interceptor.
>
>  The closure needs `Send + Sync`; see the thread section of the module header. A callback
>  is not guaranteed to be on the server thread and may be entered by several threads at
>  once.

注册一个数据包拦截器。

闭包需要 `Send + Sync`；见模块开头的线程一节。回调不保证在服务器线程上，也可能同时被几个线程进入。

## f1b0a6a6d0

>  As [`Packets::intercept`], but the interceptor runs only for the listed packet ids.
>
>  This is the form to use whenever the ids are known, which is nearly always. The
>  unfiltered form costs one callback per packet in each direction it asked for, chunk
>  data included, while here the host passes a packet no subscriber listed through before
>  taking any lock. `ids` are `MinecraftPacketIds` values in `0..1024`; an id outside that
>  range is ignored by the host, and a list with nothing left is refused.
>
>  On a host without this slot the call falls back to the unfiltered form and filters the
>  id in the trampoline, so the interceptor still sees only the ids it asked for, at the
>  cost of one callback per packet.

和 [`Packets::intercept`] 一样，但拦截器只对列出的数据包 id 运行。

只要知道 id，就该用这种形式，而这几乎总是成立的。不过滤的形式在它要求的每个方向上，每个数据包都要调用一次回调，区块数据也算在内；这里，没有任何订阅者列出的数据包，宿主在加锁之前就直接放行。`ids` 是 `0..1024` 范围内的 `MinecraftPacketIds` 值；超出范围的 id 会被宿主忽略，过滤后一个不剩的列表会被拒绝。

在没有这个槽位的宿主上，调用会退回到不过滤的形式，并在跳板函数里按 id 过滤，所以拦截器看到的仍然只是它要的那些 id，代价是每个数据包一次回调。

## 18c8099de4

>  Registers an observer of connection opens and closes.

注册一个观察连接打开和关闭的回调。

## 6862589955

>  A registered packet interceptor.
>
>  Dropping it deregisters. `forget()` keeps it alive until the mod unloads, and it is
>  explicit because registering and discarding looks identical in code to registering and
>  forgetting to keep the returned value, and the latter is a bug. The host clears what
>  remains when the mod unloads, at teardown stage 90.

一个已注册的数据包拦截器。

丢弃它就会注销。`forget()` 让它一直存活到模组卸载；要明确调用它，是因为「注册后丢弃」和「注册后忘了保存返回值」在代码里看起来一模一样，而后者是个 bug。模组卸载时，宿主会在拆除的第 90 阶段清掉剩下的拦截器。

## f12ca8eb9e

>  Gives up deregistering on drop and keeps it alive until the mod unloads.
>
>  This step is explicit because registering and discarding looks identical in code to
>  registering and forgetting to keep the returned value, and the latter is a bug where the
>  interceptor disappears the moment it is installed.

放弃丢弃时的注销，让它一直存活到模组卸载。

这一步要明确调用，因为「注册后丢弃」和「注册后忘了保存返回值」在代码里看起来一模一样，而后者是一个 bug：拦截器刚装上就没了。

## 22a7239caf

>  The direction of a packet.

数据包的方向。

## af24a2fc4b

>  Which directions to select at registration.

注册时选择哪些方向。

## 6285e6a50c

>  What the callback does with this packet.

回调对这个数据包怎么处理。

## 3b30444910

>  The two states of a connection.
>
>  A close is the only reliable signal to clear the state of a connection: one that never
>  completed the login handshake never becomes a Player and no player event covers it.

连接的两种状态。

关闭是清除一个连接的状态时唯一可靠的信号：没有完成登录握手的连接永远不会成为 Player，任何玩家事件都覆盖不到它。

## 1330077b5b

>  Server runtime control: freezing, stepping and warping ticks, plus performance sampling broken
>  down by subsystem.
>  These do not live in [`crate::host`]: that layer speaks about the host itself, meaning
>  scheduling, executing commands and the operating system, and holds for another game, while a
>  tick is the rhythm of the world simulation and is a game concept.
>
>  # A hook is installed and never removed
>
>  The detour installs lazily on the first call and stays. The reason is that a control call may
>  come from a command handler executing inside the tick, where removing a hook is unsafe. The idle
>  cost is one predictable branch per frame.
>
>  # Players still move while frozen
>
>  Freezing stops mobs, blocks, redstone and time. Movement is client authoritative and the network
>  runs outside the level tick, so players keep walking and chatting.

服务器运行时的控制：冻结、单步和加速刻，以及按子系统拆分的性能采样。

这些不放在 [`crate::host`] 里：那一层说的是宿主本身，即调度、执行命令和操作系统，换一个游戏也成立；而刻是世界模拟的节奏，是游戏里的概念。

**钩子装上以后永不卸下**

钩子在第一次调用时才安装，之后一直留着。原因是控制调用可能来自正在刻内部执行的命令处理函数，在那里卸下钩子并不安全。空闲时的开销是每帧一次可以预测的分支。

**冻结时玩家仍然能动**

冻结会停下生物、方块、红石和时间。移动由客户端决定，网络在关卡的刻之外运行，所以玩家照样能走动、聊天。

## c1e28fd205

>  The server runtime facade. Zero sized.

服务器运行时的门面。零大小。

## bff5b114f5

>  Freezes or unfreezes the world.

冻结或解冻世界。

## 67b72d3b9e

>  Valid only while frozen: lets `n` more frames through.

只在冻结时有效：再放过 `n` 帧。

## 98af3bd080

>  The time warp. `0 < factor <= 100`, below 1 is slow motion and 1.0 is normal.

时间加速。`0 < factor <= 100`，小于 1 是慢动作，1.0 是正常速度。

## 12b909f84e

>  Opens a sampling window of `ticks` frames, from 1 to 12000. Only one window at a time.

开启一个 `ticks` 帧的采样窗口，1 到 12000。同一时间只有一个窗口。

## 77ebb2a2a7

>  Takes the sampling report.
>
>  Sampling still running gives `Ok(None)`, and one window succeeds exactly once.
>
>  The per-item times are inclusive of nesting, so they are read side by side and not
>  summed.

取回采样报告。

还在采样时返回 `Ok(None)`，一个窗口只会成功一次。

各项时间包含了嵌套的部分，所以要并排对照着看，不要相加。

## 67da0a1104

>  Whether the simulation is paused.

模拟是否暂停。

## a91fd16732

>  The wall-clock period of the last frame in seconds. At 20 TPS it is 0.05.
>
>  This is the frame period, sleep included, and not the time the server spent computing
>  the tick: on an idle server it reads 0.05 all the same. It is kept for callers that want
>  the raw engine value; [`Server::tps`] and [`Server::mspt`] are the numbers a monitor
>  wants.
>
>  The host returns -1.0 when it cannot read it, translated here into an `Err`: a negative
>  frame duration would have a caller compute a negative TPS without noticing.

上一帧实际经过的时间，单位秒。20 TPS 时是 0.05。

这是帧的周期，包含休眠，和服务器计算这一刻所花的时间是两回事：空闲的服务器上它一样读到 0.05。保留它，是给想要引擎原始值的调用方用的；监控要的数字是 [`Server::tps`] 和 [`Server::mspt`]。

宿主读不出来时返回 -1.0，这里转成 `Err`：负的帧时长会让调用方在不知不觉中算出负的 TPS。

## 8fc5d90921

>  Ticks per second over the last 5 seconds of wall clock.
>
>  Measured by counting the `Level::tick` calls that really run and dividing by elapsed
>  time, so it stays right under the tick warp (reads above 20), the tick freeze (reads
>  0) and lag (reads below 20). The reciprocal of one frame period is not this number:
>  it reads above 20 on about half the frames of an idle server and reads 20 while the
>  world is frozen.
>
>  `Err` until the host has sampled its first frame, which takes two ticks after start.

最近 5 秒实际时间里每秒运行的刻数。

测量方法是数真正运行过的 `Level::tick` 调用次数，再除以经过的时间，所以在刻加速（读数高于 20）、刻冻结（读数为 0）和卡顿（读数低于 20）时都准确。一帧周期的倒数和这个数是两回事：空闲的服务器上，大约一半的帧算出来高于 20，世界冻结时它仍然是 20。

宿主采样到第一帧之前返回 `Err`，启动后要等两刻。

## e7ad72520d

>  As [`Server::tps`], over a window of `window_seconds` (1..=60, clipped by the host).

和 [`Server::tps`] 一样，但窗口是 `window_seconds` 秒（1 到 60，超出由宿主截断）。

## 2412008f68

>  Milliseconds spent computing each tick, averaged over the last 5 seconds.
>
>  This excludes the idle sleep between frames: a healthy server reads a few
>  milliseconds and only approaches 50 when it is saturated. `tick_delta_time() * 1000`
>  is not this number, it is the frame period and reads about 50 on an idle server.

最近 5 秒里，每一刻计算所花的毫秒数的平均值。

它不包括帧与帧之间空闲的休眠：健康的服务器读数是几毫秒，只有满负荷时才接近 50。`tick_delta_time() * 1000` 和这个数是两回事，那是帧的周期，空闲的服务器上大约是 50。

## 53100036f1

>  As [`Server::mspt`], over a window of `window_seconds` (1..=60, clipped by the host).

和 [`Server::mspt`] 一样，但窗口是 `window_seconds` 秒（1 到 60，超出由宿主截断）。

## 098985ca99

>  The host and the system themselves: the capabilities that involve no game concept.
>
>  What belongs here is decided by whether it speaks about the host or the world. Run
>  state, handing a task back to the server thread, executing a command and asking the
>  operating system its name all hold for another game as well; players, blocks and items
>  do not.

宿主和系统本身：不涉及任何游戏概念的能力。

放在这里的标准，是它说的是宿主还是世界。运行状态、把任务交回服务器线程、执行命令、问操作系统叫什么名字，换一个游戏也都成立；玩家、方块和物品就不行。

## 0c963d2c74

>  The host facade. Zero sized and freely `Copy`.

宿主门面。零大小，可以随意 `Copy`。

## 58c6623d75

>  Which stage the server is in. The ABI marks it thread safe, so any thread may ask.

服务器处在哪个阶段。ABI 把它标为线程安全，所以任何线程都可以问。

## 02e338635d

>  Hands a piece of work back to the server thread to run as soon as possible. Thread
>  safe.
>
>  The closure is boxed and handed to the host, taken back and run inside the callback,
>  and freed immediately afterwards. Ownership stays on the mod side throughout, which
>  follows contract §3: no ownership crosses the boundary, what is passed is an opaque
>  pointer and the host only hands it back unchanged.
>
>  It goes through `schedule_for`, which carries a mod handle, and not the ownerless
>  `schedule`. A task on the ownerless slot still fires after the mod unloads and jumps
>  into an already unmapped code segment. A task with a handle is accounted per mod by the
>  host and the whole batch is discarded at unload, with a warning suggesting you cancel
>  them yourself. The returned [`TaskId`] works with [`Host::cancel`].

把一件工作交回服务器线程，尽快运行。线程安全。

闭包被装箱交给宿主，在回调里取回并运行，运行完立刻释放。所有权自始至终留在模组这一边，符合契约 §3：没有所有权跨过边界，传过去的是一个不透明的指针，宿主只是原样交回来。

它走的是带模组句柄的 `schedule_for`，没有用不带主人的 `schedule`。不带主人的槽位上的任务，在模组卸载以后照样会触发，跳进一段已经卸载的代码。带句柄的任务由宿主按模组记账，卸载时整批丢弃，并发出一条警告，建议你自己取消它们。返回的 [`TaskId`] 可以交给 [`Host::cancel`]。

## e52b6742e0

>  As above, but runs after `delay`. Thread safe.
>
>  It takes a `Duration` rather than a bare millisecond count: whether the 5 in
>  `schedule_after(5, ...)` means five milliseconds or five seconds is answered only by the
>  parameter name, which is invisible at the call site. Putting the unit in the type makes
>  the call site carry the answer.
>
>  The ABI side is in milliseconds, so this converts once. A duration exceeding `u64`
>  milliseconds is clamped to the maximum rather than wrapping to a small number, since
>  wrapping would turn run in a year into run immediately.

同上，但在 `delay` 之后运行。线程安全。

它接收 `Duration`，不接收裸的毫秒数：`schedule_after(5, ...)` 里的 5 是五毫秒还是五秒，只有参数名能回答，而参数名在调用处看不见。把单位放进类型里，调用处自己就带着答案。

ABI 那一侧用毫秒，所以这里换算一次。超过 `u64` 毫秒的时长会被截断到最大值，不会回绕成一个小数字，否则「一年后运行」就成了「立刻运行」。

## aba0fcb620

>  Cancels a task that has not run yet. A ticket that already ran, was already cancelled,
>  or does not belong to this mod returns `false`.
>
>  Note that cancelling only voids the ticket. The closure itself is neither called nor
>  freed when the host tears down and is reclaimed when the process ends. Avoiding a leak
>  means not scheduling and cancelling in bulk on a hot path.

取消一个还没运行的任务。已经运行过、已经取消过，或者不属于这个模组的票据返回 `false`。

注意取消只是作废票据。闭包本身既不会被调用，也不会在宿主拆除时被释放，要等进程结束才回收。想避免泄漏，就不要在热路径上大量地排任务再取消。

## 2b43df493a

>  How many tasks under this mod have not run. Suited to asserting 0 in `on_unload`: a
>  host that cannot count returns `Err`, which the assertion receives in place of a 0.

这个模组名下还有多少任务没运行。适合在 `on_unload` 里断言它为 0：宿主数不出来时返回 `Err`，断言拿到的是一个错误，没有拿到 0。

## 136c003aab

> use try_pending_tasks: this answers 0 when the host cannot count, which passes the assertion it exists for

请用 `try_pending_tasks`：宿主数不出来时，这个函数回答 0，正好能通过它本该把关的那个断言

## 3b431d702c

>  How many tasks under this mod have not run, and 0 when the host cannot count.

这个模组名下还有多少任务没运行；宿主数不出来时返回 0。

## 400a5318cc

>  Executes a command as the console and returns its output.
>
>  The two `Err` cases stay apart: a missing slot, meaning the host is too old, and the
>  command itself failing, where `success == false` and the output holds the error.

以控制台的身份执行一条命令，返回它的输出。

两种 `Err` 是分开的：缺少槽位，意思是宿主太旧；命令本身执行失败，这时 `success == false`，输出里是错误信息。

## 930cb496ee

>  Lists every event id the host can currently resolve.
>
>  Printing this list when a subscription fails is far more useful than a bare subscribe
>  failed (contract §5.3: a log line has to answer what to do about it).

列出宿主当前能解析的所有事件 id。

订阅失败时打印这份列表，比光秃秃的一句「订阅失败」有用得多（契约 §5.3：日志要回答该怎么办）。

## 4dce39bff2

>  Information at the operating-system level. `prop` comes from `sys::PIER_SYS_*`.

操作系统层面的信息。`prop` 取自 `sys::PIER_SYS_*`。

## 70e11b8202

>  Information at the server level. `prop` comes from `sys::PIER_SRV_*`.

服务器层面的信息。`prop` 取自 `sys::PIER_SRV_*`。

## 8a587c3c05

>  The network protocol version of the server, derived from the running build's
>  game version.
>
>  **The host fails this on a version it does not know and on any pre-release
>  build.** An `Err` means cannot-be-determined, which must stay apart from a
>  protocol version of 0 (contract §5.2) — that is why this returns a `Result`.
>
>  When it fails, read [`Self::game_sem_version`] and map it yourself. Do not reach
>  for [`Self::level_protocol_version`]: that is the save's tag, and on any world
>  older than the server it is a different number.

服务器的网络协议版本，根据正在运行的构建的游戏版本推出。

**宿主不认识这个版本，或者它是任何预发布构建时，宿主会让这个调用失败。** `Err` 表示无法确定，必须和协议版本为 0 分开（契约 §5.2），所以它返回 `Result`。

失败时，读 [`Self::game_sem_version`]，自己去映射。不要去拿 [`Self::level_protocol_version`]：那是存档的标记，在任何比服务器旧的世界上，它都是另一个数。

## 7e27424864

>  The protocol the **level** was last written by (`LevelData::mNetworkVersion`).
>
>  This says how old the save is. It is not what the server speaks, and using it as
>  such is the bug this pair of accessors was split to prevent.

**关卡**最后一次被写入时所用的协议（`LevelData::mNetworkVersion`）。

它说明存档有多旧，和服务器使用的协议无关；把它当成服务器的协议，正是拆出这一对访问函数要防止的错误。

## 75ef2388ac

>  `"major.minor.patch"` of the running build, from `CurrentGameSemVersion`.
>
>  Carry your own version→protocol table off this when [`Self::protocol_version`]
>  does not know a build: a Pier release should not be on the critical path for
>  supporting a BDS that shipped yesterday.

正在运行的构建的 `"major.minor.patch"`，取自 `CurrentGameSemVersion`。

[`Self::protocol_version`] 不认识某个构建时，以它为依据自带一张版本到协议的对照表：支持一个昨天才发布的 BDS，不应该卡在等 Pier 发新版上。

## edfe4ac6f9

>  The BDS version string, from `Common::getGameVersionString`.

BDS 的版本字符串，取自 `Common::getGameVersionString`。

## ae9bf52943

>  Reads an environment variable.
>
>  Failing to read and reading an empty string are the same empty string here: the failure
>  bit of this slot on the ABI means only that the host could not perform the read, and an
>  existing empty variable in a process environment is the same as an absent one.

读取一个环境变量。

读取失败和读到空字符串，在这里都是同一个空字符串：这个槽位在 ABI 上的失败位只表示宿主没能执行读取，而进程环境里一个存在但为空的变量，和不存在是一样的。

## 2690d0c577

>  Sets an environment variable. It affects this process only.

设置一个环境变量。只影响这个进程。

## cc09941b44

>  Whether this host runs under Wine, and `Err` when the host cannot tell.
>
>  Worth asking on its own: some Windows APIs behave differently under Wine than on real
>  Windows, and the symptom usually appears far from the cause.

这个宿主是否运行在 Wine 下；宿主分辨不出来时返回 `Err`。

这个问题值得单独问：有些 Windows API 在 Wine 下的行为和真正的 Windows 不同，症状通常出现在离原因很远的地方。

## 1264a50999

> use try_is_wine: this answers false when the host cannot tell

请用 `try_is_wine`：宿主分辨不出来时，这个函数回答 false

## d7a4915d45

>  Whether this host runs under Wine, and `false` when the host cannot tell.

这个宿主是否运行在 Wine 下；宿主分辨不出来时返回 `false`。

## 6d0c8c9563

>  The operating system version string.

操作系统的版本字符串。

## f291b41eb3

>  The world facade.
>
>  This accessor forms no cycle with `Host`: `world` really does need
>  `Host::execute_command` to assemble a `/fill`, but the entry point on each side is only
>  the `get()` of a zero-sized facade and no state is shared. The layering check declares
>  this edge explicitly.

世界门面。

这个访问函数和 `Host` 之间不构成循环：`world` 确实需要用 `Host::execute_command` 拼出 `/fill`，但两边的入口都只是零大小门面的 `get()`，不共享任何状态。分层检查明确声明了这条边。

## 7dd34aa1d7

>  Server runtime control: freezing ticks, warping them, and performance sampling.

服务器运行时的控制：冻结刻、加速刻，以及性能采样。

## 44000bf370

>  The run stage of the server, mirroring `ll::GamingStatus`.

服务器的运行阶段，对应 `ll::GamingStatus`。

## 4dba5c30b1

>  The cross-mod event bus: broadcast, with no return value.
>
>  Complementary to [`crate::service`]: the bus is one to many, returns nothing and guarantees no
>  order, while a service is one to one, returns a value and holds its name exclusively.
>
>  # A mod does not receive its own publish
>
>  A mod does not receive its own publish. Notifying yourself is a direct function call, and
>  publishing to yourself is the one cycle no depth limit can tell apart. A cross-mod cycle, A to B
>  to A, is caught by the depth cap, and hitting it discards the innermost publish with a log line.
>
>  # The whole family is thread safe and a callback runs on the publisher's thread
>
>  A callback therefore must not touch world state; touching it means `Host::schedule` back onto
>  the server thread.

跨模组事件总线：广播，没有返回值。

和 [`crate::service`] 互补：总线是一对多，不返回任何东西，也不保证顺序；服务是一对一，有返回值，名字独占。

**模组收不到自己的发布**

模组收不到自己发布的消息。通知自己可以直接调用函数，而发布给自己是唯一一种任何深度上限都分辨不出的循环。跨模组的循环（A 到 B 再到 A）由深度上限截住，碰到上限时丢弃最内层的那次发布，并记一行日志。

**整组接口都是线程安全的，回调在发布方的线程上运行**

所以回调不能碰世界的状态；要碰，就用 `Host::schedule` 回到服务器线程。

## 0afde0f4f6

>  Subscribes to a topic.
>
>  A `true` from the callback is a veto, which only [`publish_vetoable`] reads; an
>  ordinary [`publish`] ignores the return value.

订阅一个主题。

回调返回 `true` 表示否决，只有 [`publish_vetoable`] 会读它；普通的 [`publish`] 忽略返回值。

## b060a53e22

>  Broadcasts. It returns how many subscribers really ran, and 0 is a normal result
>  meaning nobody is listening.

广播。返回实际运行了多少个订阅者，0 是正常结果，表示没有人在听。

## 6dc13c4d26

>  Broadcasts and collects the veto bit.

广播，并收集否决位。

## 1ada6d7f97

>  How many subscribers this topic currently has, across every mod.
>
>  For skipping the cost of assembling a payload nobody will read.

这个主题当前有多少订阅者，所有模组加在一起。

用来在没人会读的时候，省掉拼装载荷的开销。

## d299c73b8f

>  One subscription. Dropping it unsubscribes.
>
>  [`Subscription::forget`] keeps it alive until the mod unloads, when the host clears it.

一个订阅。丢弃它就会取消订阅。

[`Subscription::forget`] 让它一直存活到模组卸载，那时由宿主清掉。

## d373a429f6

>  The result of one vetoable broadcast.

一次可否决广播的结果。

## 62356cacb8

>  Cross-mod services: one question, one answer.
>
>  Complementary to the bus, which broadcasts and returns nothing. A name is exclusive:
>  one name has one provider, and taking one is refused by the host, whose log names the
>  holder, which is far easier to diagnose than the later registration winning.
>
>  [`call`] gives a bare `String` while [`call_json`] deserializes straight into the type
>  you want, where a parse failure is a definite error and not the fallback of an
>  `unwrap_or`.
>
>  The errors are categorized in [`CallError`]: no such service and the service refusing
>  are two different things. The first usually means a missing dependency or a misspelled
>  name, and the second is a refusal by business logic.

跨模组服务：一问一答。

和总线互补，总线是广播，不返回任何东西。名字是独占的：一个名字只有一个提供方，再去占会被宿主拒绝，宿主的日志会写出谁占着这个名字；这比后注册的悄悄胜出好排查得多。

[`call`] 返回一个裸的 `String`，[`call_json`] 直接反序列化成你要的类型，解析失败是明确的错误，不会变成某个 `unwrap_or` 的兜底值。

错误在 [`CallError`] 里分了类：服务不存在和服务拒绝是两回事。前者通常意味着缺少依赖或者名字拼错，后者是业务逻辑的拒绝。

## 403cbb0391

>  Calls a service and returns the raw reply text.

调用一个服务，返回原始的回答文本。

## 221493aac3

>  Calls a service and deserializes the reply from JSON into `T`.
>
>  This is the recommended form. It replaces the `from_str`, `as_array`, `get`,
>  `unwrap_or(0)` boilerplate and turns a malformed reply into a real error rather than a
>  perfectly ordinary looking 0.
>
>  ```ignore
>  #[derive(serde::Deserialize)]
>  struct World { dim: i32, #[serde(rename = "plotSize")] plot_size: i32 }
>
>  let worlds: Vec<World> = service::call_json("plot:worlds", "{}")?;
>  ```

调用一个服务，把回答从 JSON 反序列化成 `T`。

推荐用这种形式。它省掉了 `from_str`、`as_array`、`get`、`unwrap_or(0)` 这一串样板代码，并把格式不对的回答变成真正的错误，不会变成一个看上去再正常不过的 0。

```rust
#[derive(serde::Deserialize)]
struct World { dim: i32, #[serde(rename = "plotSize")] plot_size: i32 }

let worlds: Vec<World> = service::call_json("plot:worlds", "{}")?;
```

## 0c17935b9e

>  As above, with the request side serialized through `serde` as well.

同上，请求那一侧也通过 `serde` 序列化。

## 128030fe95

>  Calls, returning `None` on `NotFound` and raising every other error as usual.
>
>  For an optional integration that uses a dependency when installed and degrades
>  otherwise. Such code used to be written as
>  `let Ok(x) = call(..) else { return default }`, which swallowed a service error too.

调用，遇到 `NotFound` 返回 `None`，其他错误照常抛出。

用于可选的集成：依赖装了就用，没装就降级。这类代码以前写成 `let Ok(x) = call(..) else { return default }`，服务的错误也一起被吞掉了。

## 7a01ca6ca0

>  Whether any mod provides this name.
>
>  An earlier generation substring-matched inside the JSON text of `service_list`, so a
>  name that is a prefix of another matched wrongly, with `plot` hitting `plot:worlds`.
>  This really parses it.

有没有模组提供这个名字。

早先的一代在 `service_list` 的 JSON 文本里做子串匹配，一个名字是另一个名字的前缀时就会误判，比如 `plot` 会命中 `plot:worlds`。这里是真正解析以后再判断。

## 39c4982926

>  The raw JSON of the service listing, unparsed.
>
>  For when [`list`] cannot parse it, or the host added a field this layer does not know.

服务列表的原始 JSON，不做解析。

用于 [`list`] 解析不了，或者宿主加了这一层不认识的字段的时候。

## 7e1f1e1423

>  Every currently registered service.

当前已注册的所有服务。

## 16b3470af0

>  Who is calling the service callback that is running right now; see `service_caller`
>  in `abi.h`.
>
>  This is the one thing a provider can trust about a request. The request body names
>  whoever it likes, and a provider keying an owner or an acting player on it has only
>  the sender's word; this asks the host, which knows whose `service::call` is on the
>  stack. Nested calls report the innermost one.
>
>  `None` outside a callback, when the call came without a mod handle, or on a host too
>  old to have the slot. The last case matters: a provider that wants attribution must
>  decide what to do when there is none, and "attribute to nobody" and "refuse" are both
>  defensible while "attribute to the empty name" is not.

正在运行的这个服务回调，是谁调用的；见 `abi.h` 里的 `service_caller`。

这是提供方对一个请求唯一能信任的东西。请求体想写谁就写谁，按它判断所有者或执行操作的玩家，凭的只是发送方的一面之词；这个函数去问宿主，宿主知道调用栈上是谁的 `service::call`。嵌套调用时，报告最内层的那一次。

在回调之外、调用没有带模组句柄，或者宿主太旧、没有这个槽位时，返回 `None`。最后一种情况很重要：想要归属的提供方必须决定拿不到归属时怎么办，「不算到任何人头上」和「拒绝」都说得过去，「算到空名字头上」就说不过去了。

## f2441ca015

>  Registers a service.
>  ```ignore
>  let _reg = service::register("plot:worlds", |_name, _req| {
>      Ok(serde_json::to_string(&worlds()).unwrap_or_default())
>
>  })?;
>  ```
>
>  A few host-side rules, which cannot be changed here and are worth knowing:
>  * the callback runs synchronously on the caller's thread, so nothing slow belongs in it;
>  * a name already taken fails outright and does not displace the holder;
>  * a service stays reachable while its mod is disabled, since resolving it inside
>    another mod's `on_load` would otherwise fail; refusing while disabled is left to
>    the provider.

注册一个服务。

```rust
let _reg = service::register("plot:worlds", |_name, _req| {
    Ok(serde_json::to_string(&worlds()).unwrap_or_default())
})?;
```

几条宿主一侧的规则，这里改不了，值得知道：

- 回调在调用方的线程上同步运行，所以里面不能放任何慢的操作；
- 名字已被占用时直接失败，不会把占用者挤掉；
- 模组被禁用期间，服务仍然可以调用，否则在另一个模组的 `on_load` 里解析它就会失败；禁用期间要不要拒绝，由提供方自己决定。

## a4d58111cc

>  Registers a service whose request and reply are both JSON.
>
>  It removes the `to_string` and `from_str` boilerplate on both sides, and a failed
>  deserialization becomes a definite business error returned to the caller rather than
>  the provider writing its own `unwrap_or_default`.

注册一个请求和回答都是 JSON 的服务。

它省掉了两边的 `to_string` 和 `from_str` 样板代码；反序列化失败会变成一个明确的业务错误返回给调用方，不需要提供方自己写 `unwrap_or_default`。

## ace26401f9

>  A service registration handle. Dropping it deregisters.
>
>  [`Registration::forget`] keeps it alive until the mod unloads, and the host clears what
>  remains at unload.

服务注册的句柄。丢弃它就会注销。

[`Registration::forget`] 让它一直存活到模组卸载，卸载时宿主会清掉剩下的注册。

## 8e6da50d45

>  Why a call failed.

调用失败的原因。

## 7071a214aa

>  The result of a call.

一次调用的结果。

## 43abb348b0

>  The registration record of one service.

一个服务的注册记录。

## a2aa82e684

>  The same-toolchain fast lane: a direct function-table call that bypasses JSON.
>
>  The division with [`crate::service`]: service is a cross-language `(name, JSON) -> JSON`
>  channel that always holds, while a lane passes raw data and vtable pointers and holds
>  only when both sides were built by the same toolchain. A fingerprint mismatch yields no
>  pointer and falls back to service.
>
>  The fingerprint has to be computed by the caller and `0` is invalid: this slot hands
>  the vtable over as is, a consumer interprets it through its own type layout, and
>  skipping the check is type confusion. A `0` would read as anyone may connect.
>  [`list`] shows which lanes exist and passes no pointer at all.
>
>  The discipline for each call after acquiring one is in [`Lane::with`].

同工具链快速通道：绕开 JSON，直接调用函数表。

和 [`crate::service`] 的分工：服务是跨语言的 `(名字, JSON) -> JSON` 通道，总是成立；快速通道传递原始的数据指针和函数表指针，只在两边由同一个工具链构建时成立。指纹对不上时一个指针都不交出，退回到服务。

指纹必须由调用方计算，`0` 是无效的：这个槽位把函数表原样交出去，使用方按自己的类型布局去解读它，跳过检查就是类型混淆。`0` 会被理解成「谁都可以连」。[`list`] 显示有哪些通道，不交出任何指针。

获取之后每次调用要遵守的规矩，写在 [`Lane::with`] 里。

## 5067fac84c

>  Catches a panic inside a lane callback.
>
>  The table functions of a publisher are all `extern "C"`, and a panic crossing an
>  `extern "C"` boundary is undefined behavior.
>  This belongs on the first line of every table function. What runs inside is this mod's
>  own business code, no less likely to panic than anywhere else, while the consequence
>  here is far worse: the caller is another mod and its frames are on the stack.
>
>  On a panic it returns `fallback` and logs. `fallback` must be the value this table
>  function uses for cannot-answer and not for no: treating a panic as a definite negative
>  answer lets a bug make a decision that belongs to business logic.

截住快速通道回调里的 panic。

发布方的表函数都是 `extern "C"`，panic 跨过 `extern "C"` 边界是未定义行为。每个表函数的第一行都应该是它。里面运行的是这个模组自己的业务代码，出 panic 的可能性和别处一样，但在这里后果要严重得多：调用方是另一个模组，它的栈帧就在栈上。

发生 panic 时，返回 `fallback` 并记录日志。`fallback` 必须是这个表函数表示「答不上来」的值，不能是表示「否」的值：把 panic 当成一个明确的否定回答，等于让一个 bug 替业务逻辑做了决定。

## 3bf77e0ba4

>  Publishes a lane.
>
>  # Safety
>
>  The caller must guarantee that:
>
>  * `data` and `vtable` stay valid until this lane is withdrawn. The host interprets not
>    one byte of them and copies nothing, and only compares for equality and marks
>    liveness;
>  * `vtable` really points at the C-layout table `C::Table` and its shape agrees with
>    `C::FINGERPRINT`;
>  * `retain` and `release` call back into no `lane_*` slot, which would self-deadlock.

发布一个快速通道。

**安全性**

调用方必须保证：

- `data` 和 `vtable` 在这个通道撤回之前一直有效。宿主一个字节都不解读，也不复制任何东西，只比较是否相等并标记存活；
- `vtable` 确实指向按 C 布局的表 `C::Table`，它的形状和 `C::FINGERPRINT` 一致；
- `retain` 和 `release` 不回头调用任何 `lane_*` 槽位，否则会自己锁死自己。

## 2d35ed5fb5

>  Every lane. It passes no pointer at all, so it suits looking at what exists first.

所有快速通道。它不交出任何指针，所以适合先看看有什么。

## 080615a22d

>  Collects a lane call going through [`LaneStrSink`] into a `Vec<String>`.
>
>  An entry that is not UTF-8 is skipped with a warning rather than voiding the whole
>  batch: the other side may be written in another language, and one bad entry should not
>  make the whole query unanswerable.

把一次经过 [`LaneStrSink`] 的快速通道调用收集成 `Vec<String>`。

不是 UTF-8 的条目会被跳过，并记一条警告，不会让整批作废：另一边可能是用别的语言写的，一条坏数据不应该让整个查询答不上来。

## 4dd727f870

>  The raw JSON of the lane listing, unparsed.
>
>  For when [`list`] cannot parse it, or the host added a field this layer does not know.

快速通道列表的原始 JSON，不做解析。

用于 [`list`] 解析不了，或者宿主加了这一层不认识的字段的时候。

## 6fcc58518e

>  An acquired lane. Dropping it returns the lease.

一个已获取的快速通道。丢弃它就会归还租约。

## 9f8e3c1085

>  Whether the provider is still there.
>
>  Read with `Acquire`: the writer uses a release store and a relaxed read establishes no
>  happens-before relationship with it.

提供方是否还在。

用 `Acquire` 读取：写入方用的是 release 存储，而 relaxed 读取和它之间不建立任何 happens-before 关系。

## eca7c767ae

>  Runs a piece of code inside the provider. Returns `None` when the provider is gone.
>
>  `busy` goes up on the way in and down on the way out, and the host refuses to unload
>  the provider while it is up, so `FreeLibrary` cannot pull the stack frame out from
>  under you. It goes up **before** the liveness flag is read: the other order leaves a
>  window with the count at zero, where a retire passes its veto and unmaps the dylib
>  while this call is already on its way in.
>
>  The closure receives a [`LaneData`] rather than a raw `*mut c_void`, because the first
>  parameter of every function in a lane table is a `LaneData`, which is the other side's
>  self, and handing over a raw pointer would make the caller wrap it by hand at every
>  call site.

在提供方内部运行一段代码。提供方已经不在时返回 `None`。

进入时 `busy` 加一，离开时减一，它不为零时宿主拒绝卸载提供方，所以你的栈帧还在里面的时候，`FreeLibrary` 不会把代码卸掉。它要在读取存活标记**之前**加一：反过来的顺序会留下一个计数为零的窗口，退役流程在这个窗口里通过了否决、卸载了 dylib，而这次调用已经在进入的路上了。

闭包收到的是 [`LaneData`]，不是裸的 `*mut c_void`，因为通道表里每个函数的第一个参数都是 `LaneData`，也就是另一边的 self；交出裸指针的话，调用方在每个调用处都得自己包一层。

## f9239daac8

>  One publication. Dropping it withdraws the lane.

一次发布。丢弃它就会撤回通道。

## 8a93ef46fc

>  A span of UTF-8 text passed over a lane.
>
>  The producer decides how long the pointer stays valid, usually only for the duration of
>  one call (contract §3). A receiver keeping it has to copy it out.

通过快速通道传递的一段 UTF-8 文本。

指针有效多久由生产方决定，通常只在一次调用期间有效（契约 §3）。接收方要保留就得复制出来。

## 84810c8815

>  Borrows a `&str`. The lifetime of the result is not tracked by the type system, which
>  is the cost of crossing an ABI boundary, so it belongs only where it is constructed and
>  passed in immediately.

借用一个 `&str`。结果的生命周期不受类型系统跟踪，这是跨 ABI 边界的代价，所以它只该用在构造完立刻传进去的地方。

## d386b4776a

>  Reads it as a `&str`.
>
>  Content that is not valid UTF-8 returns `None` rather than going through
>  `from_utf8_unchecked`: the other side may be written in another language whose strings
>  do not necessarily pass this test.
>
>  # Safety
>  `ptr` and `len` must describe memory that is still valid now.

当作 `&str` 读取。

内容不是合法的 UTF-8 时返回 `None`，不走 `from_utf8_unchecked`：另一边可能是用别的语言写的，那种语言的字符串不一定通得过这项检查。

**安全性**

`ptr` 和 `len` 描述的内存此刻必须仍然有效。

## 275b5c3a3c

>  A contiguous span of same-typed elements passed over a lane.

通过快速通道传递的一段连续的同类型元素。

## 88884ac88d

>  # Safety
>  `ptr` and `len` must describe memory that is still valid now and really holds `T`.

**安全性**

`ptr` 和 `len` 描述的内存此刻必须仍然有效，并且里面确实是 `T`。

## 5a47524c19

>  Why acquiring a lane failed.

获取快速通道失败的原因。

## 84ec09a3dd

>  One sentence on what to do about it. This goes in the log rather than the enum name
>  (contract §5.3).

一句话说明该怎么办。这句话写进日志，不写进枚举名（契约 §5.3）。

## d7f397d51b

>  One lane contract: the function-table shape both sides agreed on.
>
>  `FINGERPRINT` has to change whenever the shape of the table changes, including field
>  order, parameter types and calling convention. It matches automatically when both sides
>  reference the same contract definition, the same version of the same crate. Copying an
>  identical constant by hand is wrong, and is exactly what it guards against.

一份快速通道契约：两边约定好的函数表形状。

表的形状一变，`FINGERPRINT` 就必须跟着变，包括字段顺序、参数类型和调用约定。两边引用同一份契约定义，也就是同一个 crate 的同一个版本时，指纹自动对得上。手抄一个相同的常量是错的，而它防的正是这种做法。

## 5968520e83

>  The registration record of a published lane.

已发布通道的注册记录。

## 40838c7d3a

>  The opaque context pointer of a lane function.
>
>  It wraps rather than using `*mut c_void` directly so that a table signature in the
>  contract reads as this parameter being that side's self.

通道函数的不透明上下文指针。

它包了一层，没有直接用 `*mut c_void`，这样契约里的表签名读起来就是：这个参数是那一边的 self。

## d0459889ff

>  The callback shape where a producer sinks one entry and a receiver copies it out.

生产方每输出一项、接收方就复制出来的回调形状。

## 137957f3e7

>  The key-value store: a mod's own persistent storage.
>
>  # This family is thread safe
>
>  The ABI marks `kvdb_*` as internally locked, one of the exceptions of contract §4, so any thread
>  may call it. Almost every other domain works on the server thread alone, and this is one of the
>  few usable straight from a `std::thread::spawn`.
>
>  # Paths are confined to the mod's own data directory
>
>  The host refuses `..` and an absolute path. The host owns the handle, force-closes it when the
>  mod unloads and warns, so forgetting to close loses no data while leaving a trace in the log.

键值存储：模组自己的持久化存储。

**这一组接口是线程安全的**

ABI 把 `kvdb_*` 标为内部加锁，是契约 §4 的例外之一，所以任何线程都可以调用。几乎所有其他领域都只能在服务器线程上工作，这是少数几个可以直接在 `std::thread::spawn` 里用的。

**路径限定在模组自己的数据目录里**

宿主拒绝 `..` 和绝对路径。句柄归宿主所有，模组卸载时宿主会强制关闭它并发出警告，所以忘了关闭不会丢数据，但会在日志里留下记录。

## 3b37237cfe

>  One open key-value store. Dropping it closes it.

一个已打开的键值存储。丢弃它就会关闭。

## 539cf36d84

>  Opens it, creating it when it does not exist.

打开它，不存在时创建。

## 17ccce09d9

>  Opens an existing one only. A store that does not exist is an `Err` and no empty one is
>  quietly created: reading a store that should hold data and creating a new one are two
>  different things.

只打开已存在的存储。存储不存在时返回 `Err`，不会悄悄建一个空的：读取一个本该有数据的存储，和新建一个存储，是两件不同的事。

## b0b344d3f5

>  Reads one key: `Ok(None)` for a key that does not exist, `Err` for a host without
>  `kvdb_get`. A database the host closed on its own, at an unload, also reads as
>  `Ok(None)`: the slot answers both with the same false, and the ABI cannot tell them
>  apart.
>
>  Both gates apply even after `open` has succeeded: `kvdb_get` sits after `kvdb_open` in
>  the table at a larger offset, and the first being covered does not imply the second is.

读取一个键：键不存在时返回 `Ok(None)`，宿主没有 `kvdb_get` 时返回 `Err`。宿主自己关掉的数据库（卸载时）读起来也是 `Ok(None)`：槽位对这两种情况回答同一个 false，ABI 分不清它们。

即使 `open` 已经成功，两道关卡也照样要检查：`kvdb_get` 在表里排在 `kvdb_open` 之后，偏移更大，前者够得着并不代表后者也够得着。

## 64056d2b94

>  Reads one key, with `None` for every case [`KvDb::try_get`] keeps apart.

读取一个键，[`KvDb::try_get`] 区分开的所有情况在这里都是 `None`。

## 696844bea9

>  Deletes one key. A key that never existed also counts as a success.

删除一个键。从来不存在的键也算成功。

## edcfcde881

>  Every key-value pair. The whole store is read into memory, so a large one needs care.

所有的键值对。整个存储都会读进内存，所以大的存储要当心。

## 7e8efdf34b

>  Economy.
>
>  # The backend is delay-loaded, and the whole family degrades rather than crashing
>
>  Without an economy backend installed, or with it disabled, each slot returns its own failure
>  value. This layer translates those into `Err`, with [`balance`] the one exception; see its own
>  documentation.
>
>  # Amounts are never negative and a transfer is taxed
>
>  The backend refuses a negative amount itself. [`transfer`] takes a cut according to the
>  `pay_tax` the backend is configured with, so the recipient receives `val - val * pay_tax` and
>  not `val`. Moving the full amount means separate [`add`] and [`reduce`] calls. The argument of
>  [`set`] is the target balance and not a delta.

经济。

**后端是延迟加载的，整组接口在缺少后端时降级，不会崩溃**

没有安装经济后端，或者后端被禁用时，每个槽位返回各自的失败值。这一层把它们转成 `Err`，只有 [`balance`] 例外；见它自己的文档。

**金额从不为负，转账要扣税**

后端自己会拒绝负的金额。[`transfer`] 按后端配置的 `pay_tax` 抽成，收款方收到的是 `val - val * pay_tax`，不是 `val`。要转出全额，就分别调用 [`add`] 和 [`reduce`]。[`set`] 的参数是目标余额，不是差额。

## 0392d66677

>  The balance.
>
>  It returns `Err` and not -1: a real balance is never negative, so a negative value can
>  only mean the question cannot be answered, because the xuid is empty, the database
>  failed, or the backend is absent.
>
>  Note this read is not free of side effects: the backend opens an account at the
>  configured default for an xuid it has not seen.

余额。

它返回 `Err`，不返回 -1：真实的余额从不为负，所以负值只能说明这个问题答不上来，原因是 xuid 为空、数据库出错，或者后端不在。

注意这次读取有副作用：对一个没见过的 xuid，后端会按配置的默认值开一个账户。

## cd6f86b4e3

>  Sets it to a target balance.

设为一个目标余额。

## 09fa4c2fd9

>  Transfers. A `from == to` is refused by the backend, and the amount the recipient
>  receives is already taxed; see the module documentation.

转账。`from == to` 会被后端拒绝，收款方收到的金额已经扣过税；见模块文档。

## 32fe1a0b4b

>  The transactions of the last `seconds` seconds, one per line, and `Err` for a host
>  without the history slot, which an empty list would hide.

最近 `seconds` 秒内的交易，每行一笔；宿主没有 history 槽位时返回 `Err`，空列表会把这一点掩盖掉。

## 1156057617

> use try_history: this answers an empty list when the host has no history slot

请用 `try_history`：宿主没有 history 槽位时，这个函数回答的是空列表

## 751334f799

>  The transactions of the last `seconds` seconds, and an empty list when the host cannot
>  read them.

最近 `seconds` 秒内的交易；宿主读不出来时返回空列表。

## ca768ce5ed

>  Clears transactions older than `seconds` seconds, and `Err` for a host that cannot.

清除早于 `seconds` 秒的交易；宿主做不到时返回 `Err`。

## e138792db6

> use try_clear_history: this does nothing, silently, on a host that cannot clear

请用 `try_clear_history`：在不能清除的宿主上，这个函数什么都不做，也不提示

## 3678ab71da

>  Clears transactions older than `seconds` seconds, doing nothing on a host that cannot.

清除早于 `seconds` 秒的交易；宿主做不到时什么都不做。

## d0f6d82821

>  The top `top_n` of the rich list, one per line, and `Err` for a host without the
>  ranking slot.

富豪榜的前 `top_n` 名，每行一个；宿主没有 ranking 槽位时返回 `Err`。

## 4a22cd74fc

> use try_ranking: this answers an empty list when the host has no ranking slot

请用 `try_ranking`：宿主没有 ranking 槽位时，这个函数回答的是空列表

## eaf07ec354

>  The top `top_n` of the rich list, and an empty list when the host cannot read it.

富豪榜的前 `top_n` 名；宿主读不出来时返回空列表。

## 6f60955f39

>  Registers a callback that runs before the event, where returning `false` vetoes the
>  transaction.
>
>  Several mods may each register their own without overwriting one another, and
>  registering the same function pointer twice is idempotent.
>  The host accounts per module and removes them when the mod unloads.
>
>  # The callback is global and not one per call
>
>  The ABI takes a raw function pointer here with no `user` parameter, so it cannot hold a
>  closure that captured its environment. This layer therefore takes an `fn` and not an
>  `impl FnMut`, which would need the state hidden in a global that is still touched after
>  the mod unloads. Carrying state means a `static` inside the mod itself, cleared in
>  `on_unload`.

注册一个在事件之前运行的回调，返回 `false` 会否决这笔交易。

多个模组可以各注册自己的，互不覆盖；同一个函数指针注册两次只算一次。宿主按模块记账，模组卸载时移除它们。

**回调是全局的，不是每次调用一个**

ABI 在这里接收一个裸函数指针，没有 `user` 参数，所以它装不下一个捕获了环境的闭包。因此这一层接收 `fn`，不接收 `impl FnMut`：后者需要把状态藏在一个全局变量里，而模组卸载以后还会有人碰它。要带状态，就在模组自己里面用一个 `static`，并在 `on_unload` 里清空。

## 3e8a42d512

>  Registers a callback that runs after the event. The return value is ignored.

注册一个在事件之后运行的回调。返回值被忽略。

## 729042b005

>  Registers a before-callback that can capture its environment. Returning `false` vetoes
>  the transaction.
>
>  There is one per mod: calling again replaces the previous one rather than running both.
>  Running several means dispatching inside the closure yourself. This is not laziness:
>  the ABI has no `user` parameter, so which closure can only be answered by a global on
>  this side, and one global holds one.

注册一个可以捕获环境的事件前回调。返回 `false` 会否决这笔交易。

每个模组只有一个：再次调用会替换之前的那个，两个不会都运行。要运行好几个，就在闭包里自己分派。这样做有原因：ABI 没有 `user` 参数，所以「是哪个闭包」只能由这一侧的一个全局变量回答，而一个全局变量只装得下一个。

## 8361e5d4d3

>  As above, the after version. The return value is ignored, so the closure returns
>  nothing.

同上，事件后的版本。返回值被忽略，所以闭包不返回任何东西。

## c1d44da065

>  Assembles the four arguments the ABI passes into a [`MoneyEvent`].
>
>  For anyone writing their own `extern "C"` callback: call it on the first line of the
>  body and the rest is safe Rust.
>
>  # Safety
>  The four arguments must be the set the host passed during the callback, and `from` and
>  `to` are valid only for its duration.

把 ABI 传来的四个参数组装成一个 [`MoneyEvent`]。

给自己写 `extern "C"` 回调的人用：在函数体第一行调用它，剩下的就都是安全的 Rust。

**安全性**

这四个参数必须是宿主在回调期间传来的那一组，`from` 和 `to` 只在回调期间有效。

## a655afff7b

>  Catches a panic inside an economy callback.
>
>  A panic crossing `extern "C"` is undefined behavior, and an economy callback is called
>  directly by the host.
>  This wraps it, treating a panic as no veto and logging: a veto is the stronger action
>  and should not be triggered by a bug.

截住经济回调里的 panic。

panic 跨过 `extern "C"` 是未定义行为，而经济回调是由宿主直接调用的。它把回调包起来，发生 panic 时视为不否决，并记录日志：否决是更强的动作，不应该由一个 bug 触发。

## d1b7b1aa70

>  What an economy event does. `Unknown` carries a value this SDK does not list, which a
>  newer economy backend may send; a veto callback deciding on an unknown kind should
>  refuse rather than guess.

一次经济事件做了什么。`Unknown` 带着一个这个 SDK 没有列出的值，更新的经济后端可能会发出这样的值；否决回调遇到不认识的种类，应当拒绝，不要去猜。

## 9951801ad3

>  Maps the ABI value, keeping one outside the list as `Unknown`.

映射 ABI 的值，列表之外的值保留为 `Unknown`。

## 4ec0474faf

>  One economy event.

一次经济事件。

## d7d2f8bb60

>  The kind, with a value this SDK does not list kept as [`MoneyEventKind::Unknown`].

事件的种类；这个 SDK 没有列出的值保留为 [`MoneyEventKind::Unknown`]。

## f26309c616

>  NBT values: the carrier of all structured data across the boundary.
>
>  No struct is passed on the Pier ABI. Event payloads, forms, items, block states, actor
>  snapshots, command arguments, service requests and replies are all SNBT strings, which
>  makes reading a field out of some SNBT the most frequent operation in mod code.
>
>  Two families of accessors, with different meanings:
>
>  * `opt_*` returns an `Option`: give it to me if it is there and I will handle the rest.
>  * `get_*` returns a `Result`, where a missing key and a type mismatch are two different
>    errors and the message carries the key name and the actual type. A protection
>    decision, for permissions, plots or economy, uses this family and fails closed on an
>    `Err`. [`crate::event`] gives the consequence of collapsing them into one answer.

NBT 值：跨越边界的所有结构化数据的载体。

Pier 的 ABI 上不传递任何结构体。事件载荷、表单、物品、方块状态、实体快照、命令参数、服务的请求和回答，全都是 SNBT 字符串，所以从一段 SNBT 里读出某个字段，是模组代码里最常做的事。

有两族访问函数，含义不同：

- `opt_*` 返回 `Option`：有就给我，剩下的我自己处理。
- `get_*` 返回 `Result`，缺少键和类型不符是两种不同的错误，消息里带着键名和实际的类型。保护判断（权限、地皮、经济）用这一族，遇到 `Err` 就拒绝。把两者合成一种回答会有什么后果，见 [`crate::event`]。

## 653f0bc31f

>  Converts SNBT text into binary.

把 SNBT 文本转成二进制。

## 122aa2a5cd

>  Converts binary into SNBT text.

把二进制转成 SNBT 文本。

## 370db06c71

>  One NBT value. The variants correspond one to one with the Bedrock tag types.
>
>  The variant set matches the earlier generation, so existing code compiles unchanged.

一个 NBT 值。各个变体和基岩版的标签类型一一对应。

变体的集合和早先的一代相同，所以现有的代码不用改就能编译。

## cf631a1f66

>  An empty compound tag.

一个空的复合标签。

## 514db21245

>  Builds a compound tag straight from key-value pairs.
>
>  ```ignore
>  let v = NbtValue::obj([
>      ("x", 10.into()),
>      ("name", "stone".into()),
>  ]);
>  ```

直接用键值对构造一个复合标签。

```rust
let v = NbtValue::obj([
    ("x", 10.into()),
    ("name", "stone".into()),
]);
```

## 6debcd186c

>  Builds a list from a sequence of values.

用一串值构造一个列表。

## 24cb48ad02

>  A double list shaped `[x, y, z]`, which is how a coordinate appears in a payload.

形如 `[x, y, z]` 的 double 列表，载荷里的坐标就是这个形状。

## ba9cf048aa

>  Parses a piece of SNBT.

解析一段 SNBT。

## 9d2445b8c9

>  Serializes to SNBT. A float always carries a `d` or `f` suffix, a byte a `b` and a long
>  an `L`. Without the suffix the other side reads `100.0` as an Int, which is the source
>  of a double being written and then failing to read back.

序列化成 SNBT。浮点数总是带 `d` 或 `f` 后缀，byte 带 `b`，long 带 `L`。没有后缀的话，另一边会把 `100.0` 读成 Int，写进去一个 double、读回来却失败，根源就在这里。

## a3e98380d2

>  The type name of this value, used only in error messages.

这个值的类型名，只用在错误消息里。

## 0abd22996f

>  Any integer type into an i64. A float does not convert, since `3.7` quietly becoming
>  `3` breeds bugs.

把任意整数类型转成 i64。浮点数不转换，因为 `3.7` 悄悄变成 `3` 会带来 bug。

## 0c383b9ce9

>  As above, narrowed to an i32. Out of range returns `None` rather than truncating.

同上，收窄成 i32。超出范围时返回 `None`，不截断。

## a1e7e51144

>  Any numeric type into an f64, integers included.

把任意数值类型转成 f64，包括整数。

## 3ffde63c31

>  A boolean. In SNBT a boolean is `1b` or `0b`, so any integer type is accepted and
>  non-zero is true.

布尔值。SNBT 里的布尔值是 `1b` 或 `0b`，所以接受任意整数类型，非零即为真。

## 1e2343dd6d

>  An `[x, y, z]` list into a triple. This is the shape the host uses for coordinates, as
>  in `pos` and `block` of `edit_trace_ray` and `min` and `max` of `actor_get_aabb`.

把 `[x, y, z]` 列表转成三元组。宿主表示坐标就用这个形状，比如 `edit_trace_ray` 的 `pos` 和 `block`，`actor_get_aabb` 的 `min` 和 `max`。

## 0ccd36881b

>  As above but as integers, for a block coordinate.

同上，但用整数，用于方块坐标。

## 255691a03b

>  Reads a direct child key.

读取一个直接的子键。

## 8a3bf64089

>  Reads item i of a list.

读取列表的第 i 项。

## fb2babbab4

>  Writes a key. It returns false when this is not a compound tag and never quietly turns
>  it into one.

写入一个键。这个值不是复合标签时返回 false，永远不会悄悄把它变成复合标签。

## 5e5a40103c

>  A dotted path with array indices, such as `"a.b[2].c"`.
>
>  An earlier generation supported plain dots only, so reading the y of the min of an aabb
>  took three lines.

用点分隔、带数组下标的路径，例如 `"a.b[2].c"`。

早先的一代只支持纯粹的点，读一个碰撞箱 min 的 y 要写三行。

## f1b7c99655

>  Reads by path. A missing value is a `Missing` carrying the full path name.

按路径读取。缺少值时得到 `Missing`，里面带着完整的路径名。

## b02bb810c7

>  Reads an integer. A missing key is a `Missing` and a mismatched type a `WrongType`. It
>  never gives back a 0.

读取一个整数。缺少键得到 `Missing`，类型不符得到 `WrongType`。它从不拿 0 来充数。

## 03188a08eb

>  Tries several paths in order and returns the first string that exists.
>
>  The reason for this method is concrete: the same concept has different key names across
>  events, `player`, `_player.name` or `name`, and business code was already writing this
>  loop.

按顺序尝试几条路径，返回第一个存在的字符串。

有这个方法的原因很具体：同一个概念在不同事件里的键名不一样，`player`、`_player.name` 或 `name`，业务代码已经在写这个循环了。

## 4008f09c65

>  Converts into a `serde_json::Value`. The type suffix information is lost, since JSON
>  does not distinguish byte from long, so this route suits saving, logging and sending to
>  the web, and not converting back to feed the host.

转成 `serde_json::Value`。类型后缀的信息会丢失，因为 JSON 不区分 byte 和 long，所以这条路适合保存、记日志和发给网页，不适合再转回来交给宿主。

## 24c7742db5

>  Converts from a `serde_json::Value`. An integer becomes a `Long` and a float a
>  `Double`, which is the lossless choice. A narrower type has to be constructed by hand.

从 `serde_json::Value` 转换。整数变成 `Long`，浮点数变成 `Double`，这是不丢信息的选择。更窄的类型要手工构造。

## e9b3dfff55

>  Encodes this tree into binary NBT.

把这棵树编码成二进制 NBT。

## 990af67038

>  Decodes a tree from binary NBT.

从二进制 NBT 解码出一棵树。

## e05e111d8a

>  The two encodings of binary NBT.

二进制 NBT 的两种编码。

## 96db161ad8

>  Why a read failed. A missing key and a type mismatch are two different things and must
>  not both collapse into `None`.

读取失败的原因。缺少键和类型不符是两回事，不能都合成 `None`。

## ebc905bc31

>  The result of a read.

一次读取的结果。

## 11b7d94e60

>  A parse failure. `at` is the byte offset of the fault.

解析失败。`at` 是出错处的字节偏移。

## 70dd80fce6

>  Simulated players: a real `ServerPlayer` the server builds.
>
>  Once built, every by-name player API applies to it: teleporting, health, inventory and
>  kicking. Only the family of make-it-do-something actions unique to it lives here.
>
>  # Actions go through one multiplexed slot
>
>  `sim_do` takes a verb plus argument SNBT, and the verb table grows on the host side
>  without taking a new table slot. The cost is that a misspelled verb is reported only at
>  runtime, so each verb is wrapped here as a named method.
>
>  # A real player is never driven
>
>  The host gates on `isSimulatedPlayer()` and a call against a real player fails outright.

模拟玩家：由服务器构造的一个真正的 `ServerPlayer`。

构造好以后，所有按名字操作玩家的接口都适用于它：传送、生命值、物品栏、踢出。只有它特有的「让它做点什么」这一族动作放在这里。

**动作经过一个复用的槽位**

`sim_do` 接收一个动作名加上参数 SNBT，动作表在宿主那边扩展，不占新的表槽位。代价是动作名拼错只能在运行时发现，所以这里把每个动作包成了一个具名方法。

**永远不会操纵真实玩家**

宿主以 `isSimulatedPlayer()` 把关，对真实玩家的调用会直接失败。

## 37504d9de3

>  Builds a simulated player. It fails when the name is taken by a real player.

构造一个模拟玩家。名字被一个真实玩家占用时失败。

## 7bdc2aeec3

>  The simulated players currently alive, and `Err` for a host without the slot, which an
>  empty list would hide.
>
>  A simulated player survives a restart with the save while an in-memory handle does not,
>  so this is how they are found again after a restart.

当前活着的模拟玩家；宿主没有这个槽位时返回 `Err`，空列表会把这一点掩盖掉。

模拟玩家会随存档在重启后保留，内存里的句柄不会，所以重启后靠这个找回它们。

## 26126a90e6

> use try_list: this answers an empty list when the host has no sim_list slot

请用 `try_list`：宿主没有 `sim_list` 槽位时，这个函数回答的是空列表

## 71f08172cd

>  The simulated players currently alive, and an empty list when the host cannot list them.

当前活着的模拟玩家；宿主列不出来时返回空列表。

## aa3706d2cc

>  One simulated player.

一个模拟玩家。

## 4cb5b910bf

>  Attaches by name to a simulated player that already exists. It does not check whether
>  it really exists; [`SimPlayer::try_is_simulated`] does that.

按名字接上一个已经存在的模拟玩家。它不检查是否真的存在；要检查请用 [`SimPlayer::try_is_simulated`]。

## 0db14386f1

>  Uses it as an ordinary player, giving the full player API.

把它当作普通玩家使用，获得完整的玩家接口。

## c1ff8669b5

>  Whether this name currently points at a live simulated player, and `Err` when the
>  host cannot tell.

这个名字当前是否指向一个活着的模拟玩家；宿主分辨不出来时返回 `Err`。

## 7dabdad4ae

> use try_is_simulated: this answers false when the host cannot tell

请用 `try_is_simulated`：宿主分辨不出来时，这个函数回答 false

## 6290b8a899

>  Whether this name currently points at a live simulated player, and `false` when the
>  host cannot tell.

这个名字当前是否指向一个活着的模拟玩家；宿主分辨不出来时返回 `false`。

## e91a1c3c06

>  Runs one verb. The argument is SNBT, and `"{}"` is passed when there is none.
>
>  The verb table is on the host side and the named methods here are a facade over it. A
>  verb the host does not recognize, an argument of the wrong shape, and a target that is
>  not a simulated player are all an `Err`.

执行一个动作。参数是 SNBT，没有参数时传 `"{}"`。

动作表在宿主那边，这里的具名方法是它的门面。宿主不认识的动作、形状不对的参数，以及目标不是模拟玩家，都会返回 `Err`。

## 39a839fd8b

>  Walks straight there. `face_target` decides whether it faces the target while walking.

径直走过去。`face_target` 决定走的时候是否面向目标。

## 476e1d1d13

>  Paths there. Unlike [`SimPlayer::move_to`] it goes around obstacles.

寻路过去。和 [`SimPlayer::move_to`] 不同，它会绕开障碍物。

## 56a6e0bbb4

>  Mines one block. `face` is the face, defaulting to 1, meaning up.

挖掉一个方块。`face` 是面，默认为 1，也就是上面。

## 46451871be

>  Mines the one in the line of sight. `hand` is the reach, in blocks.

挖掉视线上的那个方块。`hand` 是触及距离，单位是方块。

## 89c5bff84a

>  Custom dimensions: the facade of the optional `pier-dimensions` capability package.
>
>  When it is not built into the host the whole family of slots is NULL,
>  [`is_available`] returns false and every other call returns an `Err` saying the host
>  does not provide it. That is rule 3 of contract §1 at runtime: the optional package is
>  absent, the layout is unchanged, and the slots are empty.
>
>  Registration is idempotent: [`add_dimension`] and [`add_dimension_pack`] return the
>  same persisted id for the same name on the next startup, so a mod registers at startup
>  rather than probing with [`dimension_id`] first, which misses on the first startup.
>  A pack terrain is a directory with a config and a binary built by `tools/pier-pack`;
>  [`pack_inspect`] tells what it asks for and [`add_dimension_pack`] mounts it, and the
>  host stores the file hash and every bound value with the dimension.

自定义维度：可选能力包 `pier-dimensions` 的门面。

宿主没有编入它时，整组槽位都是 NULL，[`is_available`] 返回 false，其他每个调用都返回一个说明宿主没有提供的 `Err`。这就是契约 §1 第 3 条在运行时的样子：可选的包不在，布局不变，槽位为空。

注册是幂等的：[`add_dimension`] 和 [`add_dimension_pack`] 在下次启动时，对同一个名字返回同一个持久化的 id，所以模组在启动时直接注册就行，不要先用 [`dimension_id`] 探测，那样在第一次启动时会落空。地形包是一个目录，里面有一个配置文件和一个由 `tools/pier-pack` 构建的二进制文件；[`pack_inspect`] 说明它要求什么，[`add_dimension_pack`] 挂载它，宿主把文件哈希和每一个绑定的值都和维度一起保存。

## b1a5a27c1b

>  Whether this host was built with the custom dimension capability.

这个宿主是否编入了自定义维度这项能力。

## 7b4c0fca37

>  Looks up a dimension id by name.
>
>  It gives an id only for a name really registered and returns `None` otherwise, rather
>  than the undefined dimension whose value changes at runtime while looking like a valid
>  id.
>
>  It is rarely needed; see the note on registering unconditionally in the module
>  documentation.

按名字查维度 id。

只对真正注册过的名字给出 id，否则返回 `None`，不会给出那个数值在运行时会变、看起来却像有效 id 的未定义维度。

很少需要用到；见模块文档里关于无条件注册的说明。

## 75f2344e29

>  Every registered custom dimension.
>
>  A world manager adopting an existing save has to ask this first: the dimensions a
>  previous plugin created are alive in the save and players can teleport into them while
>  the manager's table has no row for them. The consequence is not a few missing rows but
>  those dimensions being governed by no rule, and a newly created world possibly being
>  assigned a dimension id that collides with theirs.

所有已注册的自定义维度。

接管一个现有存档的世界管理器必须先问这个：以前的插件创建的维度活在存档里，玩家能传送进去，管理器的表里却没有它们。后果比少几行严重：这些维度不受任何规则约束，新建的世界还可能分到一个和它们冲突的维度 id。

## 226c01f4a1

>  Sets one per-dimension rule.

设置一条按维度生效的规则。

## a548e4006a

>  Reads one rule. A dimension with no explicit registration for that rule gives
>  `Ok(None)`, meaning it follows vanilla behavior, which is different from being
>  registered with the value false. A host older than this SDK that does not know `rule`
>  also answers `Ok(None)`; the ABI gives no way to tell the two apart.

读取一条规则。这个维度对这条规则没有显式登记时返回 `Ok(None)`，表示它沿用原版行为，这和登记为 false 不一样。比这个 SDK 旧、不认识 `rule` 的宿主也回答 `Ok(None)`；ABI 没有办法区分这两种情况。

## 09ad3a8b22

>  Clears every rule of a dimension, for when the world was deleted.

清除一个维度的所有规则，用于世界被删除的时候。

## 47a2a93b9f

>  Replaces the merge marks of a dimension as a whole.
>
>  As a whole and not incrementally: an increment requires both sides to agree at all
>  times on the same current state, while unlinking clears the neighbor before storing
>  itself, and a failure in between makes the two views diverge with no way back. A whole
>  push pulls both sides back into agreement every time.
>
>  The grid comes from the template pack's CONF section at [`add_dimension_pack`]: a push
>  to a dimension without one is dropped with a warning. The geometry the mod side must
>  match: with `period = cell + gap`, a column `(x, z)` is inside a cell when
>  `mod(x, period) < cell && mod(z, period) < cell`.

整体替换一个维度的合并标记。

整体替换，不做增量：增量要求两边时刻对同一个当前状态达成一致，而解除链接会先清掉邻居再保存自己，两步之间出错，两边的视图就分开了，而且没有办法恢复。整体推送让每一次都把两边拉回一致。

网格来自 [`add_dimension_pack`] 时模板包的 CONF 段：对没有网格的维度推送会被丢弃，并记一条警告。模组一侧必须对上的几何：令 `period = cell + gap`，一列 `(x, z)` 在单元内，当且仅当 `mod(x, period) < cell && mod(z, period) < cell`。

## f1f759f9b3

>  Adds a custom dimension with a native terrain; see `md_add_dimension` in `abi.h`.
>
>  The spec is opaque to this SDK: the shape is owned by the host and the RSW world
>  manager (`rsw_world_spec::Generator::to_spec_snbt`) writes it. This function only
>  carries the string across. A pack terrain is refused here; see [`add_dimension_pack`].

添加一个使用原生地形的自定义维度；见 `abi.h` 里的 `md_add_dimension`。

描述对这个 SDK 是不透明的：形状归宿主所有，由 RSW 世界管理器（`rsw_world_spec::Generator::to_spec_snbt`）编写。这个函数只负责把字符串传过去。地形包类的地形在这里会被拒绝；见 [`add_dimension_pack`]。

## eea837858b

>  Adds a custom dimension whose terrain is a pack; see `md_add_dimension_pack` in
>  `abi.h`.
>
>  `config_path` names the pack config relative to the server root with forward slashes;
>  `spec_snbt` is a spec whose terrain has `kind:"template"` or `"volume"` plus `params`
>  and `roles`. The host verifies the pack, binds the values and stores everything with
>  the dimension. A negative return is a `PIER_PACK_*` code and becomes a [`PackError`]
>  with no problem lines; the reasons are in the host log, and [`pack_inspect`] on the
>  same path returns them as JSON.

添加一个地形来自地形包的自定义维度；见 `abi.h` 里的 `md_add_dimension_pack`。

`config_path` 指定地形包的配置文件，相对于服务器根目录，用正斜杠；`spec_snbt` 是一份描述，地形部分带有 `kind:"template"` 或 `"volume"`，以及 `params` 和 `roles`。宿主校验地形包、绑定参数值，并把一切和维度一起保存。返回负数时是一个 `PIER_PACK_*` 代码，会变成一个不带问题行的 [`PackError`]；原因写在宿主日志里，对同一个路径调用 [`pack_inspect`] 会以 JSON 返回它们。

## b7db9bb1e8

>  What a pack asks for, as the JSON document `md_pack_inspect` describes, without
>  registering anything. A refusal carries the host's problem lines.

一个地形包要求什么，以 `md_pack_inspect` 描述的 JSON 文档给出，不注册任何东西。被拒绝时带有宿主给出的问题行。

## edc5a24f6d

>  Retires a custom dimension; see `md_retire_dimension` in `abi.h`.
>
>  The host drops the name from `dimension_config.json`, from its own tables and from the
>  dimension factory, so it is not registered again on the next boot. Nothing in the
>  running engine is undone: the dimension built for this session stays and a player
>  inside it is not moved.
>
>  The chunks stay in the save and the id is not handed out again. Registering the same
>  name afterwards is a new dimension with a new id, so the old terrain is orphaned
>  rather than inherited.
>
>  `Ok(false)` when the host had no dimension of that name, which is also the answer to a
>  second call.

让一个自定义维度退役；见 `abi.h` 里的 `md_retire_dimension`。

宿主把这个名字从 `dimension_config.json`、自己的表和维度工厂里去掉，下次启动时不会再注册它。正在运行的引擎里什么都不会撤销：这次会话里构造的维度仍然在，里面的玩家也不会被移走。

区块留在存档里，id 也不会再分出去。之后用同一个名字注册的是一个新维度，有新的 id，旧的地形不会被它继承，只是留在磁盘上。

宿主没有这个名字的维度时返回 `Ok(false)`，第二次调用的回答也是这个。

## 684b96e984

>  Registers a dimension whose terrain this mod fills; see `md_add_dimension_generated` in
>  `abi.h`.
>
>  `spec_snbt` carries only seed, height and sky. A terrain section is refused, since the
>  host does not read one.
>
>  `terrain` is leaked into a `'static`: the host uses it on chunk threads for as long as
>  the dimension lives, and that lifetime is the host's, so this side has no moment at
>  which reclaiming it is safe. A refused registration leaks it too, because a generator
>  built during the attempt may still hold it. Register once per dimension.

注册一个地形由这个模组填充的维度；见 `abi.h` 里的 `md_add_dimension_generated`。

`spec_snbt` 只带种子、高度和天空。带 terrain 段会被拒绝，因为宿主不读它。

`terrain` 会被泄漏成 `'static`：维度存在多久，宿主就在区块线程上用它多久，而这个生命期归宿主管，这一侧找不到一个回收它也安全的时刻。注册被拒绝时也会泄漏，因为尝试期间构造的生成器可能还拿着它。每个维度只注册一次。

## 62e95256ab

>  Gives a dimension the cell geometry its confinement rules use; see
>  `md_set_dimension_cells`.
>
>  `cell` is the edge of a cell and `gap` the space between cells, both in blocks. A `cell`
>  of 0 removes the geometry, after which the two `PIER_DIMRULE_*_CROSS_CELL` rules have
>  nothing to answer from.

给一个维度设置约束规则所用的单元几何；见 `md_set_dimension_cells`。

`cell` 是单元的边长，`gap` 是单元之间的间隔，单位都是方块。`cell` 为 0 会移除几何，之后两条 `PIER_DIMRULE_*_CROSS_CELL` 规则就没有依据可以回答了。

## f79fd7c36d

>  The vanilla generator of a `terrain:{kind:"native"}` spec.
>
>  The values are the engine's `GeneratorType`, which starts at 1 and not 0: numbering from
>  0 would make superflat generate a nether. The spec itself names the generator with the
>  lower-case string of [`GeneratorType::spec_name`]; the numbers survive because
>  `md_list_dimensions` and old saves both carry them.

`terrain:{kind:"native"}` 描述里的原版生成器。

取值是引擎的 `GeneratorType`，从 1 开始，不从 0 开始：从 0 编号的话，超平坦会生成一个下界。描述本身用 [`GeneratorType::spec_name`] 给出的小写字符串来指定生成器；数字之所以保留，是因为 `md_list_dimensions` 和旧存档里都带着它们。

## 4c394d42c3

>  What `terrain.generator` and `sky.client` are spelled as in a spec.

在描述里，`terrain.generator` 和 `sky.client` 的写法。

## ca23c2e880

>  What the engine itself calls this generator.
>
>  Not the same string as [`GeneratorType::spec_name`]: this one appears in generation
>  parameters and in old saves. Use it when assembling something for the engine, not
>  `{:?}`.

引擎自己对这个生成器的叫法。

和 [`GeneratorType::spec_name`] 是两个不同的字符串：这个出现在生成参数和旧存档里。给引擎拼装东西时用它，不要用 `{:?}`。

## c01b96c8c4

>  The merge marks of one plot.

一块地皮的合并标记。

## d8abfa4828

>  Assembles a mask from the four directions in the order north, east, south, west.
>
>  The order is the bit order, with `NORTH` as bit 0. Writing `1 | 4` by hand makes a
>  reader look the table up in reverse, and getting it backwards shows up as plots merging
>  in the wrong direction.

按北、东、南、西的顺序，用四个方向组装一个掩码。

这个顺序就是位的顺序，`NORTH` 是第 0 位。手写 `1 | 4` 会让读的人倒过来查表，弄反了的表现是地皮朝错误的方向合并。

## 551fbd8f5c

>  Per-dimension rules. The values correspond to `PIER_DIMRULE_*`.
>
>  Why not a game rule: a Bedrock game rule applies to the whole server, so turning
>  `doMobSpawning` off for a creative plot world turns it off for the survival world too.
>  These flags are checked at the real call sites, `Spawner::spawnMob`, `Level::explode`
>  and others, so they really are per dimension.
>
>  A dimension that was never registered is entirely unaffected: the hook falls straight
>  through to the vanilla implementation and a caller need not allow vanilla dimensions
>  explicitly.

按维度生效的规则。取值对应 `PIER_DIMRULE_*`。

为什么不用游戏规则：基岩版的游戏规则对整个服务器生效，所以为了创造模式的地皮世界关掉 `doMobSpawning`，生存世界也会一起关掉。这些标志在真正的调用处检查，比如 `Spawner::spawnMob`、`Level::explode`，所以真正是按维度生效的。

从来没有登记过规则的维度完全不受影响：钩子直接落到原版实现上，调用方不需要专门放行原版维度。

## 56f0bc1f23

>  Why the host refused a pack; the values are `PIER_PACK_*`.

宿主拒绝一个地形包的原因；取值是 `PIER_PACK_*`。

## 8461f9fdf5

>  A dimension whose terrain the mod fills itself.
>
>  An implementation is handed to the chunk worker threads, so it is `Send + Sync`, and it
>  must not change once registered. That is the contract of `PierGenerateChunkFn` in
>  `abi.h`: the host calls it concurrently, the same coordinates must always give the same
>  answer, and the callback must not call any other slot of the host.
>
>  A generator that reads only what it mounted satisfies all three; one that consults the
>  current state of the world satisfies none.

地形由模组自己填充的维度。

实现会被交给区块工作线程，所以它是 `Send + Sync`，并且注册以后不能再改变。这就是 `abi.h` 里 `PierGenerateChunkFn` 的约定：宿主并发地调用它，同样的坐标必须永远给出同样的结果，回调不能调用宿主的任何其他槽位。

只读取自己挂载的数据的生成器，三条都满足；查询世界当前状态的生成器，一条都不满足。

## b7d6ebe29e

>  Fills one chunk. `materials` has `256 * height` entries, indexed
>  `(x * 16 + z) * height + y` with y counted from `min_y`; `biomes` has 256, one per
>  column. Both hold indices into the two palettes given at registration, and material
>  0 is air.
>
>  Returning false means the chunk could not be filled; the host writes air and logs it.

填充一个区块。`materials` 有 `256 * height` 项，下标是 `(x * 16 + z) * height + y`，y 从 `min_y` 开始数；`biomes` 有 256 项，每列一项。两者存的都是注册时给出的两个调色板里的索引，材料 0 是空气。

返回 false 表示这个区块填不了；宿主会写入空气，并记录日志。

## b03874d565

>  One custom dimension that has been registered.

一个已经注册的自定义维度。

## 77d8d0d81d

>  A refusal of [`add_dimension_pack`] or [`pack_inspect`], with the host's reasons when
>  the call produced any.

[`add_dimension_pack`] 或 [`pack_inspect`] 的一次拒绝；调用产生了原因时，附带宿主给出的原因。

## 58a5e9d555

>  What is handed to the host at registration, with its two palettes.

注册时交给宿主的东西，连同它的两个调色板。

## 90eb408ca8

>  Client-only capabilities.
>
>  # On a server host this whole family is empty slots
>
>  Not a compile error but a runtime `Err`. Contract §2.1: the layout is identical on every
>  target and an absent capability is a NULL slot, so the same mod source compiles for both
>  targets and loading onto the wrong one is refused explicitly by the host during the
>  handshake, from `mod_flags`.
>
>  Decide with [`is_available`] and not with a `cfg`.
>
>  # A callback runs on the client thread
>
>  Not the server thread. A hotkey callback must not touch server state.

只在客户端可用的能力。

**在服务器宿主上，这一整组都是空槽位**

在服务器宿主上调用，得到的是运行时的 `Err`，编译不会报错。契约 §2.1：所有目标上的布局都一样，缺少的能力就是 NULL 槽位，所以同一份模组源码能为两个目标编译；加载到错误的目标上时，宿主会在握手时根据 `mod_flags` 明确拒绝。

用 [`is_available`] 判断，不要用 `cfg`。

**回调在客户端线程上运行**

不在服务器线程上。热键回调不能碰服务器的状态。

## aed6edb20d

>  Whether this host was built for the client target, meaning whether the `client_*` slots
>  are filled.

这个宿主是不是为客户端目标构建的，也就是 `client_*` 槽位有没有填上。

## f052e1f19b

>  Whether the client is currently in a world.

客户端当前是否在一个世界里。

## 600e78ceed

>  The name of the local player. Not being in a world is an `Err`.

本地玩家的名字。不在世界里时返回 `Err`。

## ea8ccfe391

>  The current screen name, such as `"hud_screen"` or `"pause_screen"`.

当前界面的名字，比如 `"hud_screen"` 或 `"pause_screen"`。

## fe8045728d

>  Registers a hotkey.
>
>  `key_codes` is the default binding and `allow_remap` decides whether the player may
>  change it in the settings.
>  The callback receives (action, focus impact) and both run on the client thread.

注册一个热键。

`key_codes` 是默认的绑定，`allow_remap` 决定玩家能不能在设置里改它。回调收到（动作，焦点影响），两者都在客户端线程上运行。

## f8f624762d

>  One registered hotkey. Dropping it deregisters.

一个已注册的热键。丢弃它就会注销。

## e15fa5bcef

>  The key codes currently bound. They differ from the defaults once the player has
>  rebound them.

当前绑定的按键码。玩家重新绑定以后，就和默认值不同了。

## feac7e0369

>  Keeps it alive until the mod unloads, when the host clears it.

让它一直存活到模组卸载，那时由宿主清掉。

## d8e9bc6701

>  A key action.

一个按键动作。

## 740a2e2a1c

>  The engine's own registries: every block type, every item and every entity the running game
>  has, with the facts a rule needs to pick from them.
>
>  A mod that hands out "a random block" or "a random rare item" should not keep a list of
>  names of its own: the game has more than any such list, and the list goes stale with every
>  version. It reads these instead and decides by rules (solid, not technical, of this rarity).
>  A host older than the `registry_list` slot answers with an error, never with an empty list
>  that would look like a game without content.

引擎自己的注册表：正在运行的游戏里的每一种方块、每一种物品和每一种实体，连同按规则从中挑选时需要的信息。

发放「一个随机方块」或者「一个随机稀有物品」的模组，不应该自己维护一份名字列表：游戏里的东西比任何这样的列表都多，而且每个版本都会让列表过时。改为读取这些注册表，按规则（实心的、不是技术性方块、某个稀有度）来挑。比 `registry_list` 槽位旧的宿主会返回错误，永远不会返回一个看起来像是游戏没有任何内容的空列表。

## 9ed0b52882

>  Every block type.

所有方块类型。

## 841432f576

>  Every item, each once under its own full name.

所有物品，每种以它完整的名字出现一次。

## c0c9cc04c6

>  Every entity the level knows.

关卡知道的所有实体。

## 0d2fb9f77d

>  One entity the level knows.

关卡知道的一种实体。

## e438dc8b60

>  A land animal: the animal bit without the water-animal one.

陆地动物：有动物位，没有水生动物位。

## 1df5b06d4b

>  One block type, read from its default state.

一种方块，按它的默认状态读取。

## 9e3b8d2f69

>  One item.

一种物品。

## 777f446384

>  Reachable by commands only: barriers, command blocks, structure blocks and the like.

只能通过命令获得：屏障、命令方块、结构方块之类。

## 4c221fc88f

>  The value types every domain shares.
>
>  Only things without behavior belong here: coordinates, enums and bit flags. They touch
>  no `PierApi`, so domain modules can share them without depending on one another.
>
>  # Enums carry `from_i32` and return an `Option`
>
>  The host may be newer than the mod and report a value this side does not recognize
>  (contract §2.2). A `None` from `from_i32` means the host reported an unrecognized
>  value and stays apart from the value being 0 (§5.2). The other direction uses `as_i32`,
>  where no unknown value can arise.

各个领域共用的值类型。

只有不带行为的东西放在这里：坐标、枚举和位标志。它们不碰 `PierApi`，所以各个领域的模块可以共用它们，而不用互相依赖。

**枚举带有 `from_i32`，并返回 `Option`**

宿主可能比模组新，报告一个这一侧不认识的值（契约 §2.2）。`from_i32` 返回 `None` 表示宿主报告了一个不认识的值，这和值为 0 是两回事（§5.2）。反方向用 `as_i32`，那里不会出现未知的值。

## 5731cdd39e

>  A closed box, with both corners included.

一个闭区间的长方体，两个角都包含在内。

## 60589eb611

>  Builds a box from any two corners, taking the smaller and larger per axis, so a caller
>  need not sort them first.

用任意两个角构造一个长方体，每个轴分别取较小值和较大值，调用方不需要先排序。

## c5484af305

>  The cell count per axis. The interval is closed, so it is max - min + 1.

每个轴上的格子数。区间是闭的，所以是 max - min + 1。

## 969b8edfe8

>  The total cell count. A `u64` is used because a selection spanning a dimension easily
>  overflows a `u32`.

总格子数。用 `u64`，因为一个跨越整个维度的选区很容易超出 `u32`。

## c1a2f9019a

>  The result of one ray trace, from `actor_trace_ray` or `edit_trace_ray`.

一次射线检测的结果，来自 `actor_trace_ray` 或 `edit_trace_ray`。

## 0e707dc06d

>  The game mode. The values are part of the ABI, through `player_set_gamemode`.
>
>  Note that `Spectator` is 6 and not 3: the values in between mean other things in the
>  engine, and guessing one from the order sets the player to a different mode without an
>  error.

游戏模式。取值属于 ABI，经 `player_set_gamemode` 传递。

注意 `Spectator` 是 6，不是 3：中间的值在引擎里有别的含义，按顺序猜一个值，会在没有任何报错的情况下把玩家设成另一种模式。

## 53b8479205

>  The player permission level, through `PIER_PACT_SET_PERMISSION_LEVEL` and
>  `PIER_PPROP_PERMISSION_LEVEL`.

玩家的权限等级，经 `PIER_PACT_SET_PERMISSION_LEVEL` 和 `PIER_PPROP_PERMISSION_LEVEL` 传递。

## 20fe47c788

>  The weather. The three states of `set_weather`.

天气。`set_weather` 的三种状态。

## 6f8c3bf251

>  The difficulty.

难度。

## 3de12cc17a

>  The `SetTitlePacketPayload::TitleType` of `player_send_title`.
>
>  The three TextObject variants at 6 through 8 need a `ResolvedTextObject`, which the
>  host refuses explicitly, so they are not offered here at all.

`player_send_title` 用的 `SetTitlePacketPayload::TitleType`。

6 到 8 的三种 TextObject 变体需要 `ResolvedTextObject`，宿主会明确拒绝，所以这里根本不提供它们。

## ff74c5be02

>  The host does not read the `text` argument of Clear, Reset and Times.

Clear、Reset 和 Times 这三种，宿主不读 `text` 参数。

## bb63bfb2c1

>  An equipment slot. The numbering `actor_get_equipped_item` and `player_get_equipment`
>  use.

装备槽位。`actor_get_equipped_item` 和 `player_get_equipment` 用的编号。

## 40d5f1c6b4

>  A player ability bit. The index of `PIER_PACT_SET_ABILITY` and
>  `PIER_PACT_CAN_USE_ABILITY`.
>
>  The three carrying `Speed` are floating-point abilities and the rest are boolean.
>  Passing the wrong type raises no error and only has the value interpreted differently,
>  which is why [`Ability::is_float`] exists and
>  [`crate::player::Player::set_ability`] checks against it.

玩家的一个能力位。`PIER_PACT_SET_ABILITY` 和 `PIER_PACT_CAN_USE_ABILITY` 的下标。

名字里带 `Speed` 的三个是浮点类的能力，其余是布尔类的。传错类型不会报错，只会让值被按另一种方式解读，所以才有 [`Ability::is_float`]，[`crate::player::Player::set_ability`] 也会拿它来检查。

## dd368b7194

>  The `TextPacketType` of `player_send_message_typed`.
>
>  The host falls back to `Raw` on an out-of-range value, so no fallback branch is needed
>  here. The variants carrying an author or parameters, Chat, Whisper and Translate, take
>  a single body string on the ABI, and the author field reaches the client empty.

`player_send_message_typed` 用的 `TextPacketType`。

超出范围的值，宿主会按 `Raw` 处理，所以这里不需要兜底的分支。带作者或参数的变体，即 Chat、Whisper 和 Translate，在 ABI 上只接收一个消息体字符串，作者字段到达客户端时是空的。

## 6a01285e54

>  The three title durations, in ticks.
>
>  All three are given together or not at all: the host refuses a half-specified
>  combination rather than guessing the rest for the caller, since half a set of durations
>  has no sensible default.

标题的三个时长，单位是刻。

三个要么一起给，要么都不给：只指定一部分的组合会被宿主拒绝，它不替调用方猜剩下的值，因为一半的时长没有合理的默认值。

## c7358269de

>  Tells the engine which follow-up work a block write needs.
>
>  `NONE` is fastest and leaves the client unaware that the block changed, so a bulk fill
>  has to resynchronize afterwards, otherwise the player keeps seeing the old world until
>  that chunk is resent.

告诉引擎一次方块写入之后需要做哪些后续工作。

`NONE` 最快，但客户端不知道方块变了，所以批量填充之后必须重新同步，否则玩家会一直看到旧的世界，直到那个区块被重新发送。

## 448296e3ce

>  The value an ability bit can take. There is a boolean family and a floating-point
>  family, and [`Ability::is_float`] decides which applies.

能力位可以取的值。有布尔和浮点两类，由 [`Ability::is_float`] 决定用哪一类。

## 91efa97131

>  A block coordinate.

一个方块坐标。

## e7d0d416c1

>  An actor or exact coordinate.

一个实体坐标或精确坐标。

## e86b472e56

>  The local time, from `PIER_SYS_LOCAL_TIME`.

本地时间，取自 `PIER_SYS_LOCAL_TIME`。

# 迁移台账 —— 旧仓库 → Pier v2 重写

「功能只比旧版多、不比旧版少」的判据就是这张表：交付前逐行清点，
每一行要么 ✔（新仓库里有对应实现），要么 ✂（写明为什么按设计删除、去了哪）。
状态 ⬜ 的行 = 本轮还没迁到的：**它们的功能此刻在新仓库里缺席**。

| 旧文件 | 新位置 | 状态 |
|---|---|---|
| `CONTRACT.md` | `CONTRACT.md` | ✔ v2 重写（真实依赖图 + SPI 机制 + 空槽能力语义） |
| `—` | `COMMENTS.md` | ✔ 新增（契约 §七 的展开件：注释三层预算与禁写清单，机检 `comment-style` 守机械部分） |
| `LICENSE` | `LICENSE` | ✔ 原样保留（三份 Cargo.toml 都声明 Apache-2.0，文件却没跟过来 —— 台账里也漏了这一行，是 `ledger-covers-tree` 机检逮到的） |
| `Cargo.toml` | `Cargo.toml` | ✔ 重写（写明两条构建线之间只有契约依赖、无构建依赖） |
| `README.md` | `README.md` | ⬜ 待迁移（逐文件重写） |
| `docs/CHANGELOG.md` | `—` | ✂ 不随迁 —— v1 版本史随旧仓库归档；v2 从 ABI v1 重新纪年 |
| `docs/DESIGN.md` | `—` | ✂ 不随迁 —— v1 的设计叙事已被 CONTRACT v2 与 abi.h 文件头吸收；历史归档在旧仓库 |
| `docs/README.zh.md` | `docs/README.zh.md` | ⬜ 待迁移（逐文件重写） |
| `docs/RELOAD.md` | `docs/RELOAD.md` | ⬜ 待迁移（逐文件重写） |
| `docs/advanced/PORTING_NOTES.md` | `docs/advanced/PORTING_NOTES.md` | ⬜ 待迁移（逐文件重写） |
| `docs/advanced/abi.md` | `docs/advanced/abi.md` | ⬜ 待迁移（逐文件重写） |
| `docs/advanced/architecture.md` | `docs/advanced/architecture.md` | ⬜ 待迁移（逐文件重写） |
| `docs/advanced/decisions.md` | `docs/advanced/decisions.md` | ⬜ 待迁移（逐文件重写） |
| `docs/advanced/extending.md` | `docs/advanced/extending.md` | ⬜ 待迁移（逐文件重写） |
| `docs/advanced/memory-safety.md` | `docs/advanced/memory-safety.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/actor/command.md` | `docs/api/actor/command.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/actor/entity.md` | `docs/api/actor/entity.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/actor/gui.md` | `docs/api/actor/gui.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/actor/money.md` | `docs/api/actor/money.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/actor/player.md` | `docs/api/actor/player.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/actor/sim.md` | `docs/api/actor/sim.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/core/bus.md` | `docs/api/core/bus.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/core/lane.md` | `docs/api/core/lane.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/core/objects.md` | `docs/api/core/objects.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/core/overview.md` | `docs/api/core/overview.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/core/service.md` | `docs/api/core/service.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/core/system.md` | `docs/api/core/system.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/infra/data.md` | `docs/api/infra/data.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/infra/event.md` | `docs/api/infra/event.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/infra/packet.md` | `docs/api/infra/packet.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/infra/scoreboard.md` | `docs/api/infra/scoreboard.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/infra/server.md` | `docs/api/infra/server.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/rt/client.md` | `docs/api/rt/client.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/rt/log.md` | `docs/api/rt/log.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/rt/scheduler.md` | `docs/api/rt/scheduler.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/world/block.md` | `docs/api/world/block.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/world/container.md` | `docs/api/world/container.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/world/dimensions.md` | `docs/api/world/dimensions.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/world/item.md` | `docs/api/world/item.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/world/nbt.md` | `docs/api/world/nbt.md` | ⬜ 待迁移（逐文件重写） |
| `docs/api/world/world.md` | `docs/api/world/world.md` | ⬜ 待迁移（逐文件重写） |
| `docs/guide/commands.md` | `docs/guide/commands.md` | ⬜ 待迁移（逐文件重写） |
| `docs/guide/concepts.md` | `docs/guide/concepts.md` | ⬜ 待迁移（逐文件重写） |
| `docs/guide/events.md` | `docs/guide/events.md` | ⬜ 待迁移（逐文件重写） |
| `docs/guide/getting-started.md` | `docs/guide/getting-started.md` | ⬜ 待迁移（逐文件重写） |
| `docs/guide/logging-scheduling.md` | `docs/guide/logging-scheduling.md` | ⬜ 待迁移（逐文件重写） |
| `docs/guide/world.md` | `docs/guide/world.md` | ⬜ 待迁移（逐文件重写） |
| `docs/index.md` | `docs/index.md` | ⬜ 待迁移（逐文件重写） |
| `docs/package.json` | `docs/package.json` | ⬜ 待迁移（逐文件重写） |
| `examples/region-scan/Cargo.toml` | `examples/hello-pier/Cargo.toml` | ✔ 重写为 hello-pier |
| `examples/region-scan/manifest.json` | `examples/hello-pier/manifest.json` | ✔ 重写（type 与依赖名改为 "pier"；旧版依赖的 levilamina-rust-loader 已不存在、示例装不上） |
| `examples/region-scan/src/lib.rs` | `examples/hello-pier/src/lib.rs` | ⬜ region-scan 本体待迁移 —— 它依赖 types/command/player 等域模块，那些还没写；先以 hello-pier 验证契约 §十 四步 |
| `packages/pier-abi/include/sdk/abi.h` | `packages/pier-abi/include/sdk/abi.h` | ✔ 已重切（C11 化、单布局、四字段头；190 槽逐格同序，双编译验证） |
| `packages/pier-abi/xmake.lua` | `packages/pier-abi/xmake.lua` | ✔ 重写 |
| `packages/pier-api/include/pier/api/common.h` | `pier/api/bridge.h + pier/support/*` | ✔ 拆分重写（守卫/字符串/SNBT 落 support；解析助手落 bridge.h） |
| `packages/pier-api/include/pier/api/internal_api.h` | `packages/pier-host/include/pier/host/spi.h` | ✂ 退役 —— 它是上面那张转接表的声明面，随表一起消失；跨包协作改走 SPI 的六个注册面 |
| `packages/pier-api/include/pier/api/money_guard.h` | `同路径` | ✔ 重写（双查设计说明全保留） |
| `packages/pier-api/src/actors/Actors.cpp` | `同路径` | ✔ 重写（属性/动作全分支；不支持项的原因注释保留） |
| `packages/pier-api/src/actors/Forms.cpp` | `同路径` | ✔ 重写（Teardown 60；选择型控件下标/文本双份收口、滑块钳制、PIER_TRACE_FORM 全保留） |
| `packages/pier-api/src/actors/Money.cpp` | `同路径` | ✔ 重写（Teardown 100；LLMoneyEvent 冲突改名 PierMoneyEvent/PierMoneyCb，蹦床做 PierStr 转换） |
| `packages/pier-api/src/actors/MoneyGuard.cpp` | `同路径` | ✔ 重写 |
| `packages/pier-api/src/actors/Players.cpp` | `同路径` | ✔ 重写（能力位四坑/权限还原/侧边栏段位哈希与清除顺序注释全保留；md 校验改经维度桥 blockSourceOf 放行闸） |
| `packages/pier-api/src/actors/SimPlayer.cpp` | `同路径` | ✔ 重写 |
| `packages/pier-api/src/core/ApiTable.cpp` | `packages/pier-host/src/{ApiTable,Spi}.cpp` | ✂ 退役 —— 旧版是一张硬编码转接表（直呼各包函数），新架构由各能力包经 spi::SlotPack **自填**槽位（契约 §一 规则二）。表头四个标量的填写移到 pier-host/ApiTable.cpp |
| `packages/pier-api/src/core/Bus.cpp` | `同路径` | ✔ 重写（Teardown 20；锁外派发/深度防环全保留） |
| `packages/pier-api/src/core/Common.cpp` | `core/{Bridge,Enrich}.cpp + support/snbt + spi 维度桥` | ✔ 拆分重写（md 耦合改走 spi::DimensionBridge；enrich 独立 TU；addressOwnedBy 待迁 support） |
| `packages/pier-api/src/core/LogScheduler.cpp` | `core/{Log,Scheduler,GameStats}.cpp` | ✔ 拆三重写（Scheduler 注册 Teardown stage 10） |
| `packages/pier-api/src/core/Services.cpp` | `同路径` | ✔ 重写（Teardown 30；W13 与「不查 isEnabled」血泪注释全保留） |
| `packages/pier-api/src/net/Client.cpp` | `packages/pier-client/src/Client.cpp` | ✔ 迁入独立 pier-client 包（Teardown 110；alive 旗/双重反注册防护注释保留；KeyRegistry 归属改报宿主 NativeMod，按托管模组清理仍由本包负责） |
| `packages/pier-api/src/net/ClientStubs.cpp` | `—` | ✂ 整体退役：旧 ApiTable 静态引用全符号才需客户端桩；新架构服务端专属 TU 不编入客户端 → 槽位 NULL，SDK 按 struct_size/空槽纪律报「不支持」，观察行为与旧桩 false/-1/0 等价。-1/0 失败值约定已在各真实现处注明 |
| `packages/pier-api/src/net/PacketHooks.cpp` | `同路径` | ✔ 重写（服务端卫；双 detour/varint 编解码/快照派发/三原子门/W11 双打日志/永不卸钩理由全保留；Teardown 90） |
| `packages/pier-api/src/net/Packets.cpp` | `同路径` | ✔ 重写（服务端卫；ensureReadCompleted 拒尾垃圾、Times 先行单发、三时长全或无、单字节维度截断告警全保留） |
| `packages/pier-api/src/net/ScoreboardApi.cpp` | `同路径` | ✔ 重写（服务端卫；SET_DISPLAY 原生化理由保留） |
| `packages/pier-api/src/runtime/Commands.cpp` | `同路径` | ✔ 重写（update_soft_enum 静默 catch 补日志；default 分支撒谎注释订正） |
| `packages/pier-api/src/runtime/Events.cpp` | `runtime/{Events,CommandEvents}.cpp` | ✔ 重写（契约 §六 提供方路由 + 遮蔽告警；命令事件拆成 covers_registry 提供方） |
| `packages/pier-api/src/runtime/Extras.cpp` | `同路径` | ✔ 重写 |
| `packages/pier-api/src/runtime/Server.cpp` | `同路径` | ✔ 重写（客户端构建整文件留 NULL 槽） |
| `packages/pier-api/src/runtime/SysInfo.cpp` | `同路径` | ✔ 重写 |
| `packages/pier-api/src/runtime/data/KvDbApi.cpp` | `同路径` | ✔ 重写（Teardown 70；路径圈禁保留） |
| `packages/pier-api/src/runtime/data/NbtApi.cpp` | `同路径` | ✔ 重写 |
| `packages/pier-api/src/world/Containers.cpp` | `同路径` | ✔ 重写 |
| `packages/pier-api/src/world/Edit.cpp` | `world/{Edit,BlockResolve}.cpp` | ✔ 重写（服务端卫；三助手迁出） |
| `packages/pier-api/src/world/GapFill.cpp` | `同路径` | ✔ 重写（服务端卫；退役槽 -1、chunk_keys 零暂存与跨 DLL 崩溃根因、缩一格半径、conn_id 一致性契约全保留；PierStr 注释按新 {ptr,len} 语义订正） |
| `packages/pier-api/src/world/Items.cpp` | `同路径` | ✔ 重写（ADD_ENCHANT 桩原样保留并注明） |
| `packages/pier-api/src/world/World.cpp` | `world/{World,BlockResolve}.cpp` | ✔ 重写（方块解析助手抽到双目标 BlockResolve —— 修旧版客户端目标链接断裂） |
| `packages/pier-api/src/world/WorldInfo.cpp` | `同路径` | ✔ 重写（服务端卫；不强加载/不数村民的边界注释保留） |
| `packages/pier-api/xmake.lua` | `packages/pier-api/xmake.lua` | ✔ 重写（include 转私有） |
| `packages/pier-dimensions/include/pier/dimensions/base/Macros.h` | `—` | ✂ 退役：MORE_DIMENSIONS_API 是给 dll 导出用的，本包在新架构里是编进宿主的 object 包、不导出任何符号（能力经 SlotPack 装表），宏无处可用 |
| `packages/pier-dimensions/include/pier/dimensions/base/NativeDimensions.h` | `base/native_dimensions.h` | ✔ 重写（原生路径设计说明与台账用途全保留） |
| `packages/pier-dimensions/include/pier/dimensions/base/SimpleCustomDimension.h` | `base/simple_custom_dimension.h` | ✔ 重写（去掉 dllexport 宏并注明理由） |
| `packages/pier-dimensions/include/pier/dimensions/base/Utils.h` | `base/utils.h` | ✔ 重写（双份高度数据不一致的诊断全保留） |
| `packages/pier-dimensions/include/pier/dimensions/dim/ChunkTrace.h` | `packages/pier-dimensions/include/pier/dimensions/dim/chunk_trace.h` | ✔ 重写（追踪开关收成一处，PlotGenerator 共用；env 前缀 MORE_DIMENSIONS_* → PIER_*） |
| `packages/pier-dimensions/include/pier/dimensions/dim/CompleteBaseTypes.h` | `dim/complete_base_types.h` | ✔ 重写（补写「每个派生维度 .cpp 都要先包含」） |
| `packages/pier-dimensions/include/pier/dimensions/dim/CustomDimensionConfig.h` | `dim/custom_dimension_config.h` | ✔ 重写（补写「为什么跟着存档走而不是放 configs/」） |
| `packages/pier-dimensions/include/pier/dimensions/dim/CustomDimensionManager.h` | `packages/pier-dimensions/include/pier/dimensions/dim/custom_dimension_manager.h` | ✔ 重写（去 dllexport；✂ getDimensionIdFromName —— 它转调的 fromString 对自定义维度回读垃圾值） |
| `packages/pier-dimensions/include/pier/dimensions/dim/DimensionHeight.h` | `dim/dimension_height.h` | ✔ 重写（-512 底部的三次实测表与回退方案全保留） |
| `packages/pier-dimensions/include/pier/dimensions/dim/DimensionRules.h` | `packages/pier-dimensions/include/pier/dimensions/dim/dimension_rules.h` | ✔ 重写（13 条 static_assert 把编号钉死在 PierDimRule 上） |
| `packages/pier-dimensions/include/pier/dimensions/plot/PlotConfine.h` | `packages/pier-dimensions/include/pier/dimensions/plot/plot_confine.h` | ✔ 重写 |
| `packages/pier-dimensions/include/pier/dimensions/plot/PlotDimension.h` | `packages/pier-dimensions/include/pier/dimensions/plot/plot_dimension.h` | ✔ 重写（去 dllexport） |
| `packages/pier-dimensions/include/pier/dimensions/plot/PlotGenerator.h` | `packages/pier-dimensions/include/pier/dimensions/plot/plot_generator.h` | ✔ 重写（去 dllexport） |
| `packages/pier-dimensions/include/pier/dimensions/plot/PlotLayout.h` | `packages/pier-dimensions/include/pier/dimensions/plot/plot_layout.h` | ✔ 重写（修 kBedrockY 自引用；措辞去语言名） |
| `packages/pier-dimensions/src/dim/CustomDimensionConfig.cpp` | `同路径` | ✔ 重写（裸 printf 改日志并注明其危害；版本升级路径补注释） |
| `packages/pier-dimensions/src/dim/CustomDimensionManager.cpp` | `packages/pier-dimensions/src/dim/CustomDimensionManager.cpp` | ✔ 全量移植（五条事故结论逐条在位：闭包先于注册、id 从最大号+1、salvagedIds 保号、原生路径不动 Undefined、比较后再写盘） |
| `packages/pier-dimensions/src/dim/DimensionRules.cpp` | `packages/pier-dimensions/src/dim/DimensionRules.cpp` | ✔ 全量移植（订正「已启用 9 类」这条两个数都对不上的日志；setDimensionRule 补越界规则号的报错） |
| `packages/pier-dimensions/src/dim/NativeDimensions.cpp` | `同路径` | ✔ 重写（DimensionDefinitionGroup 非持久化那段 bug 史、回读校验、id 变更告警全保留；诊断环境变量改名 PIER_DIM_DEF_*，已列入迁移提示；删掉从未被调用的 canCreateDimension，其结论已并入 getOrCreateByName 三因注释） |
| `packages/pier-dimensions/src/dim/SimpleCustomDimension.cpp` | `packages/pier-dimensions/src/dim/SimpleCustomDimension.cpp` | ✔ 全量移植（天光按生成器分、default 分支出声、三符号懒解析全保留） |
| `packages/pier-dimensions/src/plot/PlotConfine.cpp` | `packages/pier-dimensions/src/plot/PlotConfine.cpp` | ✔ 全量移植（组遍历上界的不对称保留并写明理由；清速度失败的 catch 补说明） |
| `packages/pier-dimensions/src/plot/PlotDimension.cpp` | `packages/pier-dimensions/src/plot/PlotDimension.cpp` | ✔ 全量移植 |
| `packages/pier-dimensions/src/plot/PlotGenerator.cpp` | `packages/pier-dimensions/src/plot/PlotGenerator.cpp` | ✔ 全量移植（追踪开关不再各抄一份，改用 chunk_trace.h 那一份） |
| `packages/pier-dimensions/src/rt/ChunkTrace.cpp` | `packages/pier-dimensions/src/rt/ChunkTrace.cpp` | ✔ 全量移植（改用 hostLogger；两个空 catch 补说明；env 改名） |
| `packages/pier-dimensions/src/rt/MoreDimensionsBridge.cpp` | `src/rt/{Bridge,Slots}.cpp` | ✔ 拆重写：Bridge.cpp = DimensionBridge 实现（三层名字数据源 + **id 一致性安全闸** + toString 崩服血泪注释）；Slots.cpp = md_* 十一槽装填（GeneratorType 编号纠错史保留） |
| `packages/pier-dimensions/src/rt/Utils.cpp` | `src/base/Utils.cpp` | ✔ 重写（改用宿主统一 logger） |
| `packages/pier-dimensions/xmake.lua` | `同路径` | ✔ 重写（写明本包是 DimensionBridge 唯一实现方、缺席即降级） |
| `packages/pier-hooks/include/pier/hooks/decision_throttle.h` | `同路径` | ✔ 重写（四段设计理由全保留：XUID 键/位置入键/双结论缓存/有界清空） |
| `packages/pier-hooks/include/pier/hooks/hook_events.h` | `同路径` | ✔ 重写（提供方化契约：idMatches 认领、禁子串、covers_registry=false 遮蔽必警；优先级入订阅） |
| `packages/pier-hooks/src/engine/HookEvents.cpp` | `同路径` | ✔ 重写（EventProviderReg 接线；取消位改 CompoundTag::fromSnbt 统一解析，三形状漏配隐患根除；W11 回调异常就地打印） |
| `packages/pier-hooks/src/engine/Profiler.cpp` | `同路径` | ✔ 重写（五桶包含式计时、High 外层包 TickControl、warp 下样本语义注释保留） |
| `packages/pier-hooks/src/engine/TickControl.cpp` | `同路径` | ✔ 重写（冻结/步进/加减速三态原样；tick 内不可卸补丁理由保留） |
| `packages/pier-hooks/src/player/AttackEvent.cpp` | `同路径` | ✔ 重写（**新增 thread_local 防双派发**：双挂重载曾让计数型订阅者双倍计数；取名调用补真 try/catch，兑现原注释承诺） |
| `packages/pier-hooks/src/player/GameModeEvent.cpp` | `同路径` | ✔ 重写（重入保护、from==to 静默、装钩失败告警全保留） |
| `packages/pier-hooks/src/protect/DropItemEvent.cpp` | `同路径` | ✔ 重写（两条丢弃路径论证、NoError 取消理由、装钩失败告警；无订阅者提醒合并为一处） |
| `packages/pier-hooks/src/protect/InteractEntityEvent.cpp` | `同路径` | ✔ 重写（保留挥手动画理由；补 try/catch） |
| `packages/pier-hooks/src/protect/PressurePlateEvent.cpp` | `同路径` | ✔ 重写（两钩点教训与节流全保留） |
| `packages/pier-hooks/src/protect/ProjectileEvent.cpp` | `同路径` | ✔ 重写（组件化漂移史与五钩点分工全保留；重入闸改 thread_local；装钩状态改逐点报，主钩失败单独 error） |
| `packages/pier-hooks/src/protect/PushEntityEvent.cpp` | `同路径` | ✔ 重写（双向判定、玩家对玩家放行、节流；补 try/catch） |
| `packages/pier-hooks/src/protect/RideEvent.cpp` | `同路径` | ✔ 重写（canAddPassenger 钩点论证、载具/骑乘者反转警示；补 try/catch） |
| `packages/pier-hooks/src/protect/TakeEntityEvent.cpp` | `同路径` | ✔ 重写（**修正文件头与代码不符**：旧头声称挂 Player::take、实际挂 Arrow/ThrownTrident 的 $playerTouch；isItemActor 恒 false 的现状与保留理由写明） |
| `packages/pier-hooks/src/world/ContainerEvents.cpp` | `同路径` | ✔ 重写（StopProcessing 取消通道） |
| `packages/pier-hooks/src/world/DestroyEvents.cpp` | `同路径` | ✔ 重写（autotool 时机：origin 前派发） |
| `packages/pier-hooks/src/world/DimensionEvents.cpp` | `同路径` | ✔ 重写（唯一漏斗论证、右值请求先读后转发） |
| `packages/pier-hooks/src/world/HopperEvents.cpp` | `同路径` | ✔ 重写（ICF 折叠卫与判别器全文保留） |
| `packages/pier-hooks/src/world/UseItemOnEvent.cpp` | `同路径` | ✔ 重写（TypedStorage 引用坍缩规则、平铺坐标理由、isFirstEvent 透传） |
| `packages/pier-hooks/xmake.lua` | `同路径` | ✔ 重写 |
| `packages/pier-host/include/pier/host/host_api.h` | `pier/host/{api_table,spi}.h` | ✔ 拆分重写（表所有权 + 四个 SPI 注册面） |
| `packages/pier-host/include/pier/host/hosted_mod.h` | `同路径` | ✔ 重写（ModHostName="pier"；asMod 迁入） |
| `packages/pier-host/include/pier/host/mod_control.h` | `同路径` | ✔ 重写（type:"pier"） |
| `packages/pier-host/include/pier/host/mod_host.h` | `同路径` | ✔ 重写 |
| `packages/pier-host/src/Entry.cpp` | `同路径` | ✔ 重写（建表→引导→注册管理器；布局自检按设计删除） |
| `packages/pier-host/src/MemoryOperators.cpp` | `packages/pier-host/src/MemoryOperators.cpp` | ✔ 重写（**装载阻断级**：没有它 LeviLamina 直接拒绝装载，而错误只在装的时候才报） |
| `packages/pier-host/src/ModControl.cpp` | `同路径` | ✔ 重写（/pier；并入 events/abi；events 含合成事件） |
| `packages/pier-host/src/ModHost.cpp` | `同路径` | ✔ 重写（v1 握手：vtable struct_size/flags；SPI 否决/拆除；W-EV1 保留） |
| `packages/pier-host/xmake.lua` | `packages/pier-host/xmake.lua` | ✔ 重写 |
| `packages/pier-lane/src/Lane.cpp` | `同路径` | ✔ 重写（存活格永不释放/卸载补 release/不查 isEnabled/指纹 0 堵口/锁外跨 dylib 全保留；busy 否决改注册 UnloadVeto，清理改 Teardown 40） |
| `packages/pier-lane/xmake.lua` | `同路径` | ✔ 重写（可选性说明改写为「槽位缺席即 NULL」新纪律） |
| `bindings/rust/pier-rs/Cargo.toml` | `bindings/rust/pier-rs/Cargo.toml` | ✔ 重写（✂ server/more_dimensions 两个 feature —— v1 布局不分岔，能力改运行期判断；client 只填 mod_flags 一位） |
| `bindings/rust/pier-rs/build.rs` | `—` | ✂ 不随迁 —— 它算的是车道指纹，新仓改由 `LaneContract::FINGERPRINT` 关联常量给出：两侧引用同一个契约定义时它自然一致，而靠构建期各算各的正是「手抄一份相同常量」要防的那种情况 |
| `bindings/rust/pier-rs/src/block/actions.rs` | `bindings/rust/pier-rs/src/block/state.rs` | ✔ 重写（方块读写 + edit_* 原生写入 + 液体层（含水方块是同格第二个方块）） |
| `bindings/rust/pier-rs/src/block/gap_fill.rs` | `bindings/rust/pier-rs/src/block/edit.rs` | ✔ 重写（方块读写 + edit_* 原生写入 + 液体层（含水方块是同格第二个方块）） |
| `bindings/rust/pier-rs/src/block/mod.rs` | `bindings/rust/pier-rs/src/block/mod.rs` | ✔ 重写（方块读写 + edit_* 原生写入 + 液体层（含水方块是同格第二个方块）） |
| `bindings/rust/pier-rs/src/block/query.rs` | `bindings/rust/pier-rs/src/block/props.rs` | ✔ 重写（方块读写 + edit_* 原生写入 + 液体层（含水方块是同格第二个方块）） |
| `bindings/rust/pier-rs/src/client/events.rs` | `bindings/rust/pier-rs/src/client.rs` | ✔ 重写（客户端专属；服务端宿主上是空槽而非编译错误） |
| `bindings/rust/pier-rs/src/client/input.rs` | `bindings/rust/pier-rs/src/client.rs` | ✔ 重写（客户端专属；服务端宿主上是空槽而非编译错误） |
| `bindings/rust/pier-rs/src/client/mod.rs` | `bindings/rust/pier-rs/src/client.rs` | ✔ 重写（客户端专属；服务端宿主上是空槽而非编译错误） |
| `bindings/rust/pier-rs/src/client/status.rs` | `bindings/rust/pier-rs/src/client.rs` | ✔ 重写（客户端专属；服务端宿主上是空槽而非编译错误） |
| `bindings/rust/pier-rs/src/command/builder.rs` | `bindings/rust/pier-rs/src/command.rs` | ✔ 重写（原始文本与带类型 overload 两种；命令不可反注册，闭包刻意泄漏） |
| `bindings/rust/pier-rs/src/command/mod.rs` | `bindings/rust/pier-rs/src/command.rs` | ✔ 重写（原始文本与带类型 overload 两种；命令不可反注册，闭包刻意泄漏） |
| `bindings/rust/pier-rs/src/comms/bus.rs` | `bindings/rust/pier-rs/src/bus.rs` | ✔ 重写（跨模组广播；Drop 即退订） |
| `bindings/rust/pier-rs/src/comms/kvdb.rs` | `bindings/rust/pier-rs/src/kvdb.rs` | ✔ 重写（键值库；本族线程安全，Drop 即关闭） |
| `bindings/rust/pier-rs/src/comms/mod.rs` | `—` | ✂ 目录本身的 mod 声明；扁平化之后没有对应文件（契约 §八：一个关注点一个 TU） |
| `bindings/rust/pier-rs/src/comms/more_dimensions.rs` | `bindings/rust/pier-rs/src/dimensions.rs` | ✔ 重写（md_* 可选能力包门面；规则「没登记」与「登记为 false」分开） |
| `bindings/rust/pier-rs/src/comms/packet.rs` | `bindings/rust/pier-rs/src/packet.rs` | ✔ 重写（**由真实消费方 crossbind 定义优先级**：它是 85 个 ⬜ 里第一个被真正需要的。HookDirection 一个类型拆成 Direction/Directions；闭包从 Send 收紧到 Send+Sync；PacketHook 的 Drop 即注销、forget() 显式续命） |
| `bindings/rust/pier-rs/src/comms/service.rs` | `bindings/rust/pier-rs/src/service.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/container/mod.rs` | `bindings/rust/pier-rs/src/container.rs` | ✔ 重写（五种容器；写完要 refresh，方块容器明确拒绝） |
| `bindings/rust/pier-rs/src/container/ops.rs` | `bindings/rust/pier-rs/src/container.rs` | ✔ 重写（五种容器；写完要 refresh，方块容器明确拒绝） |
| `bindings/rust/pier-rs/src/entity/actions.rs` | `bindings/rust/pier-rs/src/entity/actions.rs` | ✔ 重写（actor_* 全族；四个关系槽的两道闸留在各自调用点） |
| `bindings/rust/pier-rs/src/entity/gap_fill.rs` | `bindings/rust/pier-rs/src/entity/relations.rs` | ✔ 重写（actor_* 全族；四个关系槽的两道闸留在各自调用点） |
| `bindings/rust/pier-rs/src/entity/mod.rs` | `bindings/rust/pier-rs/src/entity/mod.rs` | ✔ 重写（actor_* 全族；四个关系槽的两道闸留在各自调用点） |
| `bindings/rust/pier-rs/src/entity/query.rs` | `bindings/rust/pier-rs/src/entity/props.rs` | ✔ 重写（actor_* 全族；四个关系槽的两道闸留在各自调用点） |
| `bindings/rust/pier-rs/src/event/mod.rs` | `bindings/rust/pier-rs/src/event/mod.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/event/names/mob.rs` | `bindings/rust/pier-rs/src/event/names.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/event/names/mod.rs` | `bindings/rust/pier-rs/src/event/names.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/event/names/player.rs` | `bindings/rust/pier-rs/src/event/names.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/event/names/server.rs` | `bindings/rust/pier-rs/src/event/names.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/gui/custom.rs` | `bindings/rust/pier-rs/src/gui.rs` | ✔ 重写（三种表单；回调至多一次，静音那一路刻意泄漏） |
| `bindings/rust/pier-rs/src/gui/mod.rs` | `bindings/rust/pier-rs/src/gui.rs` | ✔ 重写（三种表单；回调至多一次，静音那一路刻意泄漏） |
| `bindings/rust/pier-rs/src/gui/modal.rs` | `bindings/rust/pier-rs/src/gui.rs` | ✔ 重写（三种表单；回调至多一次，静音那一路刻意泄漏） |
| `bindings/rust/pier-rs/src/gui/simple.rs` | `bindings/rust/pier-rs/src/gui.rs` | ✔ 重写（三种表单；回调至多一次，静音那一路刻意泄漏） |
| `bindings/rust/pier-rs/src/item/gap_fill.rs` | `bindings/rust/pier-rs/src/item.rs` | ✔ 重写（ItemStack 值对象；SNBT 一律经 NbtValue::to_snbt 转义） |
| `bindings/rust/pier-rs/src/item/mod.rs` | `bindings/rust/pier-rs/src/item.rs` | ✔ 重写（ItemStack 值对象；SNBT 一律经 NbtValue::to_snbt 转义） |
| `bindings/rust/pier-rs/src/item/query.rs` | `bindings/rust/pier-rs/src/item.rs` | ✔ 重写（ItemStack 值对象；SNBT 一律经 NbtValue::to_snbt 转义） |
| `bindings/rust/pier-rs/src/lane/lane_error.rs` | `bindings/rust/pier-rs/src/lane.rs` | ✔ 重写（快车道；alive 用 Acquire 读，调用期间占住 busy） |
| `bindings/rust/pier-rs/src/lane/mod.rs` | `bindings/rust/pier-rs/src/lane.rs` | ✔ 重写（快车道；alive 用 Acquire 读，调用期间占住 busy） |
| `bindings/rust/pier-rs/src/lib.rs` | `bindings/rust/pier-rs/src/lib.rs` | ✔ 重写（✂ 全部 cfg 裁剪模块树；同一份源码两个目标都编得过） |
| `bindings/rust/pier-rs/src/misc/edit.rs` | `bindings/rust/pier-rs/src/block.rs` | ✔ 重写（方块读写 + edit_* 原生写入 + 液体层（含水方块是同格第二个方块）） |
| `bindings/rust/pier-rs/src/misc/mod.rs` | `—` | ✂ 目录本身的 mod 声明；扁平化之后没有对应文件（契约 §八：一个关注点一个 TU） |
| `bindings/rust/pier-rs/src/misc/system.rs` | `bindings/rust/pier-rs/src/host.rs` | ✔ 重写（本行与下面 host.rs 那行说的是同一个旧文件；台账里它被列了两次，状态曾互相矛盾） |
| `bindings/rust/pier-rs/src/misc/types.rs` | `bindings/rust/pier-rs/src/types.rs` | ✔ 重写（共享值类型收口（坐标/枚举/位标志），域模块之间共享它而不互相依赖） |
| `bindings/rust/pier-rs/src/money/listen.rs` | `bindings/rust/pier-rs/src/money.rs` | ✔ 重写（经济全族；balance 把 -1 翻成 Err） |
| `bindings/rust/pier-rs/src/money/mod.rs` | `bindings/rust/pier-rs/src/money.rs` | ✔ 重写（经济全族；balance 把 -1 翻成 Err） |
| `bindings/rust/pier-rs/src/money/ops.rs` | `bindings/rust/pier-rs/src/money.rs` | ✔ 重写（经济全族；balance 把 -1 翻成 Err） |
| `bindings/rust/pier-rs/src/nbt/accessors.rs` | `bindings/rust/pier-rs/src/nbt/mod.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/nbt/binary.rs` | `bindings/rust/pier-rs/src/nbt/binary.rs` | ✔ 重写（SNBT ↔ 二进制，走宿主解析器） |
| `bindings/rust/pier-rs/src/nbt/mod.rs` | `bindings/rust/pier-rs/src/nbt/mod.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/nbt/parser/containers.rs` | `bindings/rust/pier-rs/src/nbt/parse.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/nbt/parser/mod.rs` | `bindings/rust/pier-rs/src/nbt/parse.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/nbt/parser/scalars.rs` | `bindings/rust/pier-rs/src/nbt/parse.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/nbt/serde.rs` | `bindings/rust/pier-rs/src/nbt/mod.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/player/actions.rs` | `bindings/rust/pier-rs/src/player/admin.rs` | ✔ 重写（player_* 全族；选择器身份纪律写在模块头） |
| `bindings/rust/pier-rs/src/player/gap_fill.rs` | `bindings/rust/pier-rs/src/player/props.rs` | ✔ 重写（player_* 全族；选择器身份纪律写在模块头） |
| `bindings/rust/pier-rs/src/player/inventory.rs` | `bindings/rust/pier-rs/src/player/items.rs` | ✔ 重写（player_* 全族；选择器身份纪律写在模块头） |
| `bindings/rust/pier-rs/src/player/mod.rs` | `bindings/rust/pier-rs/src/player/mod.rs` | ✔ 重写（player_* 全族；选择器身份纪律写在模块头） |
| `bindings/rust/pier-rs/src/player/query.rs` | `bindings/rust/pier-rs/src/player/props.rs` | ✔ 重写（player_* 全族；选择器身份纪律写在模块头） |
| `bindings/rust/pier-rs/src/player/types.rs` | `bindings/rust/pier-rs/src/types.rs` | ✔ 重写（共享值类型收口（坐标/枚举/位标志），域模块之间共享它而不互相依赖） |
| `bindings/rust/pier-rs/src/rt/error.rs` | `bindings/rust/pier-rs/src/rt/error.rs` | ✔ 重写 |
| `bindings/rust/pier-rs/src/rt/ffi.rs` | `bindings/rust/pier-rs/src/rt/ffi.rs` | ✔ 重写（PierStr 改 c_char 指针；UTF-8 校验与一次性告警保留；新增 collect_byte_chunks） |
| `bindings/rust/pier-rs/src/rt/handle.rs` | `bindings/rust/pier-rs/src/rt/handle.rs` | ✔ 重写（AtomicPtr 而非 unsafe impl Send+Sync，理由写在原地） |
| `bindings/rust/pier-rs/src/rt/logger.rs` | `bindings/rust/pier-rs/src/rt/logger.rs` | ✔ 重写（槽位改 Option，填表漏了不再崩在日志上；补 fatal） |
| `bindings/rust/pier-rs/src/rt/mod.rs` | `bindings/rust/pier-rs/src/rt/mod.rs` | ✔ 重写 |
| `bindings/rust/pier-rs/src/rt/registration.rs` | `bindings/rust/pier-rs/src/rt/registration.rs` | ✔ 重写（v1 三道握手：长度→版本区间→目标标志；✂ TARGET_MASK 高位标记；vtable 填四字段头） |
| `bindings/rust/pier-rs/src/misc/system.rs` | `bindings/rust/pier-rs/src/host.rs` | ✔ 新建（宿主/系统层面的能力聚到一处；同时是 rt::ffi 那几个 sink 的第一个调用方） |
| `bindings/rust/pier-rs/src/rt/runtime.rs` | `bindings/rust/pier-rs/src/rt/runtime.rs` | ✔ 重写（require_slot! 补第二道闸「槽非空」；报错文案 ✂ 历史产品名，改为带上宿主 ABI 版本与表长） |
| `bindings/rust/pier-rs/src/scoreboard/mod.rs` | `bindings/rust/pier-rs/src/scoreboard.rs` | ✔ 重写（scoreboard_op 多路槽的具名门面；「没有分数」与「分数是 0」分开） |
| `bindings/rust/pier-rs/src/scoreboard/ops.rs` | `bindings/rust/pier-rs/src/scoreboard.rs` | ✔ 重写（scoreboard_op 多路槽的具名门面；「没有分数」与「分数是 0」分开） |
| `bindings/rust/pier-rs/src/server/mod.rs` | `bindings/rust/pier-rs/src/server.rs` | ✔ 重写（tick 冻结/步进/倍速 + 分项采样） |
| `bindings/rust/pier-rs/src/server/ops/commands.rs` | `bindings/rust/pier-rs/src/command.rs` | ✔ 重写（原始文本与带类型 overload 两种；命令不可反注册，闭包刻意泄漏） |
| `bindings/rust/pier-rs/src/server/ops/events.rs` | `bindings/rust/pier-rs/src/event/mod.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/server/ops/mod.rs` | `—` | ✂ 目录本身的 mod 声明；扁平化之后没有对应文件（契约 §八：一个关注点一个 TU） |
| `bindings/rust/pier-rs/src/server/ops/sim.rs` | `bindings/rust/pier-rs/src/sim.rs` | ✔ 重写（模拟玩家动词表） |
| `bindings/rust/pier-rs/src/server/ops/status.rs` | `bindings/rust/pier-rs/src/host.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/server/ops/time.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ 重写（关卡读写 + 流式扫描 + 区块存档键（二进制键不过 UTF-8）） |
| `bindings/rust/pier-rs/src/server/run/data.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ 重写（关卡读写 + 流式扫描 + 区块存档键（二进制键不过 UTF-8）） |
| `bindings/rust/pier-rs/src/server/run/fill.rs` | `bindings/rust/pier-rs/src/world/commands.rs` | ✔ 重写（方块读写 + edit_* 原生写入 + 液体层（含水方块是同格第二个方块）） |
| `bindings/rust/pier-rs/src/server/run/mod.rs` | `—` | ✂ 目录本身的 mod 声明；扁平化之后没有对应文件（契约 §八：一个关注点一个 TU） |
| `bindings/rust/pier-rs/src/server/run/profiler.rs` | `bindings/rust/pier-rs/src/server.rs` | ✔ 重写（tick 冻结/步进/倍速 + 分项采样） |
| `bindings/rust/pier-rs/src/server/run/tick.rs` | `bindings/rust/pier-rs/src/server.rs` | ✔ 重写（tick 冻结/步进/倍速 + 分项采样） |
| `bindings/rust/pier-rs/src/server/run/ticking.rs` | `bindings/rust/pier-rs/src/server.rs` | ✔ 重写（tick 冻结/步进/倍速 + 分项采样） |
| `bindings/rust/pier-rs/src/server/sel/dimsel.rs` | `bindings/rust/pier-rs/src/sel.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/server/sel/mod.rs` | `bindings/rust/pier-rs/src/sel.rs` | ✔ 重写（并入扁平域模块） |
| `bindings/rust/pier-rs/src/server/world/blocks.rs` | `bindings/rust/pier-rs/src/block.rs` | ✔ 重写（方块读写 + edit_* 原生写入 + 液体层（含水方块是同格第二个方块）） |
| `bindings/rust/pier-rs/src/server/world/entities.rs` | `bindings/rust/pier-rs/src/entity.rs` | ✔ 重写（actor_* 全族；四个关系槽的两道闸留在各自调用点） |
| `bindings/rust/pier-rs/src/server/world/gap_fill.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ 重写（关卡读写 + 流式扫描 + 区块存档键（二进制键不过 UTF-8）） |
| `bindings/rust/pier-rs/src/server/world/mod.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ 重写（关卡读写 + 流式扫描 + 区块存档键（二进制键不过 UTF-8）） |
| `bindings/rust/pier-rs/src/server/world/particles.rs` | `bindings/rust/pier-rs/src/world/edit.rs` | ✔ 重写（关卡读写 + 流式扫描 + 区块存档键（二进制键不过 UTF-8）） |
| `bindings/rust/pier-rs/src/sim/actions.rs` | `bindings/rust/pier-rs/src/sim.rs` | ✔ 重写（模拟玩家动词表） |
| `bindings/rust/pier-rs/src/sim/mod.rs` | `bindings/rust/pier-rs/src/sim.rs` | ✔ 重写（模拟玩家动词表） |
| `bindings/rust/pier-rs/src/world/mod.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ 重写（关卡读写 + 流式扫描 + 区块存档键（二进制键不过 UTF-8）） |
| `bindings/rust/pier-rs/src/world/scan.rs` | `bindings/rust/pier-rs/src/world/edit.rs` | ✔ 重写（关卡读写 + 流式扫描 + 区块存档键（二进制键不过 UTF-8）） |
| `bindings/rust/pier-rs/src/world/structures.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ 重写（关卡读写 + 流式扫描 + 区块存档键（二进制键不过 UTF-8）） |
| `bindings/rust/pier-rs/tests/lane.rs` | `bindings/rust/pier-rs/tests/lane.rs` | ⬜ 待迁移（逐文件重写） |
| `bindings/rust/pier-sys-rs/Cargo.toml` | `bindings/rust/pier-sys-rs/Cargo.toml` | ✔ 重写（✂ server/client/more_dimensions 三个会改布局的 feature —— 那正是「两侧错位 7 槽」的根因；只留 client，且它一个字段都不增删） |
| `bindings/rust/pier-sys-rs/src/api.rs` | `bindings/rust/pier-sys-rs/src/api.rs` | ✔ 重写（194 字段逐格对 abi.h，sys-mirrors-abi 已守；零 #[cfg]） |
| `bindings/rust/pier-sys-rs/src/consts/actor.rs` | `bindings/rust/pier-sys-rs/src/consts/actor.rs` | ✔ 重写（85 个常量；搬运时漏掉的 PIER_AACT_ADD_EFFECT 已由机检逮到并补上） |
| `bindings/rust/pier-sys-rs/src/consts/player.rs` | `bindings/rust/pier-sys-rs/src/consts/player.rs` | ✔ 重写（73 个常量） |
| `bindings/rust/pier-sys-rs/src/consts/world.rs` | `bindings/rust/pier-sys-rs/src/consts/world.rs` | ✔ 重写（123 个常量） |
| `bindings/rust/pier-sys-rs/src/consts.rs` | `bindings/rust/pier-sys-rs/src/consts.rs` | ✔ 重写 |
| `bindings/rust/pier-sys-rs/src/lib.rs` | `bindings/rust/pier-sys-rs/src/lib.rs` | ✔ 重写（✂ PIER_ABI_TARGET_MASK / TAGGED_VERSION —— 目标标记改走 mod_flags） |
| `bindings/rust/pier-sys-rs/src/money.rs` | `src/types.rs`（并入） | ✔ 并入 types.rs；LLMoneyEvent → PierMoneyEvent（旧名与 LegacyMoney 全局同名类型撞车） |
| `bindings/rust/pier-sys-rs/src/types.rs` | `bindings/rust/pier-sys-rs/src/types.rs` | ✔ 重写（PierStr 显式 {ptr,len}；19 个回调签名逐个对上） |
| `bindings/rust/pier-sys-rs/src/vtable.rs` | `bindings/rust/pier-sys-rs/src/vtable.rs` | ✔ 重写（v1 新握手：struct_size / abi_version / mod_flags 四字段头） |
| `—` | `compile_commands.json` | ✂ 构建产物（xmake 生成的编译数据库），不是源文件；已加进 `.gitignore`。这一行存在只是为了让 `ledger-covers-tree` 对工作区里这份残留也有出处 —— 清单漏一项，按清单核对就永远查不出那一项 |
| `—` | `bindings/rust/pier-rs/src/rt/accessors.rs` | ✔ 新建（`accessors!` 宏 —— 属性访问器由常量表生成，取代四个域里几十个一行体查表方法） |
| `—` | `bindings/rust/pier-rs/src/block/state.rs` | ✔ 新建（方块状态与方块实体（从扁平的 block.rs 拆出）） |
| `—` | `bindings/rust/pier-rs/src/block/edit.rs` | ✔ 新建（方块写入与液体层（从扁平的 block.rs 拆出）） |
| `—` | `bindings/rust/pier-rs/src/entity/relations.rs` | ✔ 新建（实体关系、装备效果、射线（从扁平的 entity.rs 拆出）） |
| `—` | `bindings/rust/pier-rs/src/player/io.rs` | ✔ 新建（玩家出站通道：消息、标题、粒子、原始包（从扁平的 player.rs 拆出）） |
| `xmake.lua` | `xmake.lua` | ✔ 重写（全局宏根作用域；object 包聚合） |


## 附：升级到本版时**运行时可见**的改名

下面这些不是代码内部的事，是服主/运维会碰到的东西。逐条列出来，免得升级之后
「按老文档操作没反应」变成一次排查。

| 旧 | 新 | 影响 |
|---|---|---|
| 环境变量 `MORE_DIMENSIONS_DEF_MIN` / `..._MAX` | `PIER_DIM_DEF_MIN` / `PIER_DIM_DEF_MAX` | 仅诊断开关（覆盖告诉客户端的维度高度）。不设置时行为不变。 |
| 环境变量 `PIER_TRACE_FORM` | 不变 | — |
| 维度台账 `worlds/<levelName>/dimension_config.json` | **不变** | 早先有注释误称它在 `configs/levilamina-rust-loader/dimensions.json`，那是错的；它一直跟着存档走，升级不需要搬动任何文件。 |
| 命令 `/pier` | 不变 | — |

**没有**任何存档数据格式变更：维度 id 仍由引擎的 NameIdStore 持有，玩家存档
里的 DimensionId 不受本次重写影响。




### 新架构新建 / 改名的文件

旧仓没有对应物，所以逐行迁移时不会出现在上表里 —— 而这正是台账此前
**单向**的证据：`ledger-covers-tree` 机检加上之后一次逮到 31 个。

| 新位置 | 出处 |
|---|---|
| `packages/pier-api/include/pier/api/bridge.h` | ✔ 新建 —— 解析助手的声明 |
| `packages/pier-api/src/core/Bridge.cpp` | ✔ 新建 —— 旧仓散在各 TU 的解析助手收口 |
| `packages/pier-api/src/core/Enrich.cpp` | ✔ 新建 —— enrichEventData 独立成 TU |
| `packages/pier-api/src/core/GameStats.cpp` | ✔ 新建 |
| `packages/pier-api/src/core/Log.cpp` | ✔ 新建 —— log 槽独立成 TU（它必须不染任何域依赖） |
| `packages/pier-api/src/core/Scheduler.cpp` | ✔ 新建 |
| `packages/pier-api/src/runtime/CommandEvents.cpp` | ✔ 新建 —— 命令事件提供方（covers_registry=true） |
| `packages/pier-api/src/world/BlockResolve.cpp` | ✔ 新建 —— 修旧仓的真链接断裂：三个助手定义在 server-only 的 Edit.cpp、却被双目标的 World.cpp 引用 |
| `packages/pier-client/xmake.lua` | ✔ 新建 —— 客户端槽位独立成包 |
| `packages/pier-dimensions/include/pier/dimensions/base/native_dimensions.h` | ✔ 由旧 `base/NativeDimensions.h` 改名（snake_case） |
| `packages/pier-dimensions/include/pier/dimensions/base/simple_custom_dimension.h` | ✔ 由旧 `base/SimpleCustomDimension.h` 改名 |
| `packages/pier-dimensions/include/pier/dimensions/base/utils.h` | ✔ 由旧 `base/Utils.h` 改名 |
| `packages/pier-dimensions/include/pier/dimensions/dim/complete_base_types.h` | ✔ 由旧 `dim/CompleteBaseTypes.h` 改名 |
| `packages/pier-dimensions/include/pier/dimensions/dim/custom_dimension_config.h` | ✔ 由旧 `dim/CustomDimensionConfig.h` 改名 |
| `packages/pier-dimensions/include/pier/dimensions/dim/dimension_height.h` | ✔ 由旧 `dim/DimensionHeight.h` 改名 |
| `packages/pier-dimensions/src/base/Utils.cpp` | ✔ 由旧 `src/rt/Utils.cpp` 迁来（见上一行） |
| `packages/pier-dimensions/src/rt/Bridge.cpp` | ✔ 新建 —— spi::DimensionBridge 的唯一实现，含 id 一致性安全闸 |
| `packages/pier-dimensions/src/rt/Slots.cpp` | ✔ 新建 —— md_* 十一槽装填 |
| `packages/pier-host/include/pier/host/api_table.h` | ✔ 新建 —— PierApi 表的所有者 |
| `packages/pier-host/include/pier/host/spi.h` | ✔ 新建 —— 六个注册面，取代旧仓 ApiTable 的硬编码转接（契约 §一 规则二） |
| `packages/pier-host/src/ApiTable.cpp` | ✔ 新建 —— 只填表头四个标量，域槽位由各能力包自填 |
| `packages/pier-host/src/Spi.cpp` | ✔ 新建 —— SPI 注册表本体 |
| `packages/pier-support/include/pier/support/guard.h` | ✔ 新建 —— PIER_API_GUARD_*，旧仓的 W12 屏障收口 |
| `packages/pier-support/include/pier/support/log.h` | ✔ 新建 |
| `packages/pier-support/include/pier/support/module.h` | ✔ 新建 |
| `packages/pier-support/include/pier/support/snbt.h` | ✔ 新建 |
| `packages/pier-support/include/pier/support/str.h` | ✔ 新建 —— PierStr ↔ string_view，旧仓靠 PierStr 就是 string_view 的别名，v1 重切后必须显式转换 |
| `packages/pier-support/src/Log.cpp` | ✔ 新建 —— 宿主 logger 的唯一出处 |
| `packages/pier-support/src/Module.cpp` | ✔ 新建 —— addressOwnedBy 从 pier-api 下沉到这里 |
| `packages/pier-support/src/Snbt.cpp` | ✔ 新建 —— SNBT 转义/数字格式化，旧仓散在各处 |
| `packages/pier-support/xmake.lua` | ✔ 新建 —— pier-support 整包是新架构引入的层（日志入口必须住在比 host 更低的地方，见契约 §一） |

**统计**：✔ 199 ｜ ✂ 12 ｜ ⬜ 45（共 256 个旧文件）

---

## 第一次真机编译的修复记录（2026-08-30）

`build.bat`（MSVC 2022 + xmake）与 `cargo clippy --workspace -- -D warnings`
第一次真跑，报出四类错误。逐条：

| 报错 | 根因 | 修法 | 现在谁在守 |
|---|---|---|---|
| `Entry.cpp(47) C2065: "ModHostName" 未声明` | 用了 `hosted_mod.h` 的常量但只 include 了 `mod_host.h`，后者不传递包含 | 补 include | `tools/include-surrogate.py`（新） |
| `api.rs: cannot find type int`（2 处） | 手抄镜像时 C 的 `int` 原样搬了过来 | 见下 | `tools/rust-surrogate.py` 扩展 |
| `api.rs: expected one of ... found short` | 同上，`unsigned short` | 见下 | 同上 |
| `player.rs:173 expected item after doc comment` | `abi.h` 里跨行的**尾注**属于上一项，原地搬过来就挂到了下一项 | 重新生成三个 consts 文件，注释归属做对 | 同上 |
| `consts.rs: unused import player::*` | 上一条的连带（模块解析失败 → 零个项） | 随上一条修复 | — |

**根因不在镜像，在契约本身。** `money_*` 一族有九个槽直接抄了 LegacyMoney 的
签名，带着 `long long` / `int` / `unsigned short` —— 宽度平台相关，而其余
190 个槽全是定宽类型。C 编译器不报（它们是合法 C），Rust 侧才炸。已全部
改成 `int64_t` / `int32_t` / `uint16_t`（MSVC x64 上二进制完全相同，v1 未
发布，零成本），并加了 `abi-fixed-width` 机检。

**顺带补上了一个真缺口**：`sys-mirrors-abi` 原本只比对槽**名**，不比对签名。
`cargo check` 逮得到「`int` 不是 Rust 类型」，但逮不到「镜像写 `i32` 而
`abi.h` 是 `int64_t`」—— 那种两边都编得过，运行期读到半个数。现在 190 个槽
的签名逐参数比对，并已验证它能逮到宽度错和参数换位。

---

## 第二次真机编译的修复记录

`xmake` 过了 Entry.cpp 那一关往下编，`cargo clippy` 也往下走了一步。两侧各报一类：

| 报错 | 根因 | 修法 | 现在谁在守 |
|---|---|---|---|
| `spi.h(155) C2039: "string" 不是 "std" 的成员` | 用 `std::string` 只 include 了 `<string_view>` | 补 `<string>`；**实查全仓 17 处同类**，一并补齐 | `include-surrogate` 扩展（第二类判据） |
| `ffi.rs` 七个 helper `never used`（`-D warnings`） | 铺了基础设施但没有调用方 | 见下 | `rust-surrogate` 扩展（死代码判据） |

**那 17 处此刻能编过，只是别的头碰巧把标准头带进来了。** 删掉那个文件的
一行 include、或者换一版标准库，它们会一起炸。真机只报了 1 处，是因为
编译停在了第一个。

**七条 dead_code 的处理方式值得记。** 有三条路：加 `#[allow(dead_code)]`、
删掉、或者给它们真正的调用方。选了后两条的组合：

* `r` / `push_string` / `set_string` / `call_out_str` / `collect_strs`
  → 新增 `pier-rs/src/host.rs`（宿主与系统层面的能力：运行阶段、排期、
  执行命令、列事件 id、系统/服务器信息），它们成了这五个的**第一个调用方**；
* `collect_bytes` / `collect_byte_chunks` → **删除**。它们没有调用方，
  等第一个字节 sink 的域（NBT 二进制、数据包体）落地时再回来。

纪律是：**只发布有调用方的东西**。一个没有调用方的 helper，它对 `ctx`
类型的那些 `# Safety` 断言从来没有被任何真实调用点检验过 —— 那不是
「准备好了」，是「看起来准备好了」。`-D warnings` 在这里是对的，用
`#[allow]` 绕过去才是错的。

`hello-pier` 也跟着用上了 `host` 层：它现在真的会问服务器阶段、在线人数、
tick，列一遍事件 id，并丢一个延迟任务回服务器线程 —— 契约 §十 四步从
「读起来对」变成了「跑起来能验」。

---

## 第三次真机编译的修复记录

C++ 侧过了 pier-host / pier-hooks / pier-dimensions 的大部分（30 个 TU），
停在 `pier-lane`；Rust 侧只剩一条 dead_code。

| 报错 | 根因 | 修法 | 现在谁在守 |
|---|---|---|---|
| `hosted_mod.h(9) C1083: 无法打开 "ll/api/event/ListenerBase.h"` | `pier-lane` 没有 `add_packages("levilamina")` | 补上 | `build-config` 改为**按 include 闭包**算 |
| `C4819`（每个文件一条） | MSVC 在 936 代码页下读 UTF-8 中文注释 | 根 xmake 加 `add_cxflags("/utf-8")` | `build-config` 新增判据 |
| `host.rs: method handle is never used` | `Host::handle` 零调用方 | 删除 | `rust-surrogate` 死代码判据改为**方法感知** |

### 一、`build-config` 漏报的根因值得记

`Lane.cpp` 自己一行 `ll/` 的 include 都没写 —— 它只 include 了标准库和
`pier/`。但 `pier/host/hosted_mod.h` 里有 `ll/api/event/ListenerBase.h`。
**编译器展开的是 include 的闭包，不是第一层**，而第一版的检查只看了第一层。

这条判据的形状和 `include-surrogate` 的第二类（`std::X` 的来源）完全相同：
能不能编过取决于**闭包**里有什么，不取决于这个文件自己写了什么。

### 二、`Host::handle` 为什么会从 surrogate 底下溜过去

死代码判据是「名字在 crate 里出现次数 > 1」。对自由函数够用；对 impl 里的
方法不够 —— `handle` 这个词在 crate 里出现十几次（`Handle` 类型、`handle:`
字段、`set_runtime(api, handle)` 的参数名），计数轻松过关。

改成看**调用形状**（`.name(` / `::name(`）之后，又撞上另一个限制：
`Runtime::handle` 和 `Host::handle` 同名，`rt.handle()` 到底调的是哪个，
文本判据分不开。这是硬边界，需要类型推导。

处理方式是**如实报出来**而不是假装覆盖到：重名的方法名被明确排除，并在
输出里列出是哪些、定义在哪些类型上。一条检查说得清自己漏了什么，比它假装
什么都覆盖到了有用得多（契约 §九：PASS 只能给覆盖到的那部分打 ✓）。

### 三、顺手清掉的两处「凭空造的 API」

* `Host::handle()` —— 零调用方；
* `Host::no_task()` —— 它只是把已经公开的 `TaskId::NONE` 又包了一层。

删掉 `no_task` 之后 `use ... TaskId` 立刻变成未使用 —— 这类连锁是「删代码」
最容易漏的一环，所以 `rust-surrogate` 也加了未使用 `use` 的判据。

### 四、我在这一轮踩的第四次同族的坑

`/utf-8` 那条检查第一版写的是「根 xmake 全文里有没有出现 `/utf-8`」。
而我给那行代码写的解释性注释里就写着 `/utf-8` —— 于是把 `add_cxflags`
那一行删掉，检查照样绿。

前三次是「在剥掉 X 的文本里找 X」，这次是「在包含注释的全文里找代码特征」。
统一的形状是：**判据看的东西，和它想断言的东西，不是同一个东西。**
已改为先剥 Lua 注释再匹配 `add_cxflags(...)`。

---

## 第四次真机编译的修复记录

C++ 侧编到 `pier-api`（45%），停在 `core/Log.cpp`。

| 报错 | 根因 | 修法 |
|---|---|---|
| `Log.cpp(24) C3861: "sv" 找不到标识符`（6 处） | 用了 `pier::sv` 但没 include `pier/support/str.h` | 补 include |

**这次的重点不是那个修复，是 `include-surrogate` 本该逮到它却没逮到。**
它上一轮刚为这类问题写的，一次就漏。查下来是**四个**独立缺陷叠在一起：

1. **只扫大写开头、4 字符以上的标识符。** `sv` / `ps` / `toString` 这一族
   自由函数从来没进过视野。
2. **符号表把类成员当自由函数。** `class X { static X& getInstance(); }` 里的
   `getInstance` 被收成了一个自由符号，于是每个调 `foo.getInstance()` 的 TU
   都被要求 include 那个类的头 —— 放开第 1 条之后立刻冒出 22 条全是这种误报。
3. **只认以 `;` 收尾的函数声明。** 头文件里的**内联定义**（`inline
   std::string_view sv(PierStr s) { ... }`）以 `{` 收尾，而作用域切分恰恰
   在 `{` 处断开片段 —— 等于要求一个必然不在片段里的字符。
4. **前向声明被当成定义。** `struct DimensionFactoryInfo;` 让真正的定义变成
   「有歧义」，整个名字被排除出检查。这一条是**静默漏报**，最难发现。

修完之后它一次逮到 **4 处**（`Log.cpp` 的 `sv`、`Bridge.cpp` 的 `sv` 加
`<mutex>` / `<unordered_set>`），真机只报了 1 处 —— 其余 3 处在编译器走到
之前。

### 这一轮的第五次同族的坑

第 3 条就是：**判据看的东西（收尾符 `{`），和它想断言的东西（这里有个函数），
不是同一个东西。** 前四次分别是「在剥掉注释的文本里找注释」「在剥掉字符串的
文本里找 include 路径」「字符串正则不跨行」「在含注释的全文里找代码特征」。

五次形状完全一致。已经不是偶然，是这类文本检查的固有陷阱 ——
写判据时必须先问：**我扫描的那个文本，还含不含我要找的东西？**

### 顺带

符号表现在覆盖 102 个名字、零歧义，其中包括 `sv` / `ps` / `toString` /
`snbtEscape` / `asMod` / `idMatches` 这一整族 —— 也就是 `pier-api` 那 21 个
还没编到的文件最可能用到的东西。

---

## 第五次真机编译的修复记录

C++ 侧编到 50%（`NbtApi.cpp`），停在 `core/Enrich.cpp`。这是第一次报**引擎
API 层**的错误，而不是 include 之类的形式问题。

| 报错 | 根因 |
|---|---|
| `Enrich.cpp(199) C2228: ".get" 的左边必须有类/结构/联合`（`ActorDamageCause`） | `mCause` 装的是**枚举**，TypedStorage 对标量有特化 |
| `Enrich.cpp(216) C2039: "get" 不是 "Dimension" 的成员` | `mDimension` 装的是 `Dimension&`，引用特化 |

### 规则本来就在仓库里，只是散在四个文件的注释中

`UseItemOnEvent.cpp` 写着「标量**和引用**都坍缩；只有类类型的值才保持包装」，
`ChunkTrace.cpp` 写着「装引用时不保证有 operator->」，
`DropItemEvent.cpp` 写着 unique_ptr 那一格。三份措辞各不相同。

**规则有四个出处就等于没有出处** —— 谁也不知道哪一份是最新的、哪一份最全。
现在唯一出处是 `tools/typed-storage.py` 的文件头（含完整的四格规则表），
那三处注释改成指过去。

### 新增 `tools/typed-storage.py`

它**读引擎头**判定每个 `m*` 成员的 `T`，再逐个校验 `.get()` 用得对不对，
两个方向都查：

* 坍缩类型上写了 `.get()` → 编译错误（真机报的两条）；
* 类类型的值上**漏了** `.get()` → 同样是编译错误，只是症状不同。

引擎头在 LeviLamina 的 xmake 包目录里，这台机器上没有 —— 找不到时报
**SKIP 而不是 PASS**。用 `PIER_LL_INCLUDE` 指过去就能跑。已用一份合成的
引擎头验证过它能原样复现真机那两条。

价值在于**一次报完**：编译器一次只报第一个失败的 TU，而这个脚本把全仓
30 个 `.get()` 调用点一起验了 —— `pier-api` 还有 19 个 TU 没编到，里面
就有 `mSerializationId` / `mGameRules` / `mAttachedBlocks` 这些还没验过的。

### driver 也改了

`run-surrogates.py` 原来把「跳过」算进「全过」——那正是这套工具一直在
反对的「把没覆盖到的说成覆盖到了」。现在 PASS / SKIP 分开报，并明说
**跳过的那几条没有结论**。

### 顺带订正一条撒谎的注释

`Enrich.cpp` 里「走公开成员，不调虚函数」那句，在改用
`getDimensionId().value()` 之后就不成立了。按 §5.4 删掉，并写明为什么
不去省这一次虚调用：`Dimension::mId` 的 TypedStorage 形状我没有验证过，
而 `getDimensionId().value()` 已经随整个 pier-dimensions 编译通过 ——
**用没验证过的写法去省一次虚调用，不划算。**

---

## 第六次真机构建：C++ 侧编译完成

98 个 TU **全部编过**，prelink 跑完，`pier.dll` 链好，走到 90%。

停在这里：

    error: fatal: Not a valid object name HEAD

**这不是编译或链接错误，是 git 的。** `levibuildscript` 的 modpacker 在打包
阶段读 git 拿版本号，而工作目录是个没有提交的仓库（或根本不是仓库）。

### 这条错误的形状值得记

它出现在**十分钟构建的最后一步**，而它拦的东西**一秒就能查**。
新增 `tools/build-prereqs.py`，在 `run-surrogates` 里排**最前**：

    git init && git add -A && git commit -m "pier v1"
    git tag v1.0.0        # 可选，否则版本号退化成提交哈希

三种状态（不是仓库 / 空仓库 / 有提交）都验过，报的话术直接给出要敲的命令
（契约 §5.3：日志要能回答「我该做什么」）。

### 顺带发现台账是**单向**的

补 `LICENSE` 时发现：三份 `Cargo.toml` 都声明 `license = "Apache-2.0"`，
而 LICENSE 文件没跟过来 —— **台账里连这一行都没有**，所以逐行清点一百遍
也发现不了。

> 清单漏了一项，按清单核对就永远查不出那一项。

新增 `ledger-covers-tree` 机检（管**反方向**：工作区里的每个文件，台账里
都要有一行）。一次逮到 **31 个**从来没被清点过的文件 —— 全是新架构新建或
改名的，旧仓没有对应物，所以逐行迁移时压根不会出现。已全部补进台账并写明
出处。

台账从此双向：`ledger-count` 管「台账 → 计数」，`ledger-covers-tree` 管
「工作区 → 台账」。

---

## 第七次：构建全过，装载被拒

`pier.dll` 打包完成，放进 `mods/` 启服务器：

    ERROR [LeviLamina] 无法加载 Pier
    ERROR [LeviLamina] Pier 将不会被加载因为没有使用统一的内存分配操作符。

`packages/pier-host/src/MemoryOperators.cpp` 从头到尾没写。它 5 行。

### 台账是对的，交付说明是错的

那一行在台账里一直挂着 ⬜。而交付说明**连续三轮**写「C++ 侧全量完成」。

数据一直在表里，只是清点脚本只报总数（`✔113 ✂4 ⬜132`），而「C++ 侧完成」
是一个**分区断言** —— 一个只报总数的清点挡不住它。三轮都没被拦下。

修法是让 `ledger-count` 每次都打一张**按区域拆的 ⬜ 表**，并在 C++ 八个包
里还有 ⬜ 时直接警告「交付说明里不许写 C++ 侧全量完成」。已验证它在把
`MemoryOperators.cpp` 改回 ⬜ 时会响。

顺带清掉另外两条一直挂着的 ⬜，它们其实早该是 ✂：
`pier-api/src/core/ApiTable.cpp` 和 `internal_api.h` —— 旧版那张硬编码转接表
及其声明面，新架构由各能力包经 `spi::SlotPack` 自填槽位（契约 §一 规则二），
表已经不存在了，只是没人回去改状态。

**C++ 侧现在真的零 ⬜，而且这句话是可核对的。**

### 新增 `host-loadable` 机检

三条判据，对着 LeviLamina 的官方模板逐条来：

1. 恰好一个 TU 定义 `LL_MEMORY_OPERATORS` 并 include 那个头；
2. 恰好一处 `LL_REGISTER_MOD(...)`；
3. **那两个 TU 所在的包必须 `set_kind("object")` 且在根 xmake 的必编列表里。**

第 3 条是最值钱的一半：前两条「文件在不在」肉眼也看得出来，而「它到底有没有
被链进去」看不出来 —— 这两个 TU 零外部符号引用，静态库会整个丢掉它们，
而**丢掉之后的症状和文件根本不存在一模一样**。三条都验证过会响。

### 这一族的形状

`manifest-matches-host`（type 写错 → 模组不被扫到，无报错）、
`build-prereqs`（没有 git 提交 → 90% 处报一条不像构建错误的错误）、
`host-loadable`（缺内存算子 → 构建全过，装载被拒）。

共同点：**编译期完全看不见，装载期才报，而报出来的话和构建过程毫无关系。**
这三条现在都有脚本守着。

---

## 用真实消费方（crossbind）校准

crossbind 是一个 15880 行的跨版本协议适配 mod，原本写在旧 loader 上。
把它迁到 Pier，**只改了三个文件** —— 另外两个 crate（`bedrock-codec` /
`bedrock-protocol`，共 15749 行）一行没碰，因为它们不认识任何宿主。

这次迁移的价值不在迁移本身，在**它替我决定了 85 个 ⬜ 里先写哪一个**。

### 迁移改的三处

| 文件 | 改了什么 |
|---|---|
| `crates/crossbind-mod/src/lib.rs` | `ctx.server()` → `ctx.host()`；`HookDirection` 拆成 `Direction`/`Directions` |
| `crates/crossbind-mod/Cargo.toml` | 依赖从已归档的 `levilamina-rust-loader`（那个 path 现在指向不存在的目录）换成 `pier-rs`；三个 feature 全删 |
| `manifest.json` | `"type": "rust"` → `"pier"`；依赖 `levilamina-rust-loader` → `pier`；`abi_version` 5 → 1 |

顺带发现 crossbind 有**两份 manifest**（根目录一份、crate 下一份），
版本号还不一致（0.1.2 vs 0.1.0）。只留根目录那份发布用的。

### 新写 `pier-rs/src/packet.rs`

一处 API 设计上的改动值得记：旧 SDK 用**一个** `HookDirection` 枚举兼任
「这个包是哪个方向」和「注册时要哪些方向」，于是 `Both` 在前一个位置上是个
说不通的取值 —— 而它在 `match` 里必须被处理，只能写成 `_ =>`，
**把真正的遗漏也一起吞了**。拆成 `Direction`（两个取值）和 `Directions`
（三个取值）之后，crossbind 那个 match 变成穷尽的。

闭包约束也从 `Send` 收紧到 `Send + Sync`：ABI 明写着入站回调跑在连接被抽水
的线程上、出站跑在发起发送的线程上，**同一个闭包可能被多线程同时进入**。
旧签名让这件事看不见。

### crossbind 把 `rust-surrogate` 的四个缺陷全暴露了

它的代码风格比 Pier 自己的宽，于是一次报出 12 条，**全是误报**：

| 缺陷 | 根因 |
|---|---|
| 括号不配平（4 个文件） | `r#*"..."#*` 正则的 `r` 没有词边界，把 `Formatter"..."` 这种跨了两个字符串的片段当成一个 raw string 整段吞掉 —— **剥离器自己破坏了配平**（原文 123/123，剥完 98/100） |
| 同上 | 字符字面量正则把 `Formatter<'_>) -> Result {` 里的生命周期当成字符串开头 |
| `signed` 被报成 C 类型 | 判据是「冒号右边出现过这个词」，而 `signed` 是个**变量名** |
| `use ... Codec` 报未使用 | trait 被 use 进来的唯一目的常常就是让 `x.method()` 能解析，trait 名一次都不出现 |
| `pub const V: &[T] = &[...]` 报缺分号 | 只找 `{` 不找 `[`；改成找 `[` 之后又撞上**类型注解里的** `&[T]`，必须从 `=` 之后开始找 |

五条根因各不相同，但形状还是那一个：**判据看的东西，和它想断言的东西，
不是同一个东西。**

拿一个**风格不同的真实项目**去跑检查，是校准它的唯一办法 —— 只在自己的
代码上跑，测的是「它和我的习惯合不合」，不是「它对不对」。

## 合成事件补齐（对照 LeviLamina 事件注册表与 iListenAttentively 事件表）

对照两边的事件清单，把 Pier 缺的、且**在保护/经济场景里真正会被用到**的补上。
每一条都钉在一个已核对过签名的引擎符号上；取消语义一律使用引擎自己的失败
路径（返回 false / nullptr / NotPossibleHere），不制造半更新状态。

| 文件 | 事件 |
|---|---|
| `packages/pier-hooks/src/world/BlockDestroyEvent.cpp` | `BlockDestroyEvent` —— 钩 `Level::$destroyBlock`，覆盖末影人/凋灵/爆炸/命令等**非玩家**破坏 |
| `packages/pier-hooks/src/world/ExplosionEvent.cpp` | `ExplosionEvent` —— 钩 `Level::$explode`（High 优先级，位于维度规则之外），带来源实体与半径 |
| `packages/pier-hooks/src/world/LiquidFlowEvent.cpp` | `LiquidFlowEvent` —— 钩 `LiquidBlock::_trySpreadTo`，拦「水流进邻居地皮」 |
| `packages/pier-hooks/src/world/FarmlandDecayEvent.cpp` | `FarmlandDecayEvent` —— 钩 `FarmBlock::$transformOnFall`，拦踩坏耕地 |
| `packages/pier-hooks/src/world/PistonPushEvent.cpp` | `PistonPushEvent` —— 钩 `PistonBlockActor::_checkAttachedBlocks`，带附着方块列表 |
| `packages/pier-hooks/src/world/ChestPairEvent.cpp` | `ChestPairEvent` —— 钩 `ChestBlockActor::_tryToPairWith`，拦跨地皮大箱子 |
| `packages/pier-hooks/src/world/SpawnItemActorEvent.cpp` | `SpawnItemActorEvent` —— 钩 `Spawner::$spawnItem`，反刷物/掉落归属 |
| `packages/pier-hooks/src/world/WeatherChangeEvent.cpp` | `WeatherChangeEvent` —— 钩 `Level::$updateWeather`（只观察） |
| `packages/pier-hooks/src/player/SleepEvent.cpp` | `PlayerSleepEvent` —— 钩 `Player::$startSleepInBed`，取消走 `BedSleepingResult::NotPossibleHere` |
| `packages/pier-hooks/src/player/SlotChangeEvent.cpp` | `PlayerChangeSlotEvent` —— 钩 `Player::setSelectedSlot`（只观察） |
| `packages/pier-hooks/src/player/EatEvent.cpp` | `PlayerUseItemCompleteEvent` —— 钩 `Player::completeUsingItem`（只观察） |
| `packages/pier-hooks/src/protect/ArmorStandEvent.cpp` | `ArmorStandSwapItemEvent` —— 钩 `ArmorStand::_trySwapItem`，护住盔甲架上的装备 |
| `packages/pier-hooks/src/protect/ItemFrameEvent.cpp` | `PlayerAttackItemFrameEvent` —— 钩 `ItemFrameBlock::$attack`，护住展示框里的物品 |
| `packages/pier-hooks/src/protect/RideEvent.cpp` | 追加 `ActorRideEvent`（非玩家乘客，与 `PlayerRideEvent` 共用 detour） |
| `packages/pier-hooks/src/protect/PressurePlateEvent.cpp` | 追加 `ActorStepOnPressurePlateEvent`（非玩家实体，独立节流表） |

## Rust SDK 域封装补齐（第 1 批：NBT 地基）

上一代的安全封装覆盖了 190 个槽里的 188 个，**槽位覆盖不是问题，手感才是**。
业务侧为了从事件载荷里读一个字段，自己写了 695 行胶水（payload / hit /
wiring / dispatch），而且满屏 `unwrap_or(0)` —— 「这个键没有」和「值是 0」
被压成同一个答案，正是契约 §5.1 记载的那次土地保护绕过的形状。

所以这一批先补地基：跨边界的一切结构化数据都是 SNBT，取值手感由它决定。

| 文件 | 内容 |
|---|---|
| `bindings/rust/pier-rs/src/nbt/mod.rs` | `NbtValue` 值类型；**两套取值**：`opt_*`（Option，我自己兜底）与 `get_*`（Result，缺键与类型不符是不同的错、且带键名）；数组下标路径 `a.b[2].c`；`as_vec3`/`as_block_pos`；`From` 构造助手；`serde_json` 双向桥 |
| `bindings/rust/pier-rs/src/nbt/parse.rs` | SNBT 解析：类型后缀（`b/s/L/f/d`）、裸键与裸字符串、`true`/`false`、类型化数组 `[B;…]`/`[I;…]`/`[L;…]`、`\uXXXX` 转义、多字节 UTF-8；错误带字节偏移 |
| `bindings/rust/pier-rs/src/nbt/write.rs` | SNBT 写出：类型后缀一个不省、键一律加引号、控制字符写 `\uXXXX`、非有限浮点落 0 |

## Rust SDK 域封装补齐（第 2 批：事件与服务）

这一批把业务侧那 695 行胶水收进 SDK，并且顺着它暴露的问题重做了取值口径。

| 文件 | 内容 |
|---|---|
| `bindings/rust/pier-rs/src/event/mod.rs` | `Event`（类型化取值、`_unresolved` 感知的 `dim()`、`player()` 三形状归一、差量 `edit`/`cancel`）、RAII `Listener`（退订失败不静默）、`Wiring` 批量订阅 builder（`arm()` 整体失败即撤回 / `arm_lenient()`） |
| `bindings/rust/pier-rs/src/event/names.rs` | 事件 id 常量：LL 注册表事件 + **全部 29 个合成事件**，每条注明可否取消与载荷字段；`ALL_SYNTHETIC` 便于启动自检 |
| `bindings/rust/pier-rs/src/service.rs` | `call` / `call_json::<T>` / `call_with` / `call_optional`；`CallError` 分类（NotFound / Provider / Refused / Decode / Unavailable）；`register` / `register_json`；`exists` 改为真解析（上一代是 JSON 文本子串匹配） |
| `bindings/rust/pier-rs/src/sel.rs` | `PlayerSel` 枚举取代裸 `kind: i32`；把「`Name` 会走显示名回退、不能当身份」写进类型层（`is_stable()`） |

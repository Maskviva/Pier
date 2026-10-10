# Zig：服务器、刻与系统信息

## 函数 {#functions}

### `props.srvBdsVersion` {#props.srvBdsVersion}

```zig
pub fn srvBdsVersion(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SRV_BDS_VERSION`：取自 `Common::getGameVersionString`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`server_info_str`](../cpp/server.md#server_info_str)

### `props.srvProtocolVersion` {#props.srvProtocolVersion}

```zig
pub fn srvProtocolVersion(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SRV_PROTOCOL_VERSION`：`LevelData::mNetworkVersion`：最后一次写这个存档时用的协议版本。它说明存档有多旧，和服务器现在使用的协议无关。

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`server_info_str`](../cpp/server.md#server_info_str)

### `props.srvLevelProtocolVersion` {#props.srvLevelProtocolVersion}

```zig
pub fn srvLevelProtocolVersion(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SRV_LEVEL_PROTOCOL_VERSION`：正在运行的构建的版本号 `"major.minor.patch"`，取自 `CurrentGameSemVersion`。模组可以据此自带一张版本到协议的对照表，不必等 Pier 发新版。

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`server_info_str`](../cpp/server.md#server_info_str)

### `props.srvGameSemVersion` {#props.srvGameSemVersion}

```zig
pub fn srvGameSemVersion(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SRV_GAME_SEM_VERSION`：`PIER_SRV_GAME_SEM_VERSION`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`server_info_str`](../cpp/server.md#server_info_str)

### `props.sysOsName` {#props.sysOsName}

```zig
pub fn sysOsName(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SYS_OS_NAME`：取自 `sys_utils::getSystemName`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`sys_info_str`](../cpp/server.md#sys_info_str)

### `props.sysOsVersion` {#props.sysOsVersion}

```zig
pub fn sysOsVersion(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SYS_OS_VERSION`：取自 `sys_utils::getSystemVersion`，返回字符串

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`sys_info_str`](../cpp/server.md#sys_info_str)

### `props.sysLocale` {#props.sysLocale}

```zig
pub fn sysLocale(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SYS_LOCALE`：取自 `sys_utils::getSystemLocaleCode`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`sys_info_str`](../cpp/server.md#sys_info_str)

### `props.sysLocalTime` {#props.sysLocalTime}

```zig
pub fn sysLocalTime(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SYS_LOCAL_TIME`：取自 `sys_utils::getLocalTime`，返回 SNBT `{year,month,day,hour,minute,second,ms}`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`sys_info_str`](../cpp/server.md#sys_info_str)

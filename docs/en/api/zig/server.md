# Zig: Server, ticks and system information

## Functions {#functions}

### `props.srvBdsVersion` {#props.srvBdsVersion}

```zig
pub fn srvBdsVersion(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SRV_BDS_VERSION`: `Common::getGameVersionString`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`server_info_str`](../cpp/server.md#server_info_str)

### `props.srvProtocolVersion` {#props.srvProtocolVersion}

```zig
pub fn srvProtocolVersion(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SRV_PROTOCOL_VERSION`: `LevelData::mNetworkVersion`: the protocol the level was last written by. Says how old the save is, not what the server speaks.

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`server_info_str`](../cpp/server.md#server_info_str)

### `props.srvLevelProtocolVersion` {#props.srvLevelProtocolVersion}

```zig
pub fn srvLevelProtocolVersion(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SRV_LEVEL_PROTOCOL_VERSION`: "major.minor.patch" of the running build, from CurrentGameSemVersion. Lets a mod carry its own version→protocol table instead of waiting for a Pier release.

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`server_info_str`](../cpp/server.md#server_info_str)

### `props.srvGameSemVersion` {#props.srvGameSemVersion}

```zig
pub fn srvGameSemVersion(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SRV_GAME_SEM_VERSION`: `PIER_SRV_GAME_SEM_VERSION`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`server_info_str`](../cpp/server.md#server_info_str)

### `props.sysOsName` {#props.sysOsName}

```zig
pub fn sysOsName(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SYS_OS_NAME`: `sys_utils::getSystemName`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`sys_info_str`](../cpp/server.md#sys_info_str)

### `props.sysOsVersion` {#props.sysOsVersion}

```zig
pub fn sysOsVersion(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SYS_OS_VERSION`: `sys_utils::getSystemVersion` → string

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`sys_info_str`](../cpp/server.md#sys_info_str)

### `props.sysLocale` {#props.sysLocale}

```zig
pub fn sysLocale(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SYS_LOCALE`: `sys_utils::getSystemLocaleCode`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`sys_info_str`](../cpp/server.md#sys_info_str)

### `props.sysLocalTime` {#props.sysLocalTime}

```zig
pub fn sysLocalTime(allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_SYS_LOCAL_TIME`: `sys_utils::getLocalTime` → SNBT {year,month,day,hour,minute,second,ms}

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`sys_info_str`](../cpp/server.md#sys_info_str)

# 💾 数据存储与 NBT API

想把数据存下来，重启服务器以后还在？用键值数据库，键和值都是字符串。事件内容、物品、实体快照都是 SNBT 文本，三种语言都带了解析器，规则完全一样。

### 键值数据库

#### 打开数据库

Rust：`KvDb::open(path)`  
Go：`levilamina.OpenKvDb(path, create)`  
Zig：`levilamina.slot("kvdb_open")`  

- 参数：
    - path : 字符串  
      数据库名，放在你模组自己的数据目录下
    - create : 布尔  
      （Go、Zig）不存在时是否新建。Rust 的 `open` 会新建，`open_existing` 不会
- 返回值：数据库对象
- 返回值类型：Rust `Result<KvDb>`，Go `(*KvDb, error)`
- 对应槽位：`kvdb_open`
- 示例：
    - Rust

      ```rust title="Rust"
      let db = KvDb::open("homes")?;
      ```

    - Go

      ```go title="Go"
      db, err := levilamina.OpenKvDb("homes", true)
      defer db.Close()
      ```

    - Zig

      ```zig title="Zig"
      const open = levilamina.slot("kvdb_open") orelse return error.NotProvided;
      const db = open(levilamina.str("homes"), true);
      if (db == null) return error.Refused;
      ```

#### 读取一个键

Rust：`db.get(key)`  
Go：`db.Get(key)`  
Zig：`levilamina.slot("kvdb_get")`  

- 参数：
    - key : 字符串  
      键
- 返回值：键的值；键不存在时为「没有」，这不是错误
- 返回值类型：Rust `Option<String>`，Go `(string, bool, error)`
- 对应槽位：`kvdb_get`
- 示例：
    - Rust

      ```rust title="Rust"
      if let Some(home) = db.get("Steve") {
          // ...
      }
      ```

    - Go

      ```go title="Go"
      home, found, err := db.Get("Steve")
      ```

#### 写入一个键

Rust：`db.set(key, value)`  
Go：`db.Set(key, value)`  
Zig：`levilamina.slot("kvdb_set")`  

- 参数：
    - key : 字符串  
      键
    - value : 字符串  
      值；结构化的数据可以序列化成 JSON 或 SNBT 再存
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `bool`
- 对应槽位：`kvdb_set`
- 示例：
    - Rust

      ```rust title="Rust"
      db.set("Steve", "100,64,200")?;
      ```

    - Go

      ```go title="Go"
      err := db.Set("Steve", "100,64,200")
      ```

    - Zig

      ```zig title="Zig"
      const set = levilamina.slot("kvdb_set") orelse return error.NotProvided;
      _ = set(db, levilamina.str("Steve"), levilamina.str("100,64,200"));
      ```

#### 删除一个键

Rust：`db.del(key)`  
Go：`db.Delete(key)`  
Zig：`levilamina.slot("kvdb_del")`  

- 参数：
    - key : 字符串  
      键
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`
- 对应槽位：`kvdb_del`
- 示例：
    - Rust

      ```rust title="Rust"
      db.del("Steve")?;
      ```

    - Go

      ```go title="Go"
      err := db.Delete("Steve")
      ```

#### 遍历数据库

Rust：`db.iter()`  
Go：`db.Iter()`  
Zig：`levilamina.slot("kvdb_iter")`  

- 返回值：所有的键和值
- 返回值类型：Rust `Vec<(String, String)>`，Go `([]KeyValue, error)`
- 对应槽位：`kvdb_iter`
- 示例：
    - Rust

      ```rust title="Rust"
      for (key, value) in db.iter() {
          // ...
      }
      ```

    - Go

      ```go title="Go"
      all, err := db.Iter()
      ```

### SNBT

#### 解析 SNBT

Rust：`NbtValue::parse(text)`  
Go：`levilamina.ParseSNBT(text)`  
Zig：`levilamina.nbt.parse(arena, text)`  

- 参数：
    - text : 字符串  
      SNBT 文本
- 返回值：解析好的值，用点号路径读取字段：Rust `opt_str`、`opt_i32`，Go `OptString`、`OptInt`，Zig `getString`、`getInt`
- 返回值类型：Rust `Result<NbtValue, ParseError>`，Go `(*Nbt, error)`，Zig `nbt.ParseError!nbt.Value`
    - 不带后缀的整数是 int，太大就是 long，带小数点是 double；后缀 `b`、`s`、`L`、`f`、`d` 固定类型；`true`、`false` 是 byte。字段不存在或类型不对，返回「没有」。嵌套最多 512 层，再深会报错。
- 示例：
    - Rust

      ```rust title="Rust"
      let v = NbtValue::parse(r#"{name:"Steve",pos:{y:64}}"#)?;
      let y = v.opt_i32("pos.y");
      ```

    - Go

      ```go title="Go"
      v, err := levilamina.ParseSNBT(`{name:"Steve",pos:{y:64}}`)
      y, ok := v.OptInt("pos.y")
      ```

    - Zig

      ```zig title="Zig"
      const v = try levilamina.nbt.parse(arena, "{name:\"Steve\",pos:{y:64}}");
      const y = v.getInt("pos.y");
      ```

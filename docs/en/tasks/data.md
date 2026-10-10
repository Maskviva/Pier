# 💾 Data and NBT API

Want data that is still there after a restart? Use the key-value database; keys and values are strings. Event payloads, items and entity snapshots are SNBT text, and every language has a parser with the same rules.

### The key-value database

#### Opening a database

Rust: `KvDb::open(path)`  
Go: `levilamina.OpenKvDb(path, create)`  
Zig: `levilamina.slot("kvdb_open")`  

- Parameters:
    - path : string  
      the database's name, under your mod's data directory
    - create : boolean  
      (Go, Zig) whether to create it when missing. Rust's `open` creates, `open_existing` does not
- Return value: the database
- Return type: Rust `Result<KvDb>`, Go `(*KvDb, error)`
- Slot: `kvdb_open`
- Example:
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

#### Reading a key

Rust: `db.get(key)`  
Go: `db.Get(key)`  
Zig: `levilamina.slot("kvdb_get")`  

- Parameters:
    - key : string  
      the key
- Return value: the key's value; a missing key reads as absent, which is not an error
- Return type: Rust `Option<String>`, Go `(string, bool, error)`
- Slot: `kvdb_get`
- Example:
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

#### Writing a key

Rust: `db.set(key, value)`  
Go: `db.Set(key, value)`  
Zig: `levilamina.slot("kvdb_set")`  

- Parameters:
    - key : string  
      the key
    - value : string  
      the value; serialize structured data to JSON or SNBT first
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `bool`
- Slot: `kvdb_set`
- Example:
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

#### Deleting a key

Rust: `db.del(key)`  
Go: `db.Delete(key)`  
Zig: `levilamina.slot("kvdb_del")`  

- Parameters:
    - key : string  
      the key
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`
- Slot: `kvdb_del`
- Example:
    - Rust

      ```rust title="Rust"
      db.del("Steve")?;
      ```

    - Go

      ```go title="Go"
      err := db.Delete("Steve")
      ```

#### Every entry

Rust: `db.iter()`  
Go: `db.Iter()`  
Zig: `levilamina.slot("kvdb_iter")`  

- Return value: every key and value
- Return type: Rust `Vec<(String, String)>`, Go `([]KeyValue, error)`
- Slot: `kvdb_iter`
- Example:
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

#### Parsing SNBT

Rust: `NbtValue::parse(text)`  
Go: `levilamina.ParseSNBT(text)`  
Zig: `levilamina.nbt.parse(arena, text)`  

- Parameters:
    - text : string  
      the SNBT text
- Return value: the parsed value; read fields by dotted path: Rust `opt_str`, `opt_i32`, Go `OptString`, `OptInt`, Zig `getString`, `getInt`
- Return type: Rust `Result<NbtValue, ParseError>`, Go `(*Nbt, error)`, Zig `nbt.ParseError!nbt.Value`
    - An integer without a suffix is an int, a long when too big, a double with a decimal point; a suffix `b`, `s`, `L`, `f` or `d` fixes the type; `true` and `false` are bytes. A missing field or one of another type reads as absent. Nesting stops at 512 levels with an error.
- Example:
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

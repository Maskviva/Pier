# 📦 安装

这一页带你把 Pier 装到服务器上，再装上你的第一个模组。整个过程只要几分钟。

## 你需要准备什么

| | 版本 |
|---|---|
| 服务端 | 基岩版专用服务器（BDS）1.26.51 |
| 装载器 | LeviLamina 26.51.5 |
| 可选 | [LegacyMoney](https://github.com/LiteLDev/LegacyMoney)，用到经济功能时才需要 |

!!! tip "LegacyMoney 真的是可选的"

    没装 LegacyMoney，服务器照样正常启动。只是调用经济相关的 API 时会得到「不支持」的错误，其他功能完全不受影响。

## 第一步：安装 Pier

### 用 lip 安装（推荐）

[lip](https://lip.futrime.com) 是 LeviLamina 的包管理器，一行命令就装好了：

```bash
lip install github.com/Maskviva/Pier
```

### 手动安装

1. 从 [发布页](https://github.com/Maskviva/Pier/releases) 下载 `pier-server-windows-x64.zip`；
2. 解压到服务器的 `plugins/` 文件夹。

压缩包里只有一个 `Pier` 文件夹，解压完你会得到 `plugins/Pier/`，里面有 `manifest.json`、`Pier.dll` 和 `lang/`。

!!! warning "文件夹名必须是 Pier，大写的 P"

    LeviLamina 会拿文件夹名和 manifest 里的名字逐字比较。文件夹叫 `pier` 的话，日志里会出现
    `Mod name Pier do not match folder pier`，Pier 根本不会被加载。

    Windows 不会帮你把已有的 `pier` 改名成 `Pier`，请先删掉旧的文件夹再解压。

## 第二步：确认装好了

启动服务器。Pier 准备就绪后，日志里会出现这么一行：

```
[host] ready, ABI v2, api table 1600 bytes
```

看到 `ABI v2` 就对了。后面的字节数会随着 Pier 增加功能而变大，不用去核对。

然后在服务器控制台里输入：

```
/pier list
```

它会列出 Pier 加载了哪些模组。现在还没装模组，所以是空的。

## 第三步：装一个模组

Pier 模组和 Pier 本身一样，是 `plugins/` 下的一个文件夹，里面放着模组的 DLL 和 `manifest.json`：

```
plugins/
  Pier/
  my-mod/
    my_mod.dll
    manifest.json
```

放好以后重启服务器，再输入 `/pier list`，就能看到它了。

!!! warning "manifest 里的 type 必须是 “pier”"

    `manifest.json` 里写着 `"type": "pier"`，Pier 才会接管这个模组。写错了，模组不会被加载，**而且什么提示都没有**。
    `manifest.json` 里每个字段的含义，见 [manifest](manifest.md)。

## Pier 带来的命令

Pier 给服务器加了一条 `/pier` 命令：

| 命令 | 做什么 |
|---|---|
| `/pier list` | 列出 Pier 加载的模组 |
| `/pier events` | 列出所有能订阅的事件名，包括 Pier 自己的事件 |
| `/pier abi` | 显示 ABI 版本和函数表长度，报告兼容问题时用得到 |

!!! tip "订阅了事件却没反应？"

    先用 `/pier events` 查一下事件名有没有写对，这是最快的排查办法。

## 日志用什么语言

Pier 的日志支持多种语言，翻译文件放在 `plugins/Pier/lang/`。发行包里自带 `en_US.lang` 和 `zh_CN.lang`，中英文服务器什么都不用改。

!!! warning "安装时请整个文件夹一起复制"

    如果只复制了 `Pier.dll`，没带上 `lang/` 文件夹，中文服务器上打出来的就会是英文日志。
    英文是编译在程序里的兜底，所以不会出错，但你也找不到任何提示告诉你为什么。

用哪种语言由 `config.json` 里的 `language` 决定，默认是 `"auto"`，跟着游戏的语言走。想固定成某种语言，直接写死：

```json
{ "language": "zh_CN" }
```

`zh_CN` 和 `zh-CN` 两种写法都认。更多设置见 [配置](configuration.md)。

### 想自己改翻译？

- **改一句话**：改对应语言文件里那一行的值；
- **加一种语言**：把 `en_US.lang` 复制一份，改成新的语言码，然后翻译。没翻译的行会自动退回英文，所以翻到一半也能用；
- 翻译里的 `{}` 是占位符，Pier 会按顺序往里填内容。**个数要和 `en_US.lang` 那一行一致**，否则这一句会显示成 `[bad translation]`。顺序不会被检查，调换位置会让内容填错地方。

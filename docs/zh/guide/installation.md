# 安装

## 环境要求

| | |
|---|---|
| 服务端 | 基岩版专用服务器 1.26.32 |
| 装载器 | LeviLamina 26.40.0 |
| 可选 | [LegacyMoney](https://github.com/LiteLDev/LegacyMoney)，经济相关调用需要 |

LegacyMoney 是真的可选。Pier 对它走延迟加载，所以没装的服务器照常启动，
经济调用返回失败值，其余功能不受影响。

## 用 lip 装

[lip](https://lip.futrime.com) 是 LeviLamina 的包管理器。

```bash
lip install github.com/Maskviva/Pier
```

## 手动装

从 [发布页](https://github.com/Maskviva/pier/releases) 下载 `pier-windows-x64.zip`，
解压到 `plugins/pier/`。

## 确认装上了

启动服务器。宿主就绪后日志里有一行：

```
[host] ready, ABI v2, api table 1600 bytes
```

字节数是宿主编出来多少就是多少，每追加一个能力就会变大，所以看这行是为了确认 ABI 版本，
不是核对那个数字。

然后在控制台里：

```
/pier list
```

它列出 Pier 已装载的模组。装第一个模组之前是空的。

## Pier 给服务器加了什么

一条命令 `/pier`，有这几个子命令：

| | |
|---|---|
| `/pier list` | Pier 已装载的模组 |
| `/pier events` | 宿主能解析的全部事件 id，含合成事件 |
| `/pier abi` | ABI 版本与表长度，报兼容性问题时需要 |

订阅不触发的时候，`/pier events` 是找事件 id 最快的办法。

## 装一个模组

Pier 模组是 `mods/` 下的一个目录，里面放着 DLL 和 `manifest.json`：

```
mods/
  my-mod/
    my_mod.dll
    manifest.json
```

那个文件里写什么见 [manifest](/zh/guide/manifest)。唯一值得多看一眼的字段是
`"type": "pier"`——写错的话模组根本不会被扫到，而且什么都不报。

## 日志的语言

Pier 读 `plugins/pier/lang/<语言码>.lang`，**只读不写**。这个目录随发行包一起发，里面是
`en_US.lang` 和 `zh_CN.lang`，这两种语言的服务器不用动任何东西。

**安装时整个 mod 目录一起拷。** 只拷 dll 会把 lang 目录落下，症状是翻译过的服务器打出
英文日志——英文是编进去的兜底，所以什么都不会坏，也没有任何一句说得出为什么。

读哪一份由 `config.json` 的 `language` 决定，默认 `"auto"` 是跟引擎报出的语言走。

服主读的语言和服务器跑的语言不一样时，直接写死：

```json
{ "language": "zh_CN" }
```

`zh_CN` 和 `zh-CN` 都认。启动那一行会说这个码是配置定的还是引擎报的；指定了一个没有对应
文件的码，还会单独出一行警告。见[配置](./configuration)。

改一句话就改对应语言那个文件里的值。加一种语言就把 `en_US.lang` 复制成新的语言码再翻译。
没翻的键退回英文，所以翻到一半的文件照样能用。

`{}` 是宿主按位置填进来的占位符。一行的占位符个数和 `en_US.lang` 里那一行不一样时，
这一句会打印成 `[bad translation]`；顺序完全不检查，调换位置会让参数落错槽。


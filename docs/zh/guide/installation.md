# 安装

## 环境要求

| | |
|---|---|
| 服务端 | 基岩版专用服务器 1.26.20 |
| 装载器 | LeviLamina 26.20.4 |
| 可选 | [LegacyMoney](https://github.com/LiteLDev/LegacyMoney)，经济相关调用需要 |

LegacyMoney 是真的可选。Pier 对它走延迟加载，所以没装的服务器照常启动，
经济调用返回失败值，其余功能不受影响。

## 用 lip 装

[lip](https://lip.futrime.com) 是 LeviLamina 的包管理器。

```bash
lip install github.com/Maskviva/pier
```

## 手动装

从 [发布页](https://github.com/Maskviva/pier/releases) 下载 `pier-windows-x64.zip`，
解压到 `plugins/pier/`。

## 确认装上了

启动服务器。宿主就绪后日志里有一行：

```
[host] ready, ABI v1, api table 1560 bytes
```

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

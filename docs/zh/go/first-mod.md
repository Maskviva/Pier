# 第一个 Go 模组

这里构建的是 `examples/hello-pier-go`：一个命令、一个玩家加入事件的监听器，以及一个在模组启用一秒后运行的任务。

## 模块

模组是一个普通的 Go 模块，main 包被编译成 DLL：

```
go.mod
main.go
manifest.json
```

`go.mod` 依赖绑定。在 Pier 仓库里，示例用 `replace` 指向它；你自己的模组按版本依赖即可。

```
module hello-pier-go

go 1.21

require github.com/Maskviva/pier/bindings/go v0.0.0

replace github.com/Maskviva/pier/bindings/go => ../../bindings/go
```

## main.go

```go
package main

import (
	"time"

	"github.com/Maskviva/pier/bindings/go/levilamina"
)

type hello struct {
	joins *levilamina.Listener
}

func (h *hello) Enable(ctx *levilamina.Context) error {
	log := ctx.Logger()
	joins, err := levilamina.Subscribe("ll::event::PlayerJoinEvent", levilamina.PriorityNormal, func(ev *levilamina.Event) {
		log.Info("a player joined: " + ev.SNBT)
	})
	if err != nil {
		return err
	}
	h.joins = joins

	err = levilamina.RegisterCommand("hellogo", "Says hello from a Go mod.", levilamina.PermissionAny, func(inv *levilamina.Invocation) {
		inv.Success("hello from a Go mod, " + inv.Origin)
	})
	if err != nil {
		log.Warn(err.Error())
	}

	_, err = levilamina.ScheduleAfter(time.Second, func() { log.Info("one second later") })
	return err
}

func (h *hello) Disable(ctx *levilamina.Context) error {
	return h.joins.Unsubscribe()
}

func init() {
	levilamina.Register("hello-pier-go", &hello{})
}

func main() {}
```

`Register` 写在 `init` 里，因为宿主调用 `pier_main` 时，Go 运行时已经跑完了所有 `init`，而其他代码都还没运行。
`main` 是这种构建模式要求的，永远不会运行。

命令注册失败时只记一条警告，启用照常继续：模组的其他功能用不到这条命令，日志里那一行写着哪条命令没注册上、为什么。

## 构建

```
set CGO_ENABLED=1
go build -buildmode=c-shared -trimpath -o hello_pier_go.dll .
```

示例里的 `build.bat` 做的是同样的事，并且在 `PATH` 上找不到 Go 或 gcc 时说明缺了什么。

## 安装

`manifest.json` 写明 DLL 名，并把模组声明为 `pier` 类型：

```json
{
    "name": "hello-pier-go",
    "entry": "hello_pier_go.dll",
    "type": "pier",
    "version": "0.1.0",
    "dependencies": [{ "name": "pier" }]
}
```

把这两个文件放进 `plugins/hello-pier-go/` 再启动服务器。日志会显示模组加载、启用，一秒后出现任务打印的那一行。
`/hellogo` 在游戏里和控制台都能用。

如果模组没有加载，[排错](../guide/troubleshooting.md)列出了各条日志的含义。

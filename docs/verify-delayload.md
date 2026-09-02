# 验证 LegacyMoney 真的变成可选了

改完 `xmake.lua` 之后 **不要只看构建有没有成功** —— `/DELAYLOAD` 写错通道时
构建照常成功，产物照样带硬性依赖，症状和没改一模一样。这份文档给的是能直接
看到答案的验证方式，按可靠性从高到低排。

---

## 一、看导入表（最直接，一条命令）

在 VS 开发者命令提示符里：

```
dumpbin /imports pier.dll | findstr /i legacymoney
```

**改对了**会看到 `LegacyMoney.dll` 出现在 *delay load imports* 那一节：

```
    Section contains the following delay load imports:

      LegacyMoney.dll
                40C0A0 Import Address Table
                ...
```

**没改对**它出现在普通的 imports 一节里（前面没有 "delay load" 字样）。

只想要一个是非答案：

```
dumpbin /dependents pier.dll | findstr /i legacymoney
```

改对之后这条命令 **不该有输出** —— `/dependents` 只列硬性依赖。有输出就说明
标志没生效。

---

## 二、看链接命令行（确认 xmake 真的把标志传下去了）

```
xmake f -c -y
xmake -v 2>&1 | findstr /i DELAYLOAD
```

`-v` 会打印真实的 link.exe 命令行。**看不到 `/DELAYLOAD:LegacyMoney.dll` 就说明
这一行没被 xmake 采纳**，那时问题在构建脚本，不在链接器。

这一步能把「标志没传下去」和「传下去了但不管用」分开 —— 上一轮我把它们混
在一起猜，猜错了。

---

## 三、最终验收（把 LegacyMoney 挪走再启动）

1. 把 `plugins/LegacyMoney/` 整个目录移到别处
2. 启动 BDS

**期望**：pier 正常加载，启动日志里有一条

```
[Pier] 模组列表里没有已启用的 LegacyMoney —— 经济功能不可用：
       读取返回 0、写入返回失败。请检查是否安装并启用了 LegacyMoney
```

之后每个经济入口返回失败值（`get_money` 返回 -1，写入返回 false），rsw 那边
依赖经济的功能入口自然关闭，其余功能不受影响。

**不该出现**：`0x7E The specified module could not be found`。

---

## 如果第一步就不对，接下来查什么

按可能性排序：

1. **`xmake f -c` 没重跑。** xmake 会缓存配置，只 `xmake` 不会重新生成链接
   命令行。先 `xmake f -c -y` 再 `xmake`。

2. **`@levibuildscript/linkrule` 覆盖了 flags。** 这条规则接管了链接过程，
   有可能它自己拼命令行而不读 target 的 shflags。验证方式是第二步的 `-v`：
   如果 shflags 里有而命令行里没有，就是它吃掉了。那时的解法是把标志加进
   规则认的那个入口，或者在 `after_link` 里改。

3. **依赖是从别的 target 传进来的。** `pier-api` 也 `add_packages("legacymoney")`。
   它是 `object` kind，本身不链接，但如果 levibuildscript 把子 target 的包
   依赖汇总到最终链接，标志要加在汇总的那一层。

4. **`delayimp` 没链上。** 那时链接器会报 `__delayLoadHelper2` 未定义 ——
   这是显式错误，不会静默，所以如果构建成功了就不是这条。

第 2 条是我最怀疑的一条，而我在这边没有 xmake 和 levibuildscript，验证不了。
第二步那条 `xmake -v` 能一次性把它排除掉。

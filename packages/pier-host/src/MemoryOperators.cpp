/**
 * MemoryOperators.cpp —— 让本模组使用 LeviLamina 的统一内存分配算子。
 *
 * # 没有这个 TU 会怎样
 *
 * LeviLamina 在装载时检查每个原生模组是否用了统一算子，不用就**直接拒绝
 * 装载**：
 *
 *     ERROR [LeviLamina] 无法加载 Pier
 *     ERROR [LeviLamina] Pier 将不会被加载因为没有使用统一的内存分配操作符。
 *
 * 注意这条错误出现在**构建全部成功之后** —— 98 个 TU 编过、prelink 过、
 * pier.dll 链好、mod 打包完成，装的时候才报。它和编译期的任何检查都无关。
 *
 * # 为什么必须统一
 *
 * BDS、LeviLamina、各个模组分别链接自己的 CRT。一块内存在 A 的堆上分配、
 * 在 B 的堆上释放，就是堆损坏 —— 而 Pier 的整个存在意义就是让内存和对象
 * 跨这些边界流动（事件载荷、命令输出、跨模组服务的回复）。契约 §三 规定
 * 「谁产出谁释放」是**接口层**的纪律；这一行 `#define` 是它在**分配器层**
 * 的前提：两条纪律缺任何一条，跨边界传数据都不安全。
 *
 * # 它必须在最终产物里
 *
 * 这里定义的是全局 `operator new` / `operator delete`。它们只有真正被链接
 * 进 `pier.dll` 才起作用 —— 而这正是契约 §一 规则四（所有包
 * `set_kind("object")`）保护的东西：静态库里一个「没有外部符号引用」的
 * TU 会被链接器整个丢掉，而这个 TU 恰好没有任何人显式引用它。
 *
 * 放在 `pier-host` 而不是 `pier-support`：分配器是**进程级**的选择，属于
 * 宿主本体，不属于某个能被单独复用的工具层。
 */

// 必须在 include 之前。这个宏是那个头里「定义算子」和「只声明」之间的开关。
#define LL_MEMORY_OPERATORS

#include "ll/api/memory/MemoryOperators.h" // IWYU pragma: keep

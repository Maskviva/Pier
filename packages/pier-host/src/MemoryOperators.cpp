/**
 * MemoryOperators.cpp —— 让本模组使用 LeviLamina 的统一内存分配算子。
 *
 * 没有这个 TU，LeviLamina 在装载时会直接拒绝加载 Pier，报「没有使用统一的内存分配
 * 操作符」。这条错误出现在构建全部成功之后（编译、prelink、链接、打包都过了），和
 * 编译期的任何检查都无关。
 *
 * BDS、LeviLamina、各个模组分别链接自己的 CRT，一块内存在 A 的堆上分配、在 B 的堆
 * 上释放就是堆损坏，而 Pier 的存在意义就是让内存和对象跨这些边界流动（事件载荷、命
 * 令输出、跨模组服务的回复）。契约 §三 的「谁产出谁释放」是接口层纪律，这一行
 * #define 是它在分配器层的前提，两者缺一跨边界传数据都不安全。
 *
 * 它必须真的被链接进 pier.dll 才起作用，而这个 TU 没有任何人显式引用它 —— 那正是契
 * 约 §一 规则四（所有包 set_kind("object")）保护的东西。放在 pier-host 而不是
 * pier-support：分配器是进程级的选择，属于宿主本体。
 */

// 必须在 include 之前。这个宏是那个头里「定义算子」和「只声明」之间的开关。
#define LL_MEMORY_OPERATORS

#include "ll/api/memory/MemoryOperators.h" // IWYU pragma: keep

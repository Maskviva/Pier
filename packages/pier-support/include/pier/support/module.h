#pragma once
// 函数地址 → 所属模块的归属判定。
//
// 住在 support 而不是某个域里，因为它是「遗留形状」的通用解药：早于
// mod-scoped 约定的槽位（money 事件监听器是现存的一例）只收裸函数指针、
// 不带模组句柄 —— 宿主不知道回调属于谁，卸载时清不掉。从地址反查模块
// 是恢复归属的唯一办法，而将来任何背同样历史包袱的槽位都会需要它。
#include <cstddef>

namespace pier
{
    /** `fn` 是否落在基址为 `moduleBase` 的已加载模块里。
     *  非 Windows 平台恒 false（BDS 只有 Windows 目标；返回 false 让调用
     *  方按「查不出归属」保守处理，而不是假装查出来了）。 */
    [[nodiscard]] bool addressOwnedBy(void const* moduleBase, void const* fn) noexcept;
} // namespace pier

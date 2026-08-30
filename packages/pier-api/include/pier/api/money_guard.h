/**
 * pier/api/money_guard.h —— 把 LLMoney 后端做成可选。
 *
 * 经济 API 背靠 LegacyMoney 导出的 `LLMoney_*` 函数。它们住在
 * `LegacyMoney.dll` 里，而那个 DLL 是**延迟加载**的（见 xmake 的
 * `/DELAYLOAD:LegacyMoney.dll`）：LegacyMoney 没装时宿主照常启动，每个
 * `LLMoney_*` 符号到第一次使用才解析。
 *
 * moneyBackendReady() 在任何经济调用派发之前做契约要求的**双重**校验：
 *
 *   1. 模组表检查 —— ll::mod::ModManagerRegistry 里有名为 "LegacyMoney"
 *      的模组**且**处于 Enabled 状态。装了但被禁用的 LegacyMoney 仍导出
 *      符号，但调进一个被禁用的模组是逻辑错误，同样按「未就绪」处理。
 *
 *   2. 符号检查 —— ll::memory::SymbolView::resolve() 真能找到
 *      `LLMoney_Get` 导出。这兜住模组表兜不住的病态情形：过期/改名的
 *      DLL、换了导出的版本、或一个永远没有真目标的延迟加载桩。探测一个
 *      稳定符号（LLMoney_Get）一次并缓存 —— 核心 getter 都没有的话整族
 *      都用不了。
 *
 * 第一次失败时告警**一次**（每进程），文案可执行；之后每个经济入口返回
 * 安全默认值。永不把异常抛过 C ABI，永不让延迟加载失败硬崩 BDS。
 */
#pragma once

namespace pier::api_impl
{
    /**
     * 上述两查都过时为真。首调之后很便宜：拥有这些符号的 DLL 没法在会话
     * 中途换掉，所以符号探测做了 memoize；便宜得多的模组表/状态检查每次
     * 都跑，这样运行期禁用 LegacyMoney 能被反映出来。
     *
     * 「装上/启用 LegacyMoney」的告警每进程至多一次，在第一次校验失败时
     * 发出。必须在服务器线程上调（它碰模组注册表）。永不抛。
     */
    bool moneyBackendReady() noexcept;
} // namespace pier::api_impl

/**
 * pier/api/money_guard.h —— 把 LLMoney 后端做成可选。
 *
 * 经济 API 背靠 LegacyMoney 导出的 LLMoney_* 函数，它们住在延迟加载的
 * LegacyMoney.dll 里：没装时宿主照常启动，符号到第一次使用才解析。两半缺一
 * 不可（契约 §2.1）：这里的守卫，加上 xmake 的 /DELAYLOAD。
 *
 * moneyBackendReady() 在任何经济调用派发之前做契约要求的双重校验。一是模组表：
 * ll::mod::ModManagerRegistry 里有名为 "LegacyMoney" 的模组且处于 Enabled；装了但
 * 被禁用的仍导出符号，而调进一个被禁用的模组是逻辑错误，同样按未就绪处理。二是符
 * 号：ll::memory::SymbolView::resolve() 真能找到 LLMoney_Get 导出，兜住模组表兜不
 * 住的过期或改名 DLL、换了导出的版本、以及没有真目标的延迟加载桩。
 *
 * 第一次失败时每进程告警一次，之后每个经济入口返回安全默认值。永不把异常抛过
 * C ABI，永不让延迟加载失败硬崩 BDS。
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

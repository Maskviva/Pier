/**
 * pier/api/money_guard.h: makes the LLMoney backend optional.
 * The economy API rests on the LLMoney_* functions LegacyMoney exports, which live in
 * a delay-loaded LegacyMoney.dll. Without it installed the host still starts and the
 * symbols resolve on first use. Both halves are required (contract §2.1): the guard
 * here and /DELAYLOAD in xmake.
 * moneyBackendReady() runs both required checks before any economy call. First the
 * mod table, where ll::mod::ModManagerRegistry must hold a mod named "LegacyMoney" in
 * state Enabled, since an installed but disabled mod still exports the symbols and
 * calling into it is a logic error. Second the symbols, where
 * ll::memory::SymbolView::resolve() must find the LLMoney_Get export, covering a stale
 * or renamed DLL, a version with different exports, and a delay-load stub with no
 * target. The first failure warns once per process and every economy entry point then
 * returns a safe default. An exception is never thrown across the C ABI and a
 * delay-load failure never hard-crashes BDS.
 */
#pragma once

namespace pier::api_impl
{
    /**
     * True when both checks above pass. Cheap after the first call, because the DLL
     * owning those symbols cannot be swapped mid-session, so the symbol probe is
     * memoized. The far cheaper mod table and state check runs every time, so
     * disabling LegacyMoney at runtime is reflected.
     *
     * The warning asking for LegacyMoney to be installed or enabled is emitted at
     * most once per process, on the first failed check. Must be called on the server
     * thread, since it touches the mod registry. Never throws.
     */
    bool moneyBackendReady() noexcept;
} // namespace pier::api_impl

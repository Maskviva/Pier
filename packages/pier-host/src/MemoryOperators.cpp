/**
 * MemoryOperators.cpp: makes this mod use the unified LeviLamina allocation operators.
 *
 * Without this translation unit LeviLamina refuses to load Pier and reports that the
 * unified allocation operators are not in use. That error appears after the whole
 * build has succeeded, past compile, prelink, link and packaging, and relates to no
 * compile-time check.
 *
 * BDS, LeviLamina and each mod link their own CRT, so memory allocated on one heap and
 * freed on another is heap corruption, and Pier exists precisely to let memory cross
 * those boundaries in event payloads, command output and service replies. Contract §3
 * states the interface discipline; this #define is its precondition at the allocator
 * level. Nothing references this translation unit explicitly, which is what contract
 * §1 rule 4 protects by requiring set_kind("object"). It lives in pier-host because
 * the allocator is a process-wide choice.
 */

// Must come before the include. This macro is the switch in that header between
// defining the operators and only declaring them.
#define LL_MEMORY_OPERATORS

#include "ll/api/memory/MemoryOperators.h" // IWYU pragma: keep

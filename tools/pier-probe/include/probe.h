/**
 * probe.h — one place to record what a slot answered.
 *
 * The mod is a report, not a test suite. It calls a slot, writes down what came back,
 * and moves on; nothing here asserts, because on this engine version a slot answering
 * "no" is the expected result for a documented set of them and an assert would stop the
 * run at the first one.
 *
 * Four outcomes are kept apart because they need different actions from a reader:
 *   ABSENT   the slot pointer is NULL or past struct_size. The host was built without
 *            that package, or it is older than this mod. Nothing was called.
 *   OK       the call returned and reported success.
 *   REFUSED  the call returned and reported failure. On 26.32 this is the documented
 *            answer for the capabilities the engine no longer exposes, so it is a
 *            result and not an error.
 *   THREW    the call did not return normally. This is the only outcome that is always
 *            a defect, in the host or in this mod.
 */
#ifndef PIER_PROBE_H
#define PIER_PROBE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "sdk/abi.h"

namespace probe
{
    enum class Verdict
    {
        Absent,
        Ok,
        Refused,
        Threw
    };

    struct Row
    {
        std::string group;
        std::string slot;
        Verdict verdict;
        std::string note;
    };

    /** Everything recorded so far, in call order. */
    std::vector<Row>& rows();

    /** std::string and not char const*: several call sites build the slot label at the
     *  call, and taking a pointer would make each of them depend on the copy happening
     *  before the temporary dies. It does, but only because Row holds strings, which is
     *  the wrong thing for a caller to have to know. */
    void record(std::string group, std::string slot, Verdict v, std::string note = {});

    /** The host table and the mod handle, set once by pier_main. */
    PierApi const* api();
    PierModHandle self();
    void bind(PierApi const* a, PierModHandle h);

    /** True when the host table is long enough to contain the byte at `offset`.
     *
     *  This is the forward-compatibility rule of contract §2.2 and the reason the check
     *  is by offset rather than by pointer: an older host's table simply ends earlier,
     *  and reading the slot at all would be a read past the end of the allocation. */
    bool covers(std::size_t offset);

    void log(int level, std::string const& msg);

    /** Renders the report and logs it, then returns the counts by verdict. */
    struct Totals
    {
        int absent = 0;
        int ok = 0;
        int refused = 0;
        int threw = 0;
    };
    Totals report();

    /** Sink for a slot that answers once: the string replaces whatever was there.
     *
     *  A slot that calls its sink once per item needs `collect` below instead. Using this
     *  one there keeps only the last call, and a registry of any size is then reported as
     *  a single entry with nothing to say it was truncated. */
    void intoString(void* ctx, PierStr s);

    /** Counts the calls and keeps a sample, for a slot that answers once per item. */
    struct Collected
    {
        int count = 0;
        std::string first;
        std::string last;
    };

    void collect(void* ctx, PierStr s);

    /** "N items: first .. last", or "empty" when the sink was never called. */
    std::string describe(Collected const& c);

    std::string toStd(PierStr s);
    PierStr fromStd(std::string const& s);
} // namespace probe

/** Guards one call: absent slots are recorded without calling, and an escaping
 *  exception is recorded rather than allowed to reach the host. */
#define PIER_PROBE(group, slotname, expr)                                                          \
    do                                                                                             \
    {                                                                                              \
        if (!probe::covers(offsetof(PierApi, slotname) + sizeof(void*))                             \
            || probe::api()->slotname == nullptr)                                                  \
        {                                                                                          \
            probe::record(group, #slotname, probe::Verdict::Absent);                               \
            break;                                                                                 \
        }                                                                                          \
        try                                                                                        \
        {                                                                                          \
            expr;                                                                                  \
        }                                                                                          \
        catch (std::exception const& e)                                                            \
        {                                                                                          \
            probe::record(group, #slotname, probe::Verdict::Threw, e.what());                      \
        }                                                                                          \
        catch (...)                                                                                \
        {                                                                                          \
            probe::record(group, #slotname, probe::Verdict::Threw, "non-std exception");           \
        }                                                                                          \
    } while (false)

/** PIER_PROBE with the label given rather than taken from the slot name.
 *
 *  A slot probed several times with different arguments needs one row per argument, and
 *  the absent branch has to carry the same label as the branch that calls: six identical
 *  ABSENT rows say nothing about which six arguments were skipped. */
#define PIER_PROBE_AS(group, slotname, label, expr)                                                \
    do                                                                                             \
    {                                                                                              \
        if (!probe::covers(offsetof(PierApi, slotname) + sizeof(void*))                             \
            || probe::api()->slotname == nullptr)                                                  \
        {                                                                                          \
            probe::record(group, label, probe::Verdict::Absent);                                   \
            break;                                                                                 \
        }                                                                                          \
        try                                                                                        \
        {                                                                                          \
            expr;                                                                                  \
        }                                                                                          \
        catch (std::exception const& e)                                                            \
        {                                                                                          \
            probe::record(group, label, probe::Verdict::Threw, e.what());                          \
        }                                                                                          \
        catch (...)                                                                                \
        {                                                                                          \
            probe::record(group, label, probe::Verdict::Threw, "non-std exception");               \
        }                                                                                          \
    } while (false)

/** The common shape: call it, and a false return is Refused rather than a failure. */
#define PIER_PROBE_BOOL(group, slotname, call)                                                     \
    PIER_PROBE(group, slotname,                                                                    \
        probe::record(group, #slotname,                                                            \
                      (call) ? probe::Verdict::Ok : probe::Verdict::Refused))

#endif /* PIER_PROBE_H */

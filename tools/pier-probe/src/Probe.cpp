#include "probe.h"

#include <algorithm>
#include <cstdio>
#include <map>

namespace probe
{
    namespace
    {
        PierApi const* gApi = nullptr;
        PierModHandle gSelf = nullptr;
        std::vector<Row> gRows;

        char const* verdictName(Verdict v)
        {
            switch (v)
            {
            case Verdict::Absent:
                return "ABSENT ";
            case Verdict::Ok:
                return "OK     ";
            case Verdict::Refused:
                return "REFUSED";
            case Verdict::Threw:
                return "THREW  ";
            }
            return "?      ";
        }
    } // namespace

    std::vector<Row>& rows() { return gRows; }

    void bind(PierApi const* a, PierModHandle h)
    {
        gApi = a;
        gSelf = h;
    }

    PierApi const* api() { return gApi; }
    PierModHandle self() { return gSelf; }

    bool covers(std::size_t offset)
    {
        return gApi != nullptr && gApi->struct_size >= offset;
    }

    void record(std::string group, std::string slot, Verdict v, std::string note)
    {
        gRows.push_back(Row{std::move(group), std::move(slot), v, std::move(note)});
    }

    void log(int level, std::string const& msg)
    {
        if (gApi == nullptr || gApi->log == nullptr) return;
        gApi->log(gSelf, level, fromStd(msg));
    }

    void intoString(void* ctx, PierStr s) { *static_cast<std::string*>(ctx) = toStd(s); }

    void collect(void* ctx, PierStr s)
    {
        auto* c = static_cast<Collected*>(ctx);
        c->count++;
        c->last = toStd(s);
        if (c->count == 1) c->first = c->last;
    }

    std::string describe(Collected const& c)
    {
        if (c.count == 0) return "empty";
        char b[64];
        std::snprintf(b, sizeof b, "%d item(s): ", c.count);
        std::string out = b;
        out += c.first;
        if (c.count > 1) out += " .. " + c.last;
        return out;
    }

    std::string toStd(PierStr s)
    {
        return s.ptr == nullptr ? std::string{} : std::string{s.ptr, s.len};
    }

    PierStr fromStd(std::string const& s) { return PierStr{s.data(), s.size()}; }

    Totals report()
    {
        Totals t{};
        std::map<std::string, std::vector<Row const*>> byGroup;
        for (auto const& r : gRows)
        {
            byGroup[r.group].push_back(&r);
            switch (r.verdict)
            {
            case Verdict::Absent:
                t.absent++;
                break;
            case Verdict::Ok:
                t.ok++;
                break;
            case Verdict::Refused:
                t.refused++;
                break;
            case Verdict::Threw:
                t.threw++;
                break;
            }
        }

        log(3, "================ pier-probe ================");
        char head[256];
        std::snprintf(head, sizeof head,
                      "host table: struct_size=%u abi_version=%u host_flags=0x%x",
                      gApi ? gApi->struct_size : 0u, gApi ? gApi->abi_version : 0u,
                      gApi ? gApi->host_flags : 0u);
        log(3, head);

        for (auto const& [group, list] : byGroup)
        {
            log(3, "-- " + group);
            for (auto const* r : list)
            {
                std::string line = std::string{"   "} + verdictName(r->verdict) + "  " + r->slot;
                if (!r->note.empty()) line += "   " + r->note;
                // A slot that threw is the only line worth an error level: everything
                // else is a reading, and a reading at error level trains the reader to
                // ignore the level.
                log(r->verdict == Verdict::Threw ? 1 : 3, line);
            }
        }

        char tail[256];
        std::snprintf(tail, sizeof tail,
                      "%d probed: %d ok, %d refused, %d absent, %d threw",
                      static_cast<int>(gRows.size()), t.ok, t.refused, t.absent, t.threw);
        log(t.threw > 0 ? 1 : 3, tail);
        log(3, "============================================");
        return t;
    }
} // namespace probe

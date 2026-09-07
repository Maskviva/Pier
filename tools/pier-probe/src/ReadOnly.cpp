/**
 * ReadOnly.cpp — the slots that change nothing.
 *
 * Everything here can run on a live server with players on it. Each call reads a value
 * or lists something; none of them writes. That is what makes this file the one worth
 * running first: it establishes which capabilities the host actually has before
 * anything else touches the world.
 *
 * The values are logged rather than checked. A tick count of 0 is not wrong on a server
 * that has just started, and a seed this mod has no way to predict cannot be asserted
 * against anything. What is worth reporting is the difference between "answered" and
 * "refused", which is exactly what 26.32 changed for a documented set of slots.
 */
#include <cstdio>
#include <string>

#include "probe.h"

namespace
{
    constexpr char const* kGroup = "read-only";

    /** Renders a number into the note column. */
    std::string num(double v)
    {
        char b[64];
        std::snprintf(b, sizeof b, "%.3f", v);
        return b;
    }

    std::string num(long long v)
    {
        char b[32];
        std::snprintf(b, sizeof b, "%lld", v);
        return b;
    }

    /** First 120 characters of a listing, so one dimension list does not fill the log. */
    std::string head(std::string const& s)
    {
        if (s.size() <= 120) return s;
        return s.substr(0, 120) + "... (" + num(static_cast<long long>(s.size())) + " bytes)";
    }
} // namespace

void probeReadOnly()
{
    auto const* a = probe::api();

    // 0=Default 1=Starting 2=Running 3=Stopping. This runs from on_enable, which the
    // host calls while the server is still Starting, so 1 is the expected reading and a
    // slot that behaves differently once Running would not be caught here.
    PIER_PROBE(kGroup, gaming_status,
        {
            int32_t const st = a->gaming_status();
            char const* name = st == 0 ? "Default" : st == 1 ? "Starting"
                             : st == 2 ? "Running" : st == 3 ? "Stopping" : "?";
            probe::record(kGroup, "gaming_status", probe::Verdict::Ok,
                          std::string{name} + " (" + num(static_cast<long long>(st)) + ")");
        });

    PIER_PROBE(kGroup, get_current_tick,
        probe::record(kGroup, "get_current_tick", probe::Verdict::Ok,
                      "tick=" + num(static_cast<long long>(a->get_current_tick()))));

    PIER_PROBE(kGroup, get_tick_delta_time,
        probe::record(kGroup, "get_tick_delta_time", probe::Verdict::Ok,
                      num(a->get_tick_delta_time())));

    PIER_PROBE(kGroup, get_player_count,
        probe::record(kGroup, "get_player_count", probe::Verdict::Ok,
                      "players=" + num(static_cast<long long>(a->get_player_count()))));

    PIER_PROBE(kGroup, get_sim_paused,
        probe::record(kGroup, "get_sim_paused", probe::Verdict::Ok,
                      a->get_sim_paused() ? "paused" : "running"));

    PIER_PROBE(kGroup, sys_is_wine,
        probe::record(kGroup, "sys_is_wine", probe::Verdict::Ok,
                      a->sys_is_wine() ? "wine" : "native"));

    // A negative rate is the host saying the window has no samples yet, not a failure.
    // The probe runs a couple of seconds after start, so that is the usual answer and
    // saying so here keeps it from reading as a broken slot.
    PIER_PROBE(kGroup, get_tps,
        {
            double const v = a->get_tps(10);
            probe::record(kGroup, "get_tps", probe::Verdict::Ok,
                          v < 0 ? "10s window has no samples yet" : "10s=" + num(v));
        });

    PIER_PROBE(kGroup, get_mspt,
        {
            double const v = a->get_mspt(10);
            probe::record(kGroup, "get_mspt", probe::Verdict::Ok,
                          v < 0 ? "10s window has no samples yet" : "10s=" + num(v));
        });

    // Out-parameter reads. A false return is the host saying it does not know, which is
    // a legitimate answer before the level is up.
    PIER_PROBE(kGroup, get_time,
        {
            int64_t t = 0;
            bool const ok = a->get_time(&t);
            probe::record(kGroup, "get_time", ok ? probe::Verdict::Ok : probe::Verdict::Refused,
                          ok ? "time=" + num(static_cast<long long>(t)) : "");
        });

    PIER_PROBE(kGroup, get_difficulty,
        {
            int32_t d = 0;
            bool const ok = a->get_difficulty(&d);
            probe::record(kGroup, "get_difficulty", ok ? probe::Verdict::Ok : probe::Verdict::Refused,
                          ok ? "difficulty=" + num(static_cast<long long>(d)) : "");
        });

    PIER_PROBE(kGroup, get_seed,
        {
            int64_t seed = 0;
            bool const ok = a->get_seed(&seed);
            probe::record(kGroup, "get_seed", ok ? probe::Verdict::Ok : probe::Verdict::Refused,
                          ok ? "seed=" + num(static_cast<long long>(seed)) : "");
        });

    // Sink reads.
    PIER_PROBE(kGroup, list_players,
        {
            probe::Collected c;
            a->list_players(&c, &probe::collect);
            probe::record(kGroup, "list_players", probe::Verdict::Ok, probe::describe(c));
        });

    PIER_PROBE(kGroup, list_events,
        {
            probe::Collected c;
            a->list_events(&c, &probe::collect);
            probe::record(kGroup, "list_events", probe::Verdict::Ok, probe::describe(c));
        });

    PIER_PROBE(kGroup, md_list_dimensions,
        {
            probe::Collected c;
            a->md_list_dimensions(&c, &probe::collect);
            probe::record(kGroup, "md_list_dimensions", probe::Verdict::Ok, probe::describe(c));
        });

    // server_info_str: 0 is the BDS version string, 1 the network protocol. Item 1 is
    // the one that changed on 26.32, so both are probed by index rather than only the
    // first.
    for (int32_t prop = 0; prop <= 1; ++prop)
    {
        std::string const label = prop == 0 ? "server_info_str[0 bds]"
                                            : "server_info_str[1 protocol]";
        PIER_PROBE_AS(kGroup, server_info_str, label,
            {
                std::string out;
                bool const ok = a->server_info_str(prop, &out, &probe::intoString);
                probe::record(kGroup, label,
                              ok ? probe::Verdict::Ok : probe::Verdict::Refused, ok ? out : "");
            });
    }

    PIER_PROBE(kGroup, sys_info_str,
        {
            std::string out;
            bool const ok = a->sys_info_str(0, &out, &probe::intoString);
            probe::record(kGroup, "sys_info_str[0]",
                          ok ? probe::Verdict::Ok : probe::Verdict::Refused, ok ? head(out) : "");
        });

    PIER_PROBE(kGroup, sys_get_env,
        {
            std::string out;
            std::string const name{"PATH"};
            bool const ok = a->sys_get_env(probe::fromStd(name), &out, &probe::intoString);
            probe::record(kGroup, "sys_get_env", ok ? probe::Verdict::Ok : probe::Verdict::Refused,
                          ok ? "PATH is " + num(static_cast<long long>(out.size())) + " bytes" : "");
        });

    // Several names rather than one. A single refusal cannot tell "this rule does not
    // exist" from "the lookup is broken", and the lookup is one of the things 26.32
    // changed: the engine's own name-to-index call is inlined away and the host walks
    // GameRule::mName instead.
    //
    // Each rule appears twice, once lower case and once camel case, because the first
    // run of this probe answered only for `pvp`: a name with no case to get wrong. Both
    // spellings answering means the host folds case, one of them answering means it
    // matches the engine's spelling literally, and neither means the walk is broken.
    for (char const* rule : {"showcoordinates", "showCoordinates",
                             "dodaylightcycle", "doDaylightCycle",
                             "keepinventory", "commandblockoutput", "pvp"})
    {
        std::string const label = std::string{"game_rule_get["} + rule + "]";
        PIER_PROBE_AS(kGroup, game_rule_get, label,
            {
                std::string out;
                std::string const name{rule};
                bool const ok = a->game_rule_get(probe::fromStd(name), &out, &probe::intoString);
                probe::record(kGroup, label,
                              ok ? probe::Verdict::Ok : probe::Verdict::Refused, ok ? out : "");
            });
    }

    // list_actors and get_block need a dimension that exists. The overworld is 0 and is
    // the one dimension a running server always has.
    PIER_PROBE(kGroup, list_actors,
        {
            int count = 0;
            a->list_actors(0, &count,
                           [](void* c, PierActorId, PierStr) { ++*static_cast<int*>(c); });
            probe::record(kGroup, "list_actors", probe::Verdict::Ok,
                          "overworld actors=" + num(static_cast<long long>(count)));
        });
}

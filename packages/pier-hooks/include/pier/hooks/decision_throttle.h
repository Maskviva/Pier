/** pier/hooks/decision_throttle.h: a decision cache keyed by (player, place), for the synthetic
 * events of passive game behavior.
 * entityInside in PressurePlateEvent runs once per tick for every actor standing on the block,
 * and PushEntityEvent runs once per tick for every overlapping pair. Each dispatch assembles
 * SNBT, crosses the FFI, parses on the other side, queries a claim database and crosses back. At
 * 20 Hz one idle player standing on a pressure plate costs more than everything else combined,
 * and a farm with a dozen plates is a denial of service against the server itself. This cache is
 * a requirement and not an optimization.
 * Both allow and deny are cached, since a player walking around their own claim produces exactly
 * the same call volume. The key uses the XUID and not an Actor*, because an actor pointer is
 * recycled and within the TTL window a recycled pointer would hand a resident's allow decision to
 * a griefer. The position is part of the key, so moving one cell invalidates it immediately, and
 * the worst a stale entry does is repeat the correct decision for the same position a few ticks
 * late. Server thread only, so no lock. / */
#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

namespace pier::hooks
{
    /** 250 ms, which is 5 ticks. Long enough to matter, short enough to go
     *  unnoticed. */
    inline constexpr long long kDecisionTtlMs = 250;

    struct ThrottledDecision
    {
        int x = 0, y = 0, z = 0;
        int dim = 0;
        bool cancelled = false;
        long long atMs = 0;
    };

    inline long long throttleNowMs()
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    /**
     * Looks up the cached decision for key at (x,y,z,dim). A hit returns true and writes
     * out, while false means the caller must really dispatch and then call
     * throttleStore.
     *
     * The cache is supplied by the caller and each hook holds its own table. Sharing one
     * would let a pressure plate decision answer a push question at the same
     * coordinate.
     */
    inline bool throttleLookup(
        std::unordered_map<std::string, ThrottledDecision>& cache,
        std::string const& key,
        int x,
        int y,
        int z,
        int dim,
        long long now,
        bool& out)
    {
        auto it = cache.find(key);
        if (it == cache.end()) return false;
        auto const& c = it->second;
        if (c.x != x || c.y != y || c.z != z || c.dim != dim) return false;
        if (now - c.atMs >= kDecisionTtlMs) return false;
        out = c.cancelled;
        return true;
    }

    inline void throttleStore(
        std::unordered_map<std::string, ThrottledDecision>& cache,
        std::string const& key,
        int x,
        int y,
        int z,
        int dim,
        long long now,
        bool cancelled)
    {
        // Bounded growth: an entry is added only on a miss, and once the table exceeds
        // any plausible player count it is dropped whole. That is simpler than expiring
        // entry by entry and costs at most one extra dispatch per player.
        if (cache.size() > 512) cache.clear();
        cache[key] = ThrottledDecision{x, y, z, dim, cancelled, now};
    }
} // namespace pier::hooks

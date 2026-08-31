/**
 * pier/hooks/decision_throttle.h —— 按（玩家, 地点）为键的判定缓存，供被动游戏
 * 行为的合成事件使用。
 *
 * PressurePlateEvent 的 entityInside 对每个压着方块的实体每 tick 跑一遍，
 * PushEntityEvent 对每对重叠实体每 tick 跑一遍。每次派发要拼 SNBT、跨 FFI、在
 * 另一侧解析、查领地库、再跨回来；按 20 Hz 计，一个挂机玩家站在压力板上的开销
 * 就超过其余全部，十几块压力板的农场等于对服务器本身的拒绝服务。这个缓存是必需
 * 而不是优化。
 *
 * 放行与拒绝都缓存：玩家在自己领地里走动产生的调用量一模一样。键用 XUID 不用
 * Actor*，实体指针会被回收，TTL 窗口内一个回收的指针会把住户的放行判定发给破坏
 * 者。位置进键，挪一格立即失效，陈旧条目最坏只是重放同一位置几 tick 前的正确判
 * 定。仅服务器线程，无需加锁。
 */
#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

namespace pier::hooks
{
    /** 250 ms 即 5 tick。长到有意义，短到无感。 */
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
     * 查 key 在 (x,y,z,dim) 的缓存判定。命中返回 true 并写 out；false 表示调用方
     * 必须真派发、然后调 throttleStore。
     *
     * cache 由调用方提供，每个钩子各持一张表：共用一张会让压力板的判定在同一坐标
     * 上回答推挤的问题。
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
        // 有界增长：只在未命中时新增条目，表超过任何说得通的玩家数就整张丢掉。
        // 比逐条过期简单，清空的代价至多是每个玩家多派发一次。
        if (cache.size() > 512) cache.clear();
        cache[key] = ThrottledDecision{x, y, z, dim, cancelled, now};
    }
} // namespace pier::hooks

/**
 * pier/hooks/decision_throttle.h —— 面向**被动**游戏行为（而非玩家主动动作）
 * 的合成事件的、按（玩家, 地点）为键的判定缓存。
 *
 * # 为什么需要它
 *
 * 这个目录里多数钩子每个明确的动作触发一次：一次点击、一次丢弃、一次上
 * 骑。有两个不是：
 *
 *   PressurePlateEvent.cpp —— `entityInside` 对每个压着方块的实体每 tick
 *                             跑一遍
 *   PushEntityEvent.cpp    —— 碰撞推挤对每对重叠实体每 tick 跑一遍
 *
 * 每次派发意味着：拼一条 SNBT、跨一次 FFI、在另一侧解析、查领地库、再跨回
 * 来。按 20 Hz × 每玩家 × 每方块，一个挂机玩家站在压力板上的开销就超过插
 * 件其余全部的总和，一个有十几块压力板的农场就是对装了保护的服务器本身的
 * 拒绝服务。这里的节流不是微优化；没有它这套保护根本没法交付。
 *
 * # 为什么两种结论都缓存
 *
 * 只缓存「拒绝」是诱人的半吊子方案，而且是错的：玩家在**自己**领地里走动
 * 产生的调用量一模一样，那些全是按全价支付的「放行」判定。常见情形也必须
 * 便宜。
 *
 * # 为什么键是 XUID
 *
 * `Actor*` 是顺手的键，但不安全：实体指针会被回收，TTL 窗口内一个回收的指
 * 针会把另一个玩家的判定发给这个玩家。在要紧的那个方向上，那是一次错误的
 * **放行** —— 破坏者继承了住户的权限。相比之下每次调用哈希一个字符串便宜
 * 得很。
 *
 * # 为什么位置进键
 *
 * 缓存绝不许把判定带过领地边界。把地点编进键意味着挪一格立即失效，于是一
 * 条陈旧条目最坏也只是把「同一玩家在同一位置几 tick 前的正确判定」重放一
 * 遍。
 *
 * 仅服务器线程（和这里每个钩子一样），无需加锁。
 */
#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

namespace pier::hooks
{
    /** 250 ms = 5 tick。长到有意义，短到无感。 */
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
     * 查 `key` 在 (x,y,z,dim) 的缓存判定。命中返回 true 并写 `out`；false
     * 表示调用方必须真派发、然后调 `throttleStore`。
     *
     * `cache` 由调用方提供，让每个钩子各持一张表 —— 共用一张会让压力板的判
     * 定在同一坐标上回答推挤的问题。
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
        // 有界增长：只在未命中时新增条目，表一旦超过任何说得通的玩家数就整
        // 张丢掉。比逐条过期简单，而清空的代价至多是每个玩家多派发一次。
        if (cache.size() > 512) cache.clear();
        cache[key] = ThrottledDecision{x, y, z, dim, cancelled, now};
    }
} // namespace pier::hooks

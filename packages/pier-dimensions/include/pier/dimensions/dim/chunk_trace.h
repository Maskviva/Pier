#pragma once

#include <string>

/**
 * chunk_trace.h —— 区块生命周期追踪。
 *
 * 只在 PIER_TRACE_CHUNK=1 时才真正安装 hook，关掉时一个 detour 都不装，热路径上没
 * 有额外开销。可选变量：PIER_TRACE_CHUNK_DIM 只看某个维度（-1 为全部，默认只打自定
 * 义维度 id >= 3），PIER_TRACE_CHUNK_FAIL=1 连 tryChangeState 的失败分支一起打。
 *
 * 开关暴露在头文件里，因为 PlotGenerator::loadChunk 也要问「现在在追踪吗」。判据只
 * 能有一份：在两处各抄一遍读 env 的代码，改了变量名或判据只改一处时，症状是「追踪
 * 开着而生成那一段没有日志」，看起来像生成器根本没被调用。
 *
 * 环境变量是用户可见字符串，同受契约 §七 的产品名禁令，所以前缀是 PIER_。这是排查
 * 开关不是数据格式，旧名不做兼容。
 */
namespace pier::dimensions
{
    /** 追踪总开关（读一次 env 缓存）。PlotGenerator 也用它。 */
    bool chunkTraceEnabled();

    /** 维度过滤：-1 = 全部，-2 = 仅自定义维度(>=3)，其余 = 只看该 id。 */
    int chunkTraceDimFilter();

    /** 这个维度要不要打。 */
    bool chunkTraceWanted(int dimId);

    /** 日志里的维度标签："name(id)"，查不到名字就只有 id。 */
    std::string chunkTraceDimLabel(int dimId);

    void registerChunkTraceHooks();
    void unregisterChunkTraceHooks();
} // namespace pier::dimensions

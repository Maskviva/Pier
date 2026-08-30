#pragma once

#include <string>

/**
 * chunk_trace.h —— 区块生命周期追踪。
 *
 * 只在环境变量 `PIER_TRACE_CHUNK=1` 时才真正安装 hook —— 关掉时一个 detour
 * 都不装，热路径上没有任何额外开销。
 *
 * 用法（PowerShell）：
 *
 *     $env:PIER_TRACE_CHUNK=1
 *     $env:PIER_TRACE_CHUNK_DIM=3        # 可选，只看某个维度
 *     $env:PIER_TRACE_CHUNK_FAIL=1       # 可选，连 tryChangeState 的失败分支一起打
 *     .\bedrock_server.exe
 *
 * 默认只打自定义维度（id >= 3）。想连主世界一起看，设 `PIER_TRACE_CHUNK_DIM=-1`。
 *
 * ## 为什么开关暴露在头文件里
 *
 * `PlotGenerator::loadChunk` 也要问「现在在追踪吗」。旧版是在
 * PlotGenerator.cpp 里**又抄了一遍**读 env 的那三行 —— 抄多了的代价不是行数
 * 是**漂移**：改了变量名或判据只改一处，症状是「追踪开着，但生成那一段没有
 * 日志」，而这看起来像是生成器根本没被调用。判据只能有一份。
 *
 * ## 变量名从 MORE_DIMENSIONS_* 改成 PIER_*
 *
 * 环境变量是**用户可见字符串**，同受契约 §七 的产品名禁令。
 * 旧名不做兼容 —— 这是排查开关，不是数据格式，改名的代价只是一次文档更新。
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

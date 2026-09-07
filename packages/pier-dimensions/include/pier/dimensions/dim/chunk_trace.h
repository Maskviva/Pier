#pragma once

#include <string>

/**
 * chunk_trace.h: chunk lifecycle tracing.
 * Hooks are installed only when PIER_TRACE_CHUNK=1. With it off not a single detour is installed
 * and the hot path carries no extra cost. Optional variables: PIER_TRACE_CHUNK_DIM narrows to one
 * dimension, where -1 means all and the default covers custom dimensions with id 3 or above, and
 * PIER_TRACE_CHUNK_FAIL=1 also prints the failure branch of tryChangeState.
 *
 * The switch is exposed in the header so that the env is read in exactly one place. Copying the
 * env-reading code into a second place and later changing the variable name or the test in one of
 * them gives the symptom of tracing being on while the generation stage prints nothing, which looks
 * like the generator was never called. No generator under src/gen/ asks yet, so the generation
 * stage currently prints nothing for either reason.
 *
 * Environment variables are user-visible strings and fall under the product name ban of contract
 * §7, hence the PIER_ prefix. This is a diagnostic switch and not a data format, so no older name
 * is accepted. / */
namespace pier::dimensions
{
    /** The master tracing switch, reading the env once and caching it. */
    bool chunkTraceEnabled();

    /** Dimension filter: -1 is all, -2 is custom dimensions only, meaning id 3 or
     *  above, and any other value narrows to that id. */
    int chunkTraceDimFilter();

    /** Whether this dimension should be printed. */
    bool chunkTraceWanted(int dimId);

    /** The dimension label used in the log, "name(id)", or just the id when the name
     *  cannot be resolved. */
    std::string chunkTraceDimLabel(int dimId);

    void registerChunkTraceHooks();
    void unregisterChunkTraceHooks();
} // namespace pier::dimensions

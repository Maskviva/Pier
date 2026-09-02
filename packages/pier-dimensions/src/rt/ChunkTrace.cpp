/**
 * ChunkTrace.cpp: a diagnostic tracer for the chunk pipeline. No detour is installed by default;
 * the switches are in chunk_trace.h. It exists because "the server reaches Loaded and the client
 * shows nothing" cannot be told apart from the symptom alone, and each pair of hooks marks one fork
 * in the road. The tracer becomes part of what it observes, so the failure branch of tryChangeState
 * is off by default: in one 17-second join it contributed 24186 ERROR lines and silenced every log
 * source for 13 seconds, and the resulting "chunk loading stalls near the origin" was an observer
 * effect rather than the phenomenon. Those lines are not errors either, since a chunk source probes
 * the whole state machine and false only means it is not currently in that state.
 * PIER_TRACE_CHUNK_FAIL=1 turns them on. Picking a hook point in these headers starts with the
 * platform macro. Anything wrapped in LL_PLAT_C is a client-side query whose symbol is in the
 * export table of bedrock_server.exe, so it compiles and links while the server path never calls it
 * and the hook never fires. What works on the server is what carries no platform macro, or only
 * LL_PLAT_S. / */
#include "pier/dimensions/dim/chunk_trace.h"

#include <atomic>
#include <cstdlib>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <tuple>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/network/packet/DimensionDataPacket.h"
#include "mc/network/packet/LevelChunkPacket.h"
#include "mc/network/packet/SubChunkPacket.h"
#include "mc/network/packet/SubChunkRequestPacket.h"
#include "mc/platform/Result.h"
#include "mc/server/ChunkPositionAndDimension.h"
#include "mc/server/NetworkChunkPublisher.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/chunk/ChunkState.h"
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/world/level/dimension/Dimension.h"

#include "pier/dimensions/base/native_dimensions.h"
#include "pier/support/log.h"

namespace pier::dimensions
{
    namespace
    {
        using ::pier::hostLogger;

        /** Reads an environment variable where the value 1 means on. Read once. */
        bool envFlag(char const* name)
        {
            auto const* v = std::getenv(name);
            return v != nullptr && v[0] == '1';
        }

        bool traceFailures()
        {
            static bool const on = envFlag("PIER_TRACE_CHUNK_FAIL");
            return on;
        }

        char const* stateName(ChunkState s)
        {
            switch (s)
            {
            case ChunkState::Unloaded: return "Unloaded";
            case ChunkState::Generating: return "Generating";
            case ChunkState::Generated: return "Generated";
            case ChunkState::StructurePostProcessing: return "StructurePostProcessing";
            case ChunkState::StructurePostProcessed: return "StructurePostProcessed";
            case ChunkState::DecorationPostProcessing: return "DecorationPostProcessing";
            case ChunkState::DecorationPostProcessed: return "DecorationPostProcessed";
            case ChunkState::CheckingForReplacementData: return "CheckingForReplacementData";
            case ChunkState::NeighborAwareUpgradeNeeded: return "NeighborAwareUpgradeNeeded";
            case ChunkState::NeighborAwareUpgrading: return "NeighborAwareUpgrading";
            case ChunkState::NeedsLighting: return "NeedsLighting";
            case ChunkState::Lighting: return "Lighting";
            case ChunkState::LightingFinished: return "LightingFinished";
            case ChunkState::Loaded: return "Loaded";
            case ChunkState::Invalid: return "Invalid";
            default: return "?";
            }
        }

        /**
         * Reads the dimension id from a LevelChunk.
         *
         * The `mDimension` member is used rather than `getDimension()`, which the 26.20
         * SDK marks MCFOLD, meaning the linker folded it, so taking its address is
         * unreliable. `getDimensionId()` is virtual and an ordinary virtual call is fine.
         *
         * -999 is the sentinel for cannot-be-determined and is not a dimension. It
         * matches no filter, so the line is skipped silently, which is right for a
         * diagnostic tool: a tracer must not change the behavior of the program it
         * observes because one field could not be read.
         */
        int dimIdOf(LevelChunk const& lc)
        {
            try
            {
                // Converted implicitly to Dimension&, the same form
                // `auto& level = mLevel;` uses in PlotDimension. TypedStorage does not
                // guarantee operator-> when it holds a reference. The full rules are in
                // the file header of `tools/typed-storage.py`.
                Dimension& dim = lc.mDimension;
                return dim.getDimensionId().value();
            }
            catch (...)
            {
                return -999;
            }
        }

        /** Only so the log shows how many chunks moved in total, which helps while
         *  diagnosing. */
        std::atomic<uint64_t> gTransitions{0};
        std::atomic<uint64_t> gCreated{0};
        std::atomic<uint64_t> gLoaded{0};
        std::atomic<uint64_t> gSendOk{0};
        std::atomic<uint64_t> gSendFail{0};
        std::atomic<uint64_t> gLevelChunkPkts{0};
        std::atomic<uint64_t> gSubChunkPkts{0};
    } // namespace

    //  The switches. PlotGenerator reads this same copy.

    bool chunkTraceEnabled()
    {
        static bool const on = envFlag("PIER_TRACE_CHUNK");
        return on;
    }

    int chunkTraceDimFilter()
    {
        static int const dim = []
        {
            auto const* v = std::getenv("PIER_TRACE_CHUNK_DIM");
            if (!v) return -2;
            try
            {
                return std::stoi(v);
            }
            catch (...)
            {
                // A non-numeric value falls back to the default filter and says so,
                // otherwise setting the filter to dim 5 and seeing every other dimension
                // in the log gives no clue why.
                hostLogger().warn(
                    "[chunk] PIER_TRACE_CHUNK_DIM='{}' is not an integer, using the default of custom dimensions only, id 3 and above", v
                );
                return -2;
            }
        }();
        return dim;
    }

    bool chunkTraceWanted(int dimId)
    {
        int const f = chunkTraceDimFilter();
        if (f == -1) return true;
        if (f == -2) return dimId >= 3;
        return dimId == f;
    }

    std::string chunkTraceDimLabel(int dimId)
    {
        auto name = dimensionNameOf(dimId);
        return name.empty() ? std::to_string(dimId) : (name + "(" + std::to_string(dimId) + ")");
    }

    namespace
    {
        // A short alias inside this file, so the full name is not repeated per line.
        bool wanted(int dimId) { return chunkTraceWanted(dimId); }
        std::string dimLabel(int dimId) { return chunkTraceDimLabel(dimId); }
    } // namespace

    /*
     * A chunk was created, meaning someone requested that coordinate.
     *
     * This is the watershed between a request-side and a generation-side fault:
     *   no line for a distant coordinate -> nobody asked for it, and the fault is in
     *                                       ChunkViewSource, the player radius or the
     *                                       view distance, not the generator
     *   a line but nothing after it      -> the request arrived and is stuck in the
     *                                       state machine, so read the transitions below
     */
    LL_TYPE_INSTANCE_HOOK(
        LevelChunkCtorTraceHook,
        HookPriority::Normal,
        LevelChunk,
        &LevelChunk::$ctor,
        void*,
        ::Dimension& dimension,
        ::ChunkPos const& cp,
        bool readOnly,
        ::SubChunkInitMode initBlocks,
        bool initializeMetaData,
        ::LevelChunkBlockActorStorage::TrackingMode blockActorTrackingMode
    )
    {
        auto* ret = origin(dimension, cp, readOnly, initBlocks, initializeMetaData, blockActorTrackingMode);

        int const dimId = dimension.getDimensionId().value();
        if (wanted(dimId))
        {
            auto const n = gCreated.fetch_add(1) + 1;
            hostLogger().info(
                "[create] dim={} chunk=({}, {}) readOnly={} total={}",
                dimLabel(dimId), cp.x, cp.z, readOnly ? 1 : 0, n
            );
        }
        return ret;
    }

    /*
     * Every state transition. A healthy chunk runs the whole way:
     *
     *   Unloaded -> Generating -> Generated -> StructurePostProcessing ->
     *   StructurePostProcessed -> DecorationPostProcessing ->
     *   DecorationPostProcessed -> ... -> Loaded
     *
     * The client only receives the ones that reach the end. Where a chunk stops is
     * visible here.
     */
    LL_TYPE_INSTANCE_HOOK(
        LevelChunkChangeStateTraceHook,
        HookPriority::Normal,
        LevelChunk,
        &LevelChunk::changeState,
        void,
        ::ChunkState from,
        ::ChunkState to
    )
    {
        int const dimId = dimIdOf(*this);
        if (wanted(dimId))
        {
            auto const& cp = mPosition.get();
            auto const cur = mLoadState->load();
            auto const n = gTransitions.fetch_add(1) + 1;
            if (to == ChunkState::Loaded) gLoaded.fetch_add(1);
            hostLogger().info(
                "[state ] dim={} chunk=({}, {}) {} -> {} (currently {}) total={}",
                dimLabel(dimId), cp.x, cp.z, stateName(from), stateName(to), stateName(cur), n
            );
            if (cur != from)
            {
                hostLogger().warn(
                    "[state!] dim={} chunk=({}, {}) expected to transition from {} but is currently {}, so this transition is lost",
                    dimLabel(dimId), cp.x, cp.z, stateName(from), stateName(cur)
                );
            }
        }
        origin(from, to);
    }

    LL_TYPE_INSTANCE_HOOK(
        LevelChunkTryChangeStateTraceHook,
        HookPriority::Normal,
        LevelChunk,
        &LevelChunk::tryChangeState,
        bool,
        ::ChunkState from,
        ::ChunkState to
    )
    {
        int const dimId = dimIdOf(*this);
        auto const cur = mLoadState->load();
        bool const ok = origin(from, to);
        if (wanted(dimId))
        {
            auto const& cp = mPosition.get();
            if (ok)
            {
                hostLogger().info(
                    "[try   ] dim={} chunk=({}, {}) {} -> {} succeeded",
                    dimLabel(dimId), cp.x, cp.z, stateName(from), stateName(to)
                );
            }
            else if (traceFailures())
            {
                // Debug level and not error: a failed probe is a normal return of the
                // state machine.
                hostLogger().debug(
                    "[try  -] dim={} chunk=({}, {}) {} -> {} did not transition, current state is {}",
                    dimLabel(dimId), cp.x, cp.z, stateName(from), stateName(to), stateName(cur)
                );
            }
        }
        return ok;
    }

    /*
     * The send side: whether anything actually left after Loaded.
     *
     * Loaded only means the server has that chunk ready, not that it was ever put into a
     * LevelChunkPacket. Between the two sits NetworkChunkPublisher, which maintains a
     * send region around the player position and serializes a few chunks off the queue
     * each tick.
     *
     * So a [send ] line means the packet went out and the fault is on the client side,
     * in the dimension definition, the height range or the subchunk requests. No
     * [send ] line means nothing was sent at all and the fault is in the publisher, in
     * the region center or radius, a mismatched dimension id, or the player not being
     * recognized as inside this dimension.
     */
    LL_TYPE_INSTANCE_HOOK(
        NetworkChunkPublisherSendTraceHook,
        HookPriority::Normal,
        NetworkChunkPublisher,
        &NetworkChunkPublisher::_sendQueuedChunk,
        bool,
        ::ChunkPositionAndDimension const& queuedChunk,
        ::ClientBlobCache::Server::TransferBuilder* cachedTransfer
    )
    {
        bool const ok = origin(queuedChunk, cachedTransfer);

        int const dimId = queuedChunk.mType->value();
        if (wanted(dimId))
        {
            auto const& cp = queuedChunk.mPos.get();
            if (ok)
            {
                auto const n = gSendOk.fetch_add(1) + 1;
                hostLogger().info(
                    "[send  ] dim={} chunk=({}, {}) sent to the client, total={}", dimLabel(dimId), cp.x, cp.z, n
                );
            }
            else
            {
                // A false usually means the chunk is not ready yet and the next tick
                // will retry, so one occurrence is not an error. A chunk appearing here
                // repeatedly without ever reaching the line above is one that cannot be
                // sent.
                auto const n = gSendFail.fetch_add(1) + 1;
                hostLogger().info(
                    "[send -] dim={} chunk=({}, {}) not sent this time, still queued, total={}",
                    dimLabel(dimId), cp.x, cp.z, n
                );
            }
        }
        return ok;
    }

    /*
     * The send region itself. The center and the radius decide which chunks the
     * publisher is willing to send, and an unexpectedly small radius or a center that
     * does not match the player position both show up as distant chunks never appearing.
     */
    LL_TYPE_INSTANCE_HOOK(
        NetworkChunkPublisherMoveRegionTraceHook,
        HookPriority::Normal,
        NetworkChunkPublisher,
        &NetworkChunkPublisher::moveRegion,
        void,
        ::BlockPos const& position,
        uint blockRadius,
        ::Vec3 const& direction,
        float minDistance
    )
    {
        // getChunksSentSinceStart() is not used: the header wraps it in LL_PLAT_S, this
        // project never defines that macro, and whether referencing it compiles depends
        // on the build configuration. Counting here is steadier.
        hostLogger().info(
            "[region] send region center=({}, {}, {}) radius={} blocks (about {} chunks) sent this session={}",
            position.x, position.y, position.z, blockRadius, blockRadius / 16, gSendOk.load()
        );
        origin(position, blockRadius, direction, minDistance);
    }

    /*  What the client was actually told
     * DimensionDataPacket is the only channel through which a client learns about a
     * custom dimension. It serializes the whole DimensionDefinitionGroup, and from it the
     * client learns which dimensions exist, how tall each is, which generator it uses and
     * what its dimension id is. Without it, or with wrong values inside it, the client
     * can only drop the chunks of that dimension: everything is fine on the server and
     * the player sees nothing. What to check here:
     *   - whether the custom dimension is in the list at all; absent means the client
     *     was never told
     *   - whether the id is right and not -1; -1 means the write-back step had no effect
     *   - whether the height matches dimension_height.h and the pair passed to the
     *     Dimension constructor exactly; a mismatch means the client allocates its
     *     buffer at the wrong height and every subchunk is off */
    LL_TYPE_INSTANCE_HOOK(
        DimensionDataPacketWriteTraceHook,
        HookPriority::Normal,
        DimensionDataPacket,
        &DimensionDataPacket::$write,
        void,
        ::BinaryStream& stream
    )
    {
        try
        {
            auto const& defs = *mDimensionDefinitionGroup->mDimensionDefinitions;
            hostLogger().info("[dimdata] sending the dimension definition table to the client, {} entries:", defs.size());
            for (auto const& entry : defs)
            {
                // A scalar member, an int or an enum, is used directly with no .get().
                // ll::TypedStorage is a wrapper carrying .get() and operator-> only when
                // it holds a class type; holding a scalar it is that scalar itself.
                // mDimensionType is a struct, so -> stays.
                hostLogger().info(
                    "[dimdata]   '{}' id={} height={}..{} generator={}",
                    entry.first,
                    entry.second.mDimensionType->value(),
                    entry.second.mHeightMinimum,
                    entry.second.mHeightMaximum,
                    static_cast<int>(entry.second.mGeneratorType)
                );
            }
            if (defs.empty())
            {
                hostLogger().error(
                    "[dimdata] the definition table is empty, so the client learns of no "
                    "custom dimension and the chunks of those dimensions are dropped on "
                    "arrival"
                );
            }
        }
        catch (...)
        {
            hostLogger().warn("[dimdata] reading the dimension definition table failed; the packet itself is unaffected");
        }
        origin(stream);
    }

    /*
     * Which subchunks the client is actually asking for.
     * Two observed data sets: on dim=0, the control that renders correctly, the client requests
     * indices -4..19 and every one succeeds, 2 to 4 per packet. On dim=1000 it requests -24..-32,
     * every one out of range, always 27 per packet. That dimension really has terrain at -4..4, and
     * the difference to -24..-32 is a constant 28, which means one side treats the bottom of the
     * dimension as subchunk -28, y = -448, instead of -4, y = -64.
     *
     * The reply alone does not say which side is wrong, and the two cases need entirely different
     * fixes. SubChunkRequestPacket carries mArePositionsAbsolute: a position may be absolute or an
     * offset from mCenterPos, and the absolute values and the offsets live in two different arrays,
     * mSubChunkPos and mSubChunkPosOffsets. If the two dimensions differ on that flag, that is the
     * answer. Only the first 6 are printed per dimension. / */
    LL_TYPE_INSTANCE_HOOK(
        SubChunkRequestReadTraceHook,
        HookPriority::Normal,
        SubChunkRequestPacket,
        &SubChunkRequestPacket::$_read,
        ::Bedrock::Result<void>,
        ::ReadOnlyBinaryStream& stream
    )
    {
        auto result = origin(stream);
        try
        {
            int const dimId = mDimensionType->value();
            static std::mutex mtx;
            static std::map<int, int> shown;
            {
                std::lock_guard lock{mtx};
                if (shown[dimId] >= 6) return result;
                shown[dimId] += 1;
            }

            auto const& absList = mSubChunkPos.get();
            auto const& offList = mSubChunkPosOffsets.get();
            auto const& centre = mCenterPos.get();

            std::string absY;
            for (auto const& p : absList) absY += std::to_string(p.y) + " ";
            std::string offY;
            for (auto const& o : offList) offY += std::to_string(static_cast<int>(o.mY)) + " ";

            hostLogger().info(
                "[req] dim={} absolute={} center=({}, {}, {}) requests={} "
                "absolute table {} entries (y: {}) offset table {} entries (y: {})",
                dimLabel(dimId),
                mArePositionsAbsolute ? 1 : 0,
                centre.x, centre.y, centre.z,
                mRequestCount,
                absList.size(), absY.empty() ? std::string{"-"} : absY,
                offList.size(), offY.empty() ? std::string{"-"} : offY
            );
        }
        catch (...)
        {
            // A tracer that cannot read the packet contents prints one line fewer and
            // never affects the observed flow: origin already ran above and only a return
            // follows here.
        }
        return result;
    }

    /*
     * What the out-of-range decision is actually made against.
     *
     * When every subchunk reply is IndexOutOfBounds while the dimension's own
     * mHeightRange matches the definition sent to the client exactly, the fault is not
     * two disagreeing heights but the index being judged disagreeing with the numbering
     * base of the dimension. The typical shape: the client computes (y - minY) / 16 and
     * gets 0..23 while the server expects absolute subchunk indices -4..19. Neither is
     * wrong on its own and they differ by 4.
     *
     * This prints the moment the decision is made: the index that came in, the range of
     * the dimension, and the verdict. The function runs on every subchunk request, tens
     * of thousands of times per join, so the same (dimension, index, result) prints once.
     */
    LL_TYPE_INSTANCE_HOOK(
        DimensionSubChunkRangeTraceHook,
        HookPriority::Normal,
        Dimension,
        &Dimension::isSubChunkHeightWithinRange,
        bool,
        short const& subChunkHeight
    )
    {
        bool const ok = origin(subChunkHeight);
        try
        {
            int const dimId = getDimensionId().value();
            static std::mutex mtx;
            static std::set<std::tuple<int, int, bool>> seen;
            {
                std::lock_guard lock{mtx};
                if (!seen.insert({dimId, static_cast<int>(subChunkHeight), ok}).second) return ok;
            }
            auto const& range = mHeightRange.get();
            hostLogger().info(
                "[range] dim={} judged subchunk index {} as {}; dimension range {}..{} "
                "({} subchunks, lowest {}), client-side generation={}",
                dimLabel(dimId),
                static_cast<int>(subChunkHeight),
                ok ? "in range" : "out of range",
                static_cast<int>(range.mMin),
                static_cast<int>(range.mMax),
                getHeightInSubchunks(),
                getMinHeight(),
                isClientSideGenerationEnabled() ? 1 : 0
            );
        }
        catch (...)
        {
            // As above: one line fewer and no change to the verdict, which origin
            // already computed into ok.
        }
        return ok;
    }

    /*
     *  What the step-one packet actually carries
     *
     *   mSubChunksCount = 0 with mClientNeedsToRequestSubchunks = 1
     *       -> an empty shell packet; the block data waits for the client to ask, which
     *          is step two
     *   mSubChunksCount > 0 with mSerializedChunk carrying length
     *       -> the block data is in this packet and step two never happens
     *
     * The two modes lead to entirely different investigations. If the overworld and the
     * plot world differ on this line, that is the answer. One line per chunk, a few
     * hundred per join.
     */
    LL_TYPE_INSTANCE_HOOK(
        LevelChunkPacketWriteTraceHook,
        HookPriority::Normal,
        LevelChunkPacket,
        &LevelChunkPacket::$write,
        void,
        ::BinaryStream& stream
    )
    {
        try
        {
            gLevelChunkPkts.fetch_add(1);
            int const dimId = mDimensionId->value();
            // No dimension filter here either: the overworld is the control.
            auto const& cp = mPos.get();
            hostLogger().info(
                "[levelchunk] dim={} chunk=({}, {}) subchunks={} clientMustRequest={} "
                "requestLimit={} payloadBytes={} cache={} cacheEntries={}",
                dimLabel(dimId), cp.x, cp.z,
                mSubChunksCount,
                mClientNeedsToRequestSubchunks ? 1 : 0,
                mClientRequestSubChunkLimit,
                mSerializedChunk.get().size(),
                mCacheEnabled ? 1 : 0,
                mCacheMetadata.get().size()
            );
        }
        catch (...)
        {
            hostLogger().warn("[levelchunk] reading the chunk packet failed; the packet itself is unaffected");
        }
        origin(stream);
    }

    /*
     * The subchunk reply: what the server answered when the client asked for terrain.
     * Modern Bedrock sends a chunk in two steps. LevelChunkPacket carries only the fact that a
     * chunk column exists plus an mClientNeedsToRequestSubchunks flag, with no block data, and the
     * client builds an empty column from it. The client then asks for each subchunk with
     * SubChunkRequestPacket and the server answers with SubChunkPacket, which is where the block
     * data crosses. Step one succeeding while the columns stay empty is exactly what makes a chunk
     * look the same color as the void.
     * Each entry of a SubChunkPacket carries its own result code. Success(1) and SuccessAllAir(6)
     * are normal. LevelChunkDoesntExist(2) means the server cannot find that column.
     * WrongDimension(3) means the dimension does not match, the one a custom dimension is most
     * likely to hit. PlayerDoesntExist(4) means the player index is stale. IndexOutOfBounds(5)
     * means the subchunk y index is outside the dimension height range. / */
    LL_TYPE_INSTANCE_HOOK(
        SubChunkPacketWriteTraceHook,
        HookPriority::Normal,
        SubChunkPacket,
        &SubChunkPacket::$write,
        void,
        ::BinaryStream& stream
    )
    {
        try
        {
            gSubChunkPkts.fetch_add(1);
            int const dimId = mDimensionType->value();
            // wanted() is deliberately not applied here: the overworld is the only
            // dimension that renders correctly and its result codes are the control.
            // Seeing "dim=0 success=N" next to "dim=1000 wrongDimension=N" in one log
            // needs no further reasoning.
            auto const& data = mSubChunkData.get();
            int cnt[8]{};
            for (auto const& d : data)
            {
                auto const r = static_cast<int>(d.mResult);
                cnt[(r >= 0 && r < 8) ? r : 0]++;
            }
            auto const& c = mCenterPos.get();
            hostLogger().info(
                "[subchunk] dim={} center=({}, {}, {}) entries={} | "
                "success={} allAir={} noSuchChunk={} wrongDimension={} noSuchPlayer={} "
                "outOfRange={} undefined={}",
                dimLabel(dimId), c.x, c.y, c.z, data.size(),
                cnt[1], cnt[6], cnt[2], cnt[3], cnt[4], cnt[5], cnt[0]
            );
            if (cnt[2] || cnt[3] || cnt[4] || cnt[5])
            {
                hostLogger().error(
                    "[subchunk] dim={} had subchunk requests refused, so that block data "
                    "never reaches the client and the player sees empty space; the line "
                    "above says which kind",
                    dimLabel(dimId)
                );
            }
        }
        catch (...)
        {
            hostLogger().warn("[subchunk] reading the subchunk reply failed; the packet itself is unaffected");
        }
        origin(stream);
    }

    namespace
    {
        using ChunkTraceHookReg = ll::memory::HookRegistrar<
            LevelChunkCtorTraceHook,
            LevelChunkChangeStateTraceHook,
            LevelChunkTryChangeStateTraceHook,
            NetworkChunkPublisherSendTraceHook,
            NetworkChunkPublisherMoveRegionTraceHook>;

        using PacketTraceHookReg = ll::memory::HookRegistrar<
            DimensionDataPacketWriteTraceHook,
            LevelChunkPacketWriteTraceHook,
            SubChunkPacketWriteTraceHook,
            SubChunkRequestReadTraceHook,
            DimensionSubChunkRangeTraceHook>;
    } // namespace

    void registerChunkTraceHooks()
    {
        // All chunk tracing, including the control lines for the dimension definition
        // table, LevelChunkPacket and SubChunkPacket, is off unless PIER_TRACE_CHUNK=1
        // is set.
        if (!chunkTraceEnabled()) return;
        PacketTraceHookReg::hook();
        ChunkTraceHookReg::hook();
        hostLogger().warn(
            "[chunk] chunk tracing is on (PIER_TRACE_CHUNK=1, dimension filter {}); the "
            "log volume is large, turn it off once the investigation is done",
            chunkTraceDimFilter() == -2 ? std::string{"custom dimensions only (>=3)"}
            : chunkTraceDimFilter() == -1 ? std::string{"all"}
                                          : std::to_string(chunkTraceDimFilter())
        );
    }

    void unregisterChunkTraceHooks()
    {
        if (!chunkTraceEnabled()) return;
        hostLogger().info(
            "[chunk] packet totals: {} LevelChunkPacket, {} SubChunkPacket",
            gLevelChunkPkts.load(), gSubChunkPkts.load()
        );
        if (gLevelChunkPkts.load() > 0 && gSubChunkPkts.load() == 0)
        {
            hostLogger().error(
                "[chunk] {} LevelChunkPacket were sent and not a single SubChunkPacket; "
                "either the client never requested subchunks or the requests were consumed "
                "elsewhere. No block data ever left the server",
                gLevelChunkPkts.load()
            );
        }
        PacketTraceHookReg::unhook();
        ChunkTraceHookReg::unhook();
        hostLogger().info(
            "[chunk] tracing finished: {} creations, {} state transitions, {} reached "
            "Loaded, {} sent to clients, {} left queued and unsent",
            gCreated.load(), gTransitions.load(), gLoaded.load(), gSendOk.load(), gSendFail.load()
        );
        if (gLoaded.load() > 0 && gSendOk.load() == 0)
        {
            hostLogger().error(
                "[chunk] {} chunks reached Loaded on the server and none was sent through "
                "NetworkChunkPublisher, so the fault is on the send side and not the "
                "generation side; check the [region] lines above for a sensible center and "
                "radius and for the dimension id being this dimension",
                gLoaded.load()
            );
        }
    }
} // namespace pier::dimensions

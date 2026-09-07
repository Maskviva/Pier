#pragma once

/** native_dimensions.h: a wrapper over the native custom dimension interface of the BDS 26.20
 * engine. This host takes the native path only. The older FakeDimensionId approach, rewriting the
 * dimension id of outbound packets, intercepting DimensionDataPacket and faking a trip through
 * the nether before a dimension change, is removed entirely and is mutually exclusive with this
 * one. The MoreDimensions approach does not apply either: on 26.20 neither
 * VanillaDimensions::DimensionMap() nor mFactoryMap is a data source for getOrCreateDimension.
 * The engine resolves a name through DimensionManager::mDimensionNameIdStore when building a
 * dimension from an id, and with no entry there it returns an expired WeakRef, which is why
 * blockSourceOf returns nullptr and a teleport is reported as failed. registerCustomDimension()
 * handed registration back to the engine, which allocated the id and wrote it into the NameIdStore
 * of the save. On 26.32 that entry point and the lookup that read the table back are both gone, so
 * the function refuses and no custom dimension can be created; NativeDimensions.cpp states the
 * reasoning and what was rejected in its place. No function throws. A failure returns nullopt,
 * false or nullptr and logs. */

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "mc/world/level/GeneratorType.h"

class Dimension;

namespace pier::dimensions
{
    namespace native
    {
        /** Whether the engine DimensionManager is reachable, which means Level is
         *  open. */
        bool available();

        /**
         * Refuses, and logs why once per process. The engine entry point that allocated
         * the id is not reachable on this version, so a dimension cannot be created and
         * one a save already holds cannot be found again. The parameters are kept so the
         * signature survives the day the entry point comes back.
         *
         * @return        always nullopt
         */
        std::optional<int>
        registerCustomDimension(std::string const& name, int minY, int maxY, GeneratorType gen);

        /** Always nullopt on this engine version. The table it read is not reachable, so
         *  the id a save already holds cannot be reported; NativeDimensions.cpp states
         *  why answering out of the host's own ledger was rejected instead. */
        std::optional<int> engineDimensionId(std::string const& name);

        /** Whether the engine considers this id currently valid. */
        bool isActive(int dimId);

        /**
         * Forces the dimension object into existence by name, on the native path.
         *
         * This exists instead of going in by id because the engine resolves id to name
         * internally through NameIdStore, so entering by name skips one reverse lookup
         * and narrows the failure surface. The returned raw pointer is owned by
         * DimensionRegistry and must not be cached by the caller.
         */
        Dimension* getOrCreateByName(std::string const& name);
    } // namespace native

    //  The host-side name to id ledger
    //
    // An entry is recorded on a successful registration, and both faces of the dimension
    // bridge, selectorNameOf and blockSourceOf, plus md_get_dimension_id, consult it
    // first. Its data comes from the id the engine actually returned, so a private
    // mirror cannot drift from the engine, which is what makes resolving a dimension name
    // out of the config file unreliable.

    void rememberDimension(std::string const& name, int id);

    /** Drops both faces of one entry. rememberDimension(name, -1) is not the same thing:
     *  it leaves -1 mapped back to the name, so dimensionNameOf(-1) starts answering. */
    void forgetDimension(std::string const& name);
    std::string dimensionNameOf(int id);      // Empty string when not found
    int dimensionIdOf(std::string_view name); // -1 when not found
    void forEachRegisteredDimension(std::function<void(std::string const&, int)> const& fn);

    /** For logging: flattens the ledger into "name=id, name=id". */
    std::string describeRegisteredDimensions();
} // namespace pier::dimensions

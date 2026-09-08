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
         * Registers a dimension and returns the id the engine gave it.
         *
         * A name the engine already knows is returned as it stands and nothing is
         * registered a second time. For a new name the definition and the factory go in
         * with a suggested id, the instance is built, and the id is read back off the
         * Dimension; a disagreement is logged and the engine's number wins.
         *
         * @return        the engine's id, or nullopt when the definition group cannot be
         *                read, the definition is refused, or the instance cannot be built
         */
        std::optional<int>
        registerCustomDimension(std::string const& name, int minY, int maxY, GeneratorType gen);

        /** The id the engine currently has for this name, read out of
         *  DimensionDefinitionGroup. nullopt means the engine does not know the name —
         *  it no longer means the question cannot be asked. */
        std::optional<int> engineDimensionId(std::string const& name);

        /** Whether the dimension definition group can be read, which is what deciding an
         *  id depends on. Level being open is not the same question, and md_is_available
         *  answers with this one. */
        bool definitionGroupReadable();

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

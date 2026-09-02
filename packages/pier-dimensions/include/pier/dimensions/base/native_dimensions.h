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
 * hands registration back to the engine: it first ensures the definition is in
 * DimensionDefinitionGroup, then calls serverRegisterCustomDimension() for the id the engine
 * allocates, which the engine writes into the NameIdStore of the save and restores on the next
 * boot. The mFactoryMap entry is still overwritten afterwards, because by default the engine
 * builds a generic data-driven dimension while DimensionFactory::create looks that map up by name
 * and the later write wins. No function throws. A failure returns nullopt, false or nullptr and
 * logs. / */

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
         * Registers a custom dimension through the native engine flow.
         *
         * @param name    the dimension name, which is also the key of the factory map
         * @param minY    the world bottom, written into the DimensionDefinition
         * @param maxY    the world top
         * @param gen     the generator type. createGenerator is taken over by this
         *                package, so this only affects a few engine defaults for the
         *                dimension, and Flat is the safest value
         * @return        the dimension id the engine allocated, or nullopt on failure
         */
        std::optional<int>
        registerCustomDimension(std::string const& name, int minY, int maxY, GeneratorType gen);

        /** Asks the engine for the id of a name. nullopt when it is not registered. */
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
    std::string dimensionNameOf(int id);      // Empty string when not found
    int dimensionIdOf(std::string_view name); // -1 when not found
    void forEachRegisteredDimension(std::function<void(std::string const&, int)> const& fn);

    /** For logging: flattens the ledger into "name=id, name=id". */
    std::string describeRegisteredDimensions();
} // namespace pier::dimensions

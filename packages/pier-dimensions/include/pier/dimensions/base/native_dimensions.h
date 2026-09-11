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

#include <unordered_set>

#include "mc/deps/game_refs/WeakRef.h"
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
         * Registers a dimension and returns the id the engine gave it. A name the engine
         * already knows is returned as it stands; for a new name the definition and the
         * factory go in with a suggested id and the id is read back off the Dimension.
         *
         * `preferred` is the id this dimension already holds, suggested when free so it
         * keeps the number its saved chunks are under; `taken` must not contain it.
         * `bindFactory` puts the caller's closure into the engine factory map, and is
         * called after `_registerCustomDimensionWithFactory`, which registers a factory of
         * the engine's own over whatever was there.
         *
         * @return        the engine's id, or nullopt when the definition group cannot be
         *                read, the definition is refused, or the instance cannot be built
         */
        std::optional<int>
        registerCustomDimension(
            std::string const& name, int minY, int maxY, GeneratorType gen,
            ::std::unordered_set<int> const& taken, std::function<void()> const& bindFactory,
            int preferred = -1
        );

        /** The id the engine currently has for this name, read out of
         *  DimensionDefinitionGroup. nullopt means the engine does not know the name —
         *  it no longer means the question cannot be asked. */
        std::optional<int> engineDimensionId(std::string const& name);

        /** Whether the dimension definition group can be read, which is what deciding an
         *  id depends on. Level being open is not the same question, and md_is_available
         *  answers with this one. */
        bool definitionGroupReadable();

        /**
         * The first id a custom dimension may take.
         *
         * Not 3. `VanillaDimensions::Undefined()` sits just past the vanilla three and
         * the engine compares against it to mean no dimension, so a dimension holding
         * that number is read as absent by every path that makes the comparison: the
         * registry stored nothing for it, `getDimension` came back empty, and a teleport
         * left the player where they were. 1000 is where the engine's own script-api
         * registration allocates from, and where the ids in a save written by
         * MoreDimensions 0.14 come from.
         */
        inline constexpr int firstCustomDimensionId = 1000;

        /**
         * The name the engine is given for a dimension, which is not always the name the
         * caller uses.
         *
         * The engine's own registration path takes `namespace:name` and MoreDimensions,
         * the only implementation known to work on this generation, rejects anything else
         * before it ever reaches the engine. A bare name is therefore qualified with
         * `pier:` here, at the one boundary that talks to the engine, while the ledger,
         * the config file and every mod-facing call keep the name the caller chose.
         */
        std::string engineNameOf(std::string const& name);

        /**
         * The key a Dimension of this host's belongs under in DimensionRegistry, with the
         * object's own registry id set to match. nullopt for anything else.
         *
         * A Dimension carries a DimensionType, the signed int this host and the save
         * speak, and a DimensionIdType, the unsigned short that keys the registry. The
         * only mapping between them is DimensionManager::mDimensionNameIdStore, which
         * nothing exported writes since 26.20, so a dimension built for a name that table
         * has never held comes out with the two disagreeing. registerDimension assigns,
         * so leaving it stores the dimension over another one's slot and destroys it.
         */
        ::std::optional<int> claimRegistryKey(::Dimension& d);

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

        /**
         * The instance under an id, built through the factory when the registry has none.
         *
         * The one entry point that never calls getOrCreateDimension, which is what makes
         * it safe to use from a hook on that function: going through getOrCreateByName
         * there would re-enter the detour and recurse until the stack is gone. Unknown,
         * meaning the engine could not be asked whether an instance is already
         * registered, returns nullptr rather than building over a live dimension.
         */
        Dimension* ensureBuilt(std::string const& name, int id);

        /** Whether the engine currently holds a Dimension under this id. False also when
         *  the question cannot be asked, and the two are separated in the log. */
        bool hasInstance(int dimId);

        /** A reference to the instance under this id, empty when there is none. This is
         *  what a detour on getOrCreateDimension hands back to the engine: the engine's
         *  own getDimension resolves the id through its name table, which has no row
         *  for a host-registered dimension, so it answers empty for exactly these ids
         *  while the registry table holds the instance. */
        ::WeakRef<::Dimension> instanceRef(int dimId);

        /** What this host remembers under an id, without asking the engine. The hooks on
         *  `getDimension` use this and not `instanceRef`, which asks the engine first and
         *  would therefore call back into the hook. */
        ::WeakRef<::Dimension> rememberedInstanceRef(int dimId);

        /**
         * Drops what this host holds under an id, for a dimension being retired.
         *
         * The instance ledger owns the dimensions this host registered, because the engine
         * does not keep them alive reliably and a resolution that answered empty for a
         * live dimension was read as permission to build a second one, which then replaced
         * the first while the engine was still using it. Letting go is therefore a handover
         * and not a destruction: the object is kept aside, since a player may be standing
         * in it and the engine holds references it does not own.
         */
        void forgetInstance(int dimId);

        /** Releases every dimension this host holds, while the level is still standing. */
        void forgetAllInstances();
    } // namespace native

    //  The host-side name to id ledger
    //
    // An entry is recorded on a successful registration, and both faces of the dimension
    // bridge, selectorNameOf and blockSourceOf, plus md_get_dimension_id, consult it
    // first. Its data comes from the id the engine actually returned, so a private
    // mirror cannot drift from the engine, which is what makes resolving a dimension name
    // out of the config file unreliable.

    void rememberDimension(std::string const& name, int id);

    /**
     * Whether a player's game has been told that a dimension exists.
     *
     * The list of dimensions reaches a client once, in the data it is sent while joining,
     * and there is no way to send it again mid-session. A dimension registered after that
     * point is one the client cannot enter: it has no definition for the id, so it does
     * not load the destination and the player is left where they were. Comparing when a
     * dimension became available against when a session started answers it exactly, and
     * an unrecorded player is answered yes rather than have a guard break a teleport.
     */
    void noteDimensionAvailable(int id);
    void noteClientSession(long long playerId);
    bool clientKnowsDimension(long long playerId, int id);

    /** Drops both faces of one entry. rememberDimension(name, -1) is not the same thing:
     *  it leaves -1 mapped back to the name, so dimensionNameOf(-1) starts answering. */
    void forgetDimension(std::string const& name);
    std::string dimensionNameOf(int id);      // Empty string when not found
    int dimensionIdOf(std::string_view name); // -1 when not found
    void forEachRegisteredDimension(std::function<void(std::string const&, int)> const& fn);

    /** For logging: flattens the ledger into "name=id, name=id". */
    std::string describeRegisteredDimensions();
} // namespace pier::dimensions

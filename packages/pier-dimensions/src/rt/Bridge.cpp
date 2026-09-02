/**
 * pier-dimensions/rt/Bridge.cpp: the only implementation of spi::DimensionBridge.
 * core/Bridge.cpp on the api side knows nothing about custom dimensions beyond the rule
 * that id 3 or above goes through the bridge. Resolving a custom name needs three
 * sources at once, the registration ledger, the engine DimensionMap and the config
 * mirror, plus knowing which engine APIs must not be touched (see resolveIdByName), and
 * that knowledge holds only here. Without this package the bridge is absent and the api
 * side recognizes only the three vanilla dimensions, warning once per function.
 * blockSourceOf carries a safety gate: it must verify that the Dimension the engine
 * built reports the requested dim as its own id (spi.h §6). Once the ledger id and the
 * engine id drift apart, block writes land silently in the wrong dimension, because the
 * caller receives a valid BlockSource, and teleporting a player there throws an uncaught
 * exception on a chunk worker thread and fastfails with 0xC0000409. Only this package
 * can force a dimension into existence by name and then ask it for its id, so the check
 * lives here; the api side treats a non-null result as its only condition to proceed.
 */
#include <optional>
#include <set>
#include <string>
#include <string_view>

#include "ll/api/service/Bedrock.h"

#include "mc/util/BidirectionalUnorderedMap.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionType.h"
#include "mc/world/level/dimension/VanillaDimensions.h"

#include "pier/dimensions/base/native_dimensions.h"
#include "pier/dimensions/dim/custom_dimension_config.h"

#include "pier/host/spi.h"
#include "pier/support/log.h"

namespace pier::dimensions::rt
{
    namespace
    {
        /**
         * Name to id across three sources, ordered by trust: the registration ledger through
         * dimensionIdOf, holding the id the engine returned on a successful registration; the
         * engine DimensionMap; and the config mirror, which warns when it is the one that hits.
         * VanillaDimensions::toString() must not be used for a reverse lookup or a round trip.
         * fromString() answers an unknown name with Undefined(), which addDimension rewrites at
         * runtime to stay one above the highest custom id, so it always looks like a plausible id
         * and returning it convinces a caller that a nonexistent dimension exists. The object
         * toString() returns does not match the MSVC std::string layout and the text bytes land
         * where _Mysize belongs, so a dimension named "red" makes the consumer run memcpy(dst, src,
         * 0x646572) and die in VCRUNTIME140. Only DimensionMap() is read: it returns a const
         * reference, constructs and copies nothing, crosses no ABI, and is the table BDS itself
         * queries. */
        int resolveIdByName(std::string const& wanted)
        {
            if (wanted.empty()) return -1;
            if (wanted == "overworld") return 0;
            if (wanted == "nether") return 1;
            if (wanted == "the_end") return 2;

            // 1. The registration ledger
            if (int const id = dimensionIdOf(wanted); id >= 0) return id;

            // 2. The engine DimensionMap
            {
                auto const& dimMap = ::VanillaDimensions::DimensionMap();
                auto const hit = dimMap.mRight.find(wanted);
                if (hit != dimMap.mRight.end())
                {
                    auto const id = hit->second.value();
                    // Undefined() is rewritten at runtime to one above the highest
                    // allocated id, so it is always a plausible-looking number. A name
                    // that resolves to it is a name that was never registered.
                    if (id >= 0 && id != ::VanillaDimensions::Undefined().value()) return id;
                }
            }

            // 3. The config mirror, as a fallback
            auto const& list = CustomDimensionConfig::getConfig().dimensionList;
            auto const it = list.find(wanted);
            if (it == list.end()) return -1;

            pier::hostLogger().warn(
                "[dim] '{}' resolved to id {} from the config mirror rather than the engine "
                "dimension table; the two have drifted apart, so teleports and block writes "
                "may land in the wrong place, check the dimension registrations in the save",
                wanted, it->second.dimId
            );
            return it->second.dimId;
        }

        /** Id to name. The ledger first, then the engine table. An empty string when
         *  neither has it. */
        std::string resolveNameById(int32_t dim)
        {
            if (dim < 0) return {};
            switch (dim)
            {
            case 0:
                return "overworld";
            case 1:
                return "nether";
            case 2:
                return "the_end";
            default:
                break;
            }

            if (auto name = dimensionNameOf(dim); !name.empty()) return name;

            auto const& dimMap = ::VanillaDimensions::DimensionMap();
            auto const hit = dimMap.mLeft.find(DimensionType{dim});
            if (hit != dimMap.mLeft.end()) return hit->second;

            return {};
        }

        /** Complains once per id, so a call inside a loop does not turn into an
         *  incident. */
        bool firstComplaintFor(int32_t dim)
        {
            static std::set<int32_t> seen;
            return seen.insert(dim).second;
        }

        //  The two faces of spi::DimensionBridge

        std::string bridgeSelectorNameOf(int32_t dim)
        {
            auto name = resolveNameById(dim);
            if (name.empty() && firstComplaintFor(dim))
            {
                pier::hostLogger().warn(
                    "[dim] no name for dimension {}: the registration ledger, the engine "
                    "dimension table and the config mirror all lack it; registered are: {}",
                    dim, describeRegisteredDimensions()
                );
            }
            return name;
        }

        ::BlockSource* bridgeBlockSourceOf(int32_t dim)
        {
            auto name = resolveNameById(dim);
            if (name.empty())
            {
                if (firstComplaintFor(dim))
                {
                    pier::hostLogger().error(
                        "[dim] dimension {} has no resolvable name, so no instance can be built; registered are: {}",
                        dim, describeRegisteredDimensions()
                    );
                }
                return nullptr;
            }

            // Forces the dimension into existence by name. The engine resolves id to
            // name internally through NameIdStore, so entering by name skips one reverse
            // lookup and narrows the failure surface.
            auto* real = native::getOrCreateByName(name);
            if (!real) return nullptr; // getOrCreateByName already logged one of three reasons

            //  Safety gate: the engine instance id must equal the requested dim
            //
            // See the file header. On a mismatch this BlockSource must not be handed
            // over: the caller would write blocks through it, landing silently in the
            // wrong dimension, or allow a teleport on the strength of it, which throws
            // an uncaught exception on a chunk thread and fastfails the process with
            // 0xC0000409.
            int const realId = real->getDimensionId().value();
            if (realId != dim)
            {
                pier::hostLogger().error(
                    "[dim] refusing to provide a BlockSource for dimension {}: the ledger "
                    "calls it '{}', but the instance the engine built under that name reports "
                    "id {}. The ledger and the engine have drifted apart, and continuing "
                    "would either write blocks into the wrong dimension or abort on a chunk "
                    "thread during a teleport",
                    dim, name, realId
                );
                return nullptr;
            }

            return &real->getBlockSourceFromMainChunkSource();
        }

        spi::DimensionBridge const gBridge{
            &bridgeSelectorNameOf,
            &bridgeBlockSourceOf,
        };

        /** Bootstrap. Installs the bridge, before any API call, on the host startup
         *  path. */
        void bootstrap()
        {
            spi::setDimensionBridge(&gBridge);
            pier::hostLogger().debug("[dim] dimension bridge installed, custom dimensions available");
        }

        // Stage 10. The bridge must be installed before any dimension registration at
        // stage 20 or later, because the self-check on the registration path asks the
        // bridge back for the id the engine actually built.
        spi::BootstrapReg regBoot{{10, "dimension-bridge", &bootstrap}};
    } // namespace

    /** Name resolution for the other TUs of this package, used by the self-check on
     *  the addDimension path. */
    int idByName(std::string const& name) { return resolveIdByName(name); }
} // namespace pier::dimensions::rt

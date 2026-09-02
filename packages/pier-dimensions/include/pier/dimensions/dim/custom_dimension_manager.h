#pragma once

/** custom_dimension_manager.h: the registration hub for custom dimensions.
 * The only place in this package that allocates a dimension id. Three sources meet here and are
 * forced into agreement: the engine NameIdStore, dimension_config.json and the host's own name
 * ledger. DimensionFactoryInfo is the bundle a factory closure receives: arguments holds the
 * derived-dimension constructor parameters from the engine, valid only while the closure runs;
 * data holds this package's payload, the seed and layout; dimId is the id already decided, which
 * the closure must not guess at again. addDimension is a template and a caller writes
 * addDimension<PlotDimension>(name, seed, layout). The dimension type is fixed at compile time
 * and both the factory closure and generateNewData follow from it, as a template parameter rather
 * than a runtime enum so that adding a dimension kind needs no change here. This package is an
 * object package compiled into the host and exports no symbol, so there is no export macro. No
 * id-by-name function forwarding to VanillaDimensions::fromString may be added either: that route
 * reads back garbage for a custom dimension, for the reason rt/Bridge.cpp gives. / */

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/dimension/DimensionType.h"

class Dimension;
class DerivedDimensionArguments;

namespace pier::dimensions
{
    struct DimensionFactoryInfo
    {
        DerivedDimensionArguments& arguments;
        CompoundTag const& data;
        DimensionType dimId;
    };

    class CustomDimensionManager
    {
        struct Impl;
        std::unique_ptr<Impl> impl;

        CustomDimensionManager();
        ~CustomDimensionManager();

    public:
        using DimensionFactoryT = std::shared_ptr<Dimension>(DimensionFactoryInfo const&);

        CustomDimensionManager(CustomDimensionManager const&) = delete;
        CustomDimensionManager& operator=(CustomDimensionManager const&) = delete;

        /**
         * The singleton. Construction reads the config and installs hooks, so the first
         * call must happen once Level is ready, since it reads levelName to locate the
         * save directory.
         */
        static CustomDimensionManager& getInstance();

        /**
         * Registers a dimension. On success it returns the id the engine allocated, 3 or
         * above.
         *
         * Any failure throws and never returns a value that looks like an id. Failure
         * here has one shape, a registration that did not take effect, and a caller
         * teleporting a player with a fake id makes the engine throw an uncaught
         * exception on a chunk worker thread and fastfail the process with 0xC0000409.
         * Cannot-be-determined and the answer is 3 must stay apart (contract §5.2). On
         * the ABI side `PIER_API_GUARD_END_VAL(-1)` turns the exception into -1.
         */
        template <std::derived_from<Dimension> D, class... Args>
        DimensionType addDimension(std::string const& dimName, Args&&... args)
        {
            return addDimension(
                dimName,
                [dimName](DimensionFactoryInfo const& info) -> std::shared_ptr<Dimension>
                {
                    return std::make_shared<D>(dimName, info);
                },
                [&] { return D::generateNewData(std::forward<Args>(args)...); }
            );
        }

    protected:
        DimensionType addDimension(
            std::string const& dimName,
            std::function<DimensionFactoryT> factory,
            std::function<CompoundTag()> const& newData
        );
    };
} // namespace pier::dimensions

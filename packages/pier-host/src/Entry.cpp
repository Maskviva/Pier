#include <memory>

#include "ll/api/mod/ModManagerRegistry.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"

#include "sdk/abi.h"

#include "pier/host/api_table.h"
#include "pier/host/hosted_mod.h"
#include "pier/host/mod_host.h"
#include "pier/host/spi.h"

#ifndef PIER_BUILD_CLIENT
#include "pier/host/mod_control.h"
#endif

namespace pier
{
    class LoaderMod
    {
    public:
        static LoaderMod& getInstance()
        {
            static LoaderMod instance;
            return instance;
        }

        [[nodiscard]] ll::mod::NativeMod& getSelf() const { return *ll::mod::NativeMod::current(); }

        bool load()
        {
            auto& logger = getSelf().getLogger();

            // The order is the correctness argument:
            //   1. Fill the table. Each capability package writes its function
            //      pointers into PierApi and an absent package leaves NULL, which is
            //      the whole mechanism behind "optional" (contract §2.1).
            //   2. Bootstrap. Steps that must run as soon as the host is up, such as
            //      dimensions installing hooks and reading its config.
            //   3. Register the manager. Only from this point can LeviLamina dispatch
            //      a pier mod, and such a mod must receive a fully built table.
            spi::buildApi(mutableApi(), logger);
            spi::runBootstrap(logger);

            if (!ll::mod::ModManagerRegistry::getInstance().addManager(std::make_shared<ModHost>()))
            {
                logger.error("[host] mod manager '{}' registration failed", ModHostName);
                return false;
            }
            logger.info("[host] ready, ABI v{}, api table {} bytes", PIER_ABI_VERSION, sizeof(PierApi));
            return true;
        }

        bool enable()
        {
#ifndef PIER_BUILD_CLIENT
            // /pier provides runtime load, unload and self-check. It is registered
            // from enable() and not load() because CommandRegistrar needs the server
            // command system to be up.
            mod_control::registerCommand();
#endif
            return true;
        }

        bool disable() { return true; }
    };
} // namespace pier

LL_REGISTER_MOD(pier::LoaderMod, pier::LoaderMod::getInstance());

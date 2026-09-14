#include <memory>
#include <string>

#include "ll/api/io/Logger.h"
#include "ll/api/mod/ModManagerRegistry.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"

#include "sdk/abi.h"

#include "pier/support/config.h"
#include "pier/support/i18n.h"

#include "pier/host/api_table.h"
#include "pier/host/hosted_mod.h"
#include "pier/host/mod_host.h"
#include "pier/host/spi.h"

#ifndef PIER_BUILD_CLIENT
#include "pier/host/mod_control.h"
#endif

namespace pier
{
    namespace
    {
        /** The color the engine writes its own startup lines in, as 24-bit SGR.
         *
         *  A console renders this and does not render `§`, so a line an operator reads at
         *  a glance carries the escape itself. */
        constexpr char const* kHi = "\x1b[38;2;135;206;250m";
        constexpr char const* kOff = "\x1b[0m";
    } // namespace

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
            //   1. Settings, then languages, so every line below is in the operator's
            //      language, including what was wrong with the settings themselves.
            //   2. Fill the table. An absent capability package leaves its slots NULL,
            //      which is the whole mechanism behind "optional" (contract §2.1).
            //   3. Bootstrap, for work that must run as soon as the host is up.
            //   4. Register the manager. Only from here can LeviLamina dispatch a pier
            //      mod, and such a mod must receive a fully built table.
            reportStartup(logger);
            spi::buildApi(mutableApi(), logger);
            spi::runBootstrap(logger);

            if (!ll::mod::ModManagerRegistry::getInstance().addManager(std::make_shared<ModHost>()))
            {
                logger.error("[host] mod manager '{}' registration failed", ModHostName);
                return false;
            }
            // The build stamp is on this line because the question it answers comes up
            // on every report: whether the binary running is the one that was just
            // changed. A log that cannot answer it costs a round trip each time.
            logger.info("[host] {}{}{}", kHi,
                        pier::trf("host.ready", PIER_ABI_VERSION, sizeof(PierApi), __DATE__, __TIME__),
                        kOff);
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

    private:
        /** config.json, then the language it names, then everything wrong with either.
         *
         *  The settings are read before the catalog exists, because the language is one
         *  of them, so their problems arrive as keys and are rendered here. An operator
         *  whose config file is broken reads about it in the language of their server. */
        void reportStartup(ll::io::Logger& logger)
        {
            auto const modDir = getSelf().getModDir();
            auto problems = pier::loadConfig(modDir.string());

            auto const langDir = (modDir / "lang").string();
            // Read, never written. The directory ships in the release, next to the dll,
            // and this host does not put files into a mod directory an operator owns.
            auto fromDisk = pier::loadLanguages(langDir, pier::config().language);
            // Printed every boot, not only when a file was found. An operator running
            // an English log on a translated server needs the line that says the lang
            // directory held nothing, and that line only exists if it prints either way.
            logger.info(
                "[host] {}{}{}", kHi,
                pier::trf("lang.loaded", pier::localeCode(),
                          pier::localeFromConfig() ? pier::tr("lang.pick.config")
                                                   : pier::tr("lang.pick.engine"),
                          pier::activeKeyCount(),
                          fromDisk > 0 ? std::string_view{langDir} : pier::tr("lang.source.builtin")),
                kOff);
            // A language named in the config with no file behind it is the one case the
            // line above cannot express: it reports a full key count, because the
            // built-in English answers, while nothing the operator asked for applies.
            if (fromDisk == 0 && pier::localeFromConfig())
            {
                logger.warn("[host] {}", pier::trf("lang.no_file", pier::localeCode()));
            }

            for (auto const& p : problems)
            {
                // Error and not warning for anything the operator wrote that is not in
                // effect. The permissive direction is the dangerous one here: an empty
                // mods.disabled loads exactly what they disabled.
                auto const line = pier::trv(p.key, p.args);
                if (p.level == pier::ConfigProblem::Level::Note) logger.info("[host] {}", line);
                else logger.error("[host] {}", line);
            }
        }
    };
} // namespace pier

LL_REGISTER_MOD(pier::LoaderMod, pier::LoaderMod::getInstance());

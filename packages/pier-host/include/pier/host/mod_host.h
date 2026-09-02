#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ll/api/Expected.h"
#include "ll/api/mod/Manifest.h"
#include "ll/api/mod/ModManager.h"

#include "pier/host/hosted_mod.h"

namespace pier
{
    /**
     * The ModManager for manifests carrying `"type": "pier"`.
     *
     * Registered by the loader inside ll_mod_load. ModRegistrar resolves the manager
     * only when it dispatches a load (see ModManagerRegistry::loadMod) and sorts
     * topologically by `dependencies`, so this manager is guaranteed to exist by the
     * time any mod declaring
     *   "dependencies": [{ "name": "pier" }]
     * is dispatched to it.
     */
    class ModHost : public ll::mod::ModManager
    {
    public:
        ModHost();
        ~ModHost() override;

        ll::Expected<> load(ll::mod::Manifest manifest) override;
        ll::Expected<> unload(std::string_view name) override;

        /**
         * The running manager. nullptr before ll_mod_load and after shutdown.
         * Set in the constructor and cleared in the destructor. The shared_ptr is
         * owned by the registry, so this is only an observer.
         */
        static ModHost* instance();

        /*  Runtime mod control, the backend of /pier
         * loadMod, unloadMod, enableMod and disableMod on ModManagerRegistry are all
         * private and befriended only to ModRegistrar and Mod, so after startup no
         * public LeviLamina path can load another mod. What remains available is the
         * inherited protected ModManager interface, which these two wrappers use.
         *
         * One consequence is worth remembering. A mod brought up this way lives in
         * this manager's own table and not in the LeviLamina dependency graph.
         * Dependency checking is therefore done by ModControl reading manifests
         * directly, which as a side effect also finds a native C++ mod that depends
         * on a pier mod. */

        /** Brings up a parsed manifest by calling load() and then enable(). */
        ll::Expected<> controlLoad(ll::mod::Manifest manifest);

        /** Takes one down by calling disable(), which fires on_disable, then
         *  unload(). */
        ll::Expected<> controlUnload(std::string_view name);

        /** Base address of a loaded mod's dylib, or nullptr when it is not loaded.
         *  Used to detect the Windows FreeLibrary reference count trap. The same base
         *  address after a reload means the image was never unmapped and the new dll
         *  on disk was not read. */
        [[nodiscard]] void const* moduleBase(std::string_view name) const;

        /** Names of every mod this manager currently has loaded. */
        [[nodiscard]] std::vector<std::string> loadedNames() const;

        /** Snapshot of every currently loaded mod, holding strong references. For
         *  capability packages that must resolve ownership from a callback address,
         *  such as the ownerless legacy slots in Money and Scheduler. ll::mod::Mod is
         *  not a polymorphic type, so dynamic_cast on the Mod& from
         *  ModManagerRegistry::mods() is not valid, whereas every entry in this
         *  manager's table is certainly a HostedMod. */
        [[nodiscard]] std::vector<std::shared_ptr<HostedMod>> hostedMods() const;
    };
} // namespace pier

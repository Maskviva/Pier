#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ll/api/Expected.h"
#include "ll/api/mod/Manifest.h"

namespace pier::mod_control
{
    /**
     * One `mods/<dir>/manifest.json` on disk carrying `"type": "pier"`.
     *
     * Read from the file directly instead of reusing ll::mod::Manifest, which has no
     * place for `reloadSafe`. The LeviLamina reflection deserializer walks the
     * members of the struct and looks each one up in the JSON. It never walks the
     * keys of the JSON, so an extra top-level field in a manifest is invisible to
     * LeviLamina and does not affect normal startup loading.
     */
    struct Candidate
    {
        std::string name;
        std::string entry;
        std::string version;
        std::vector<std::string> dependencies;
        /** Top-level `"reload_safe": true` in manifest.json. */
        bool reloadSafe = false;
        /** Non-empty when the manifest exists but cannot be used.
         *  Named `problem` rather than `error` so that a call site does not confuse
         *  it with the error channel of ll::Expected. */
        std::string problem;
    };

    /** Rescans mods/ on disk. `/pier list` calls this, and it is the only way a
     *  directory added after server startup becomes visible.
     *
     *  Deliberately uncached, so every query hits the disk. A cache buys nothing
     *  here, since these commands are typed by hand and never run in a loop, and it
     *  could report a stale entry point for a freshly rebuilt dll, which is the very
     *  case this command exists to support. */
    std::vector<Candidate> rescan();

    /** Reads one by name from disk, bypassing any cache so it is never stale. */
    ll::Expected<Candidate> readCandidate(std::string_view name);

    /** Builds an ll::mod::Manifest from manifest.json. */
    ll::Expected<ll::mod::Manifest> readManifest(std::string_view name);

    /**
     * Every loaded mod that transitively declares `name` in `dependencies`, in an
     * order that is safe to unload in, with a dependent ahead of what it depends on.
     * Native C++ mods are included, since one can depend on a pier mod just as
     * easily as another pier mod can.
     */
    std::vector<std::string> dependentsOf(std::string_view name);

    /** `name` is loaded and owned by this host, which makes it a pier mod. */
    bool isHostedMod(std::string_view name);

    /** `name` is loaded by any manager. */
    bool isLoadedAnywhere(std::string_view name);

    /** Registers `/pier`. Server builds only, called once from enable(). */
    void registerCommand();
} // namespace pier::mod_control

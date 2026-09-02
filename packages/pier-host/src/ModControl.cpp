/**
 * ModControl.cpp: runtime load, unload and reload of pier mods, and the /pier command
 * that drives them.
 *
 * Three constraints shape this file. First, LeviLamina has no public runtime load API.
 * loadMod, unloadMod, enableMod and disableMod on ModManagerRegistry are all private
 * and only the inherited protected ModManager interface is reachable, wrapped in
 * ModHost::controlLoad and controlUnload. A mod brought up here lives in this manager's
 * own table and not in the LeviLamina dependency graph, so dependency checking reads
 * manifests directly. Second, no dll is copied. A reload is a state reset that reruns
 * pier_main and rereads config, not a hot code swap. Replacing a rebuilt dll goes
 * through unload, rebuild, load, and that path is subject to Windows FreeLibrary
 * reference counting, see checkImageSwapped(). Third, reload is open only to mods
 * declaring "reload_safe": true, and unload is refused while dependents exist unless
 * --cascade is given.
 */
#include "pier/host/mod_control.h"

#include <algorithm>
#include <deque>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ll/api/Config.h" // Pulls in nlohmann/json.hpp
#include "ll/api/io/FileUtils.h"
#include "ll/api/mod/Mod.h"
#include "ll/api/mod/ModManagerRegistry.h"
#include "ll/api/reflection/Deserialization.h"
#include "ll/api/utils/StringUtils.h"

#ifndef PIER_BUILD_CLIENT
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/runtime/ParamKind.h"
#include "ll/api/command/runtime/RuntimeCommand.h"
#include "ll/api/command/runtime/RuntimeOverload.h"
#include "ll/api/event/EventBus.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#endif

#include "sdk/abi.h"

#include "pier/host/api_table.h"
#include "pier/host/hosted_mod.h"
#include "pier/host/mod_host.h"
#include "pier/host/spi.h"
#include "pier/support/log.h"
#include "pier/support/str.h"

namespace pier::mod_control
{
    namespace fs = std::filesystem;

    namespace
    {
        fs::path manifestPathOf(std::string_view name)
        {
            return ll::mod::getModsRoot() / ll::string_utils::sv2u8sv(name) / u8"manifest.json";
        }

        /** Parses one manifest.json into a Candidate. Failures go to `problem`. */
        Candidate parseCandidate(fs::path const& file, std::string const& dirName)
        {
            Candidate c;
            c.name = dirName;

            auto text = ll::file_utils::readFile(file);
            if (!text)
            {
                c.problem = "manifest.json could not be read";
                return c;
            }
            nlohmann::json j;
            try
            {
                // (text, cb, allow_exceptions, ignore_comments), the same call shape
                // the LeviLamina config loader itself uses.
                j = nlohmann::json::parse(*text, nullptr, true, true);
            }
            catch (std::exception const& e)
            {
                c.problem = std::string{"manifest.json is not valid JSON: "} + e.what();
                return c;
            }
            if (!j.is_object())
            {
                c.problem = "manifest.json top level is not an object";
                return c;
            }
            if (!j.contains("type") || !j["type"].is_string()
                || j["type"].get<std::string>() != ModHostName)
            {
                c.problem = "not-pier"; // Sentinel, filtered silently and not reported
                return c;
            }
            if (j.contains("name") && j["name"].is_string())
            {
                c.name = j["name"].get<std::string>();
            }
            if (j.contains("entry") && j["entry"].is_string())
            {
                c.entry = j["entry"].get<std::string>();
            }
            else
            {
                c.problem = "manifest.json has no entry";
                return c;
            }
            if (j.contains("version") && j["version"].is_string())
            {
                c.version = j["version"].get<std::string>();
            }
            // reload_safe is a plain bool at the top level. The LeviLamina
            // deserializer walks the members of the Manifest struct and looks each one
            // up in the JSON, never the keys of the JSON, so this extra field is
            // invisible to it and does not affect normal startup loading.
            if (j.contains("reload_safe"))
            {
                if (j["reload_safe"].is_boolean())
                {
                    c.reloadSafe = j["reload_safe"].get<bool>();
                }
                else
                {
                    c.problem = "reload_safe must be true or false";
                }
            }
            if (j.contains("dependencies") && j["dependencies"].is_array())
            {
                for (auto const& d : j["dependencies"])
                {
                    if (d.is_object() && d.contains("name") && d["name"].is_string())
                    {
                        c.dependencies.push_back(d["name"].get<std::string>());
                    }
                    else if (d.is_string())
                    {
                        c.dependencies.push_back(d.get<std::string>());
                    }
                }
            }
            // getModsRoot()/<dir>/entry resolves by directory name, so a mod whose
            // manifest "name" disagrees with its directory cannot be loaded by path at
            // all. Catching it here keeps it from surfacing later as an opaque
            // file-not-found.
            if (c.name != dirName)
            {
                c.problem =
                    "manifest name (\"" + c.name + "\") does not match directory (\"" + dirName + "\")";
            }
            return c;
        }

        /** Name of each loaded mod mapped to the dependencies it declares. */
        std::unordered_map<std::string, std::vector<std::string>> loadedDependencyMap()
        {
            std::unordered_map<std::string, std::vector<std::string>> out;

            auto absorb = [&out](ll::mod::Mod& mod)
            {
                auto const& mf = mod.getManifest();
                auto& deps = out[mod.getName()];
                if (mf.dependencies)
                {
                    for (auto const& d : *mf.dependencies)
                    {
                        deps.push_back(d.name);
                    }
                }
            };

            // The registry covers everything LeviLamina loaded at startup, native C++
            // mods included, which matters because a C++ mod can depend on a pier mod
            // just as easily as another pier mod can.
            for (auto& mod : ll::mod::ModManagerRegistry::getInstance().mods())
            {
                absorb(mod);
            }
            // Union with this host's own table. A mod brought up by /pier load may be
            // outside the registry's view and must not be lost here.
            if (auto* mgr = ModHost::instance())
            {
                for (auto& mod : mgr->mods())
                {
                    absorb(mod);
                }
            }
            return out;
        }
    } // namespace

    std::vector<Candidate> rescan()
    {
        std::vector<Candidate> found;
        std::error_code ec;
        auto root = ll::mod::getModsRoot();
        if (!fs::is_directory(root, ec)) return found;
        for (auto const& entry : fs::directory_iterator(root, ec))
        {
            if (ec) break;
            if (!entry.is_directory(ec)) continue;
            auto file = entry.path() / u8"manifest.json";
            if (!fs::is_regular_file(file, ec)) continue;

            auto dirName = ll::string_utils::u8str2str(entry.path().filename().u8string());
            auto c = parseCandidate(file, dirName);
            if (c.problem == "not-pier") continue; // Belongs to another manager
            found.push_back(std::move(c));
        }
        std::sort(found.begin(), found.end(),
                  [](Candidate const& a, Candidate const& b) { return a.name < b.name; });
        return found;
    }

    ll::Expected<Candidate> readCandidate(std::string_view name)
    {
        // Deliberately bypasses any cache. `/pier load` must see the manifest as it
        // is on disk right now, not as it was at the last `/pier list`.
        auto file = manifestPathOf(name);
        std::error_code ec;
        if (!fs::is_regular_file(file, ec))
        {
            return ll::makeStringError("mods/" + std::string(name) + "/manifest.json not found");
        }
        auto c = parseCandidate(file, std::string(name));
        if (c.problem == "not-pier")
        {
            return ll::makeStringError(
                "'" + std::string(name) + "' is not a \"type\": \"" + std::string(ModHostName) + "\" mod"
            );
        }
        if (!c.problem.empty())
        {
            return ll::makeStringError("'" + std::string(name) + "': " + c.problem);
        }
        return c;
    }

    ll::Expected<ll::mod::Manifest> readManifest(std::string_view name)
    {
        auto file = manifestPathOf(name);
        auto text = ll::file_utils::readFile(file);
        if (!text)
        {
            return ll::makeStringError("mods/" + std::string(name) + "/manifest.json could not be read");
        }
        nlohmann::json j;
        try
        {
            j = nlohmann::json::parse(*text, nullptr, true, true);
        }
        catch (std::exception const& e)
        {
            return ll::makeStringError(std::string{"manifest.json could not be parsed: "} + e.what());
        }
        ll::mod::Manifest manifest;
        // Reuses the LeviLamina reflection deserializer, so a hot-loaded mod gets a
        // manifest identical in meaning to one loaded at startup.
        if (auto e = ll::reflection::deserialize<ll::mod::Manifest>(manifest, j); !e)
        {
            return ll::forwardError(e.error());
        }
        return manifest;
    }

    bool isHostedMod(std::string_view name)
    {
        auto* mgr = ModHost::instance();
        return mgr && mgr->hasMod(name);
    }

    bool isLoadedAnywhere(std::string_view name)
    {
        if (isHostedMod(name)) return true;
        return ll::mod::ModManagerRegistry::getInstance().hasMod(name);
    }

    std::vector<std::string> dependentsOf(std::string_view name)
    {
        auto deps = loadedDependencyMap();
        std::string const target{name};

        // Reverse edges, from a dependency to the mods that depend on it
        std::unordered_map<std::string, std::vector<std::string>> dependedBy;
        for (auto const& [mod, list] : deps)
        {
            for (auto const& d : list)
            {
                dependedBy[d].push_back(mod);
            }
        }

        // Transitive closure of everything that needs target, directly or not
        std::unordered_set<std::string> affected;
        std::deque<std::string> queue{target};
        while (!queue.empty())
        {
            auto cur = queue.front();
            queue.pop_front();
            auto it = dependedBy.find(cur);
            if (it == dependedBy.end()) continue;
            for (auto const& dep : it->second)
            {
                if (dep == target) continue;
                if (affected.insert(dep).second) queue.push_back(dep);
            }
        }

        // Order so that a mod always precedes what it depends on, which is the order
        // an unload must follow. Kahn's algorithm on the induced subgraph. Naive BFS
        // depth gets diamond dependencies wrong.
        std::unordered_map<std::string, int> indeg;
        for (auto const& m : affected) indeg[m] = 0;
        for (auto const& m : affected)
        {
            for (auto const& d : deps[m])
            {
                if (affected.count(d)) indeg[d]++;
            }
        }
        std::vector<std::string> ready;
        for (auto const& [m, n] : indeg)
        {
            if (n == 0) ready.push_back(m);
        }
        std::sort(ready.begin(), ready.end()); // Deterministic output

        std::vector<std::string> ordered;
        while (!ready.empty())
        {
            auto cur = ready.back();
            ready.pop_back();
            ordered.push_back(cur);
            for (auto const& d : deps[cur])
            {
                if (!affected.count(d)) continue;
                if (--indeg[d] == 0) ready.push_back(d);
            }
        }
        // A dependency cycle leaves nodes that were never emitted. They are appended
        // at the end instead of dropped silently, because the caller still needs to
        // know they exist.
        for (auto const& m : affected)
        {
            if (std::find(ordered.begin(), ordered.end(), m) == ordered.end())
            {
                ordered.push_back(m);
            }
        }
        return ordered;
    }

#ifndef PIER_BUILD_CLIENT
    namespace
    {
        /**
         * Base address of each mod at the time it was last unloaded.
         *
         * The development workflow is unload, rebuild, load, which spans two commands
         * and is invisible to the checks on the reload path. Keeping the address here
         * lets `/pier load` answer the one question that matters after a rebuild,
         * whether the new dll was really mapped in or Windows handed back the old
         * image.
         */
        std::unordered_map<std::string, void const*> gLastUnloadBase;

        /**
         * FreeLibrary on Windows is reference counted and does not force an unmap. An
         * unfinished TLS destructor, a live COM object or a CRT pointer into the image
         * each keep the dll mapped, and the next LoadLibrary hands back the same base
         * address. The freshly built dll on disk is then never read and the server
         * keeps running the old code silently.
         *
         * Comparing the base address across one unload and load is the only cheap way
         * to catch this, and it is the exact failure mode of the rebuild-then-load
         * workflow.
         */
        void checkImageSwapped(CommandOutput& output, std::string const& name, void const* before)
        {
            auto* mgr = ModHost::instance();
            if (!mgr || before == nullptr) return;
            void const* after = mgr->moduleBase(name);
            if (after != nullptr && after == before)
            {
                output.error(
                    "'" + name + "' kept the same dll base address after reload; Windows "
                    "did not truly unload the image, because FreeLibrary is reference "
                    "counted. After a rebuild the server is still running the old code. "
                    "Common causes are mod-spawned threads that were not joined, TLS "
                    "destructors that did not finish, and handles left open. Restarting "
                    "the server is the only reliable way to make new code take effect."
                );
            }
        }

        struct ParsedArgs
        {
            std::string sub;
            std::string name;
            bool cascade = false;
            bool unknownFlag = false;
            std::string unknownFlagText;
        };

        ParsedArgs parseArgs(std::string const& raw)
        {
            ParsedArgs a;
            std::vector<std::string> tokens;
            std::string cur;
            for (char ch : raw)
            {
                if (ch == ' ' || ch == '\t')
                {
                    if (!cur.empty()) tokens.push_back(std::exchange(cur, {}));
                }
                else
                {
                    cur += ch;
                }
            }
            if (!cur.empty()) tokens.push_back(std::move(cur));

            for (auto const& t : tokens)
            {
                if (t.rfind("--", 0) == 0)
                {
                    if (t == "--cascade")
                    {
                        a.cascade = true;
                    }
                    else
                    {
                        a.unknownFlag = true;
                        a.unknownFlagText = t;
                    }
                }
                else if (a.sub.empty())
                {
                    a.sub = t;
                }
                else if (a.name.empty())
                {
                    a.name = t;
                }
            }
            return a;
        }

        void cmdList(CommandOutput& output)
        {
            // A real rescan of the disk and not a cache dump. A mod directory added or
            // rebuilt after startup becomes visible through this command.
            auto found = rescan();
            auto* mgr = ModHost::instance();

            output.success("pier mods (mods/ rescanned)");
            if (found.empty())
            {
                output.success("no \"type\": \"" + std::string(ModHostName) + "\" mod found");
                return;
            }
            size_t loaded = 0;
            for (auto const& c : found)
            {
                bool const isLoaded = mgr && mgr->hasMod(c.name);
                if (isLoaded) loaded++;

                std::string line = c.name;
                if (!c.version.empty()) line += " v" + c.version;
                line += isLoaded ? "  [loaded" : "  [not loaded";
                if (isLoaded)
                {
                    auto mod = mgr->getMod(c.name);
                    line += (mod && mod->isEnabled()) ? "/enabled" : "/disabled";
                }
                line += "]";
                line += c.reloadSafe ? "  [reload_safe]" : "  [not reload_safe]";
                if (!c.problem.empty()) line += "  problem: " + c.problem;
                output.success(line);
            }
            output.success(
                "scanned " + std::to_string(found.size()) + " mod(s), " + std::to_string(loaded) + " loaded"
            );
        }

        void cmdLoad(CommandOutput& output, std::string const& name)
        {
            auto* mgr = ModHost::instance();
            if (!mgr)
            {
                output.error("pier host is not ready yet");
                return;
            }
            if (mgr->hasMod(name))
            {
                output.error("'" + name + "' is already loaded; to replace its code run /pier unload " + name);
                return;
            }
            if (isLoadedAnywhere(name))
            {
                output.error("'" + name + "' is already loaded by another mod manager and is not taken over here");
                return;
            }

            auto cand = readCandidate(name);
            if (!cand)
            {
                output.error(cand.error().message());
                return;
            }
            // Refuse on a missing dependency instead of loading and waiting for the
            // failure. The error would then surface from inside the mod and read far
            // worse than this one line.
            std::vector<std::string> missing;
            for (auto const& d : cand->dependencies)
            {
                if (!isLoadedAnywhere(d)) missing.push_back(d);
            }
            if (!missing.empty())
            {
                std::string list;
                for (auto const& m : missing)
                {
                    if (!list.empty()) list += ", ";
                    list += m;
                }
                output.error("'" + name + "' has dependencies that are not loaded: " + list);
                return;
            }

            auto manifest = readManifest(name);
            if (!manifest)
            {
                output.error(manifest.error().message());
                return;
            }
            if (auto e = mgr->controlLoad(std::move(*manifest)); !e)
            {
                output.error("loading '" + name + "' failed: " + e.error().message());
                return;
            }
            output.success("loaded and enabled '" + name + "'");
            // If this name was unloaded during this session, the base address tells
            // whether the rebuilt dll really replaced the old one or Windows handed
            // the old image back.
            if (auto it = gLastUnloadBase.find(name); it != gLastUnloadBase.end())
            {
                checkImageSwapped(output, name, it->second);
                gLastUnloadBase.erase(it);
            }
            if (!cand->reloadSafe)
            {
                output.success(
                    "note: '" + name + "' does not declare \"reload_safe\": true in its "
                    "manifest, so /pier reload is not available for it and unload plus "
                    "load is the only path"
                );
            }
        }

        /** Shared unload path. Returns false when nothing was done. */
        bool doUnload(CommandOutput& output, std::string const& name, bool cascade,
                      std::vector<std::string>* unloadedOut)
        {
            auto* mgr = ModHost::instance();
            if (!mgr || !mgr->hasMod(name))
            {
                output.error("'" + name + "' is not loaded as a pier mod");
                return false;
            }

            auto dependents = dependentsOf(name);
            if (!dependents.empty())
            {
                std::string list;
                for (auto const& d : dependents)
                {
                    if (!list.empty()) list += ", ";
                    list += d;
                    if (!isHostedMod(d)) list += "(C++)";
                }
                if (!cascade)
                {
                    // Refusing is the default. Unloading it from under a dependent
                    // leaves that dependent holding function pointers into a freed
                    // dylib, which only shows up on the next call into it.
                    output.error(
                        "refusing to unload '" + name + "', mods still depend on it: " + list
                        + ". Add --cascade to unload them together."
                    );
                    return false;
                }
                // A cascade can only take down mods this host manages. A native C++
                // mod belongs to another manager and there is no path to unload it
                // from here.
                std::vector<std::string> foreign;
                for (auto const& d : dependents)
                {
                    if (!isHostedMod(d)) foreign.push_back(d);
                }
                if (!foreign.empty())
                {
                    std::string flist;
                    for (auto const& f : foreign)
                    {
                        if (!flist.empty()) flist += ", ";
                        flist += f;
                    }
                    output.error(
                        "--cascade cannot proceed either, some dependents are not managed "
                        "by this host (" + flist
                        + ") and cannot be unloaded from here; handle them manually or restart."
                    );
                    return false;
                }
            }

            // Dependents first, then the target. The order from dependentsOf() already
            // places a mod ahead of everything it depends on.
            std::vector<std::string> order = dependents;
            order.push_back(name);

            for (auto const& m : order)
            {
                if (!mgr->hasMod(m)) continue;
                gLastUnloadBase[m] = mgr->moduleBase(m);
                if (auto e = mgr->controlUnload(m); !e)
                {
                    output.error("unloading '" + m + "' failed: " + e.error().message());
                    output.error("mods already unloaded are not restored automatically; check the server state.");
                    return false;
                }
                output.success("unloaded '" + m + "'");
                if (unloadedOut) unloadedOut->push_back(m);
            }
            return true;
        }

        void cmdUnload(CommandOutput& output, std::string const& name, bool cascade)
        {
            if (doUnload(output, name, cascade, nullptr))
            {
                output.success(
                    "note: to switch to a rebuilt dll, replace the file now, run /pier list "
                    "to refresh, then /pier load " + name
                );
            }
        }

        void cmdReload(CommandOutput& output, std::string const& name, bool cascade)
        {
            auto* mgr = ModHost::instance();
            if (!mgr || !mgr->hasMod(name))
            {
                output.error("'" + name + "' is not loaded as a pier mod");
                return;
            }

            auto cand = readCandidate(name);
            if (!cand)
            {
                output.error(cand.error().message());
                return;
            }
            // The gate. reload is offered only to mods that declare they survive it.
            // Everything else takes the cold path.
            if (!cand->reloadSafe)
            {
                output.error(
                    "'" + name + "' does not declare \"reload_safe\": true in manifest.json, so reload is refused."
                );
                output.error(
                    "reload reruns " PIER_MAIN_SYMBOL ", so the mod must join every thread, "
                    "close every handle and cancel every timer through schedule_cancel in "
                    "on_unload, and its global state must be re-enterable. Add the field "
                    "only once that holds. Until then use /pier unload " + name
                    + " followed by /pier load " + name + "."
                );
                return;
            }

            // A cascade reloads the dependents too, and what they go through is the
            // same reload the gate above guards, so each one is checked as well and an
            // unmarked mod cannot slip in through the back door.
            if (cascade)
            {
                for (auto const& dep : dependentsOf(name))
                {
                    if (!isHostedMod(dep)) continue; // Reported later by doUnload
                    auto dc = readCandidate(dep);
                    if (!dc || !dc->reloadSafe)
                    {
                        output.error(
                            "'" + dep + "' in the cascade does not declare \"reload_safe\": "
                            "true and cannot be reloaded with it. Use /pier unload " + name
                            + " --cascade instead, then load each one."
                        );
                        return;
                    }
                }
            }

            std::vector<std::string> unloaded;
            if (!doUnload(output, name, cascade, &unloaded)) return;

            // Bring them back in the reverse of the order they went down.
            std::reverse(unloaded.begin(), unloaded.end());
            bool allOk = true;
            for (auto const& m : unloaded)
            {
                auto manifest = readManifest(m);
                if (!manifest)
                {
                    output.error("reloading '" + m + "' failed: " + manifest.error().message());
                    allOk = false;
                    break;
                }
                if (auto e = mgr->controlLoad(std::move(*manifest)); !e)
                {
                    output.error("reloading '" + m + "' failed: " + e.error().message());
                    output.error("'" + m + "' is now unloaded; fix it, then run /pier load " + m + " to bring it up.");
                    allOk = false;
                    break;
                }
                // A reload is a state reset, so an unchanged base address on this path
                // is the expected result and not a symptom. Running checkImageSwapped
                // here would warn every time. That check belongs to the unload,
                // rebuild, load path, where an unchanged base address does mean the new
                // dll was never read. The stored address is dropped so that a later
                // /pier load of the same mod does not compare against it.
                gLastUnloadBase.erase(m);
                output.success("reloaded '" + m + "'");
            }
            if (allOk)
            {
                output.success(
                    "'" + name + "' reloaded: state reset and config reread, dll code unchanged"
                );
            }
        }

        /** /pier events lists every subscribable event id in one place, both the ones
         *  in the LeviLamina dynamic registry and the ones synthesized by each event
         *  provider such as hooks and command events. Anyone tracking down what an
         *  event is called needs the complete list. */
        void cmdEvents(CommandOutput& output)
        {
            size_t n = 0;
            for (auto&& [modName, id] : ll::event::EventBus::getInstance().events())
            {
                output.success(std::string{id.name} + "  (from " + std::string{modName} + ")");
                n++;
            }
            struct Ctx
            {
                CommandOutput* out;
                size_t* n;
            } ctx{&output, &n};
            spi::forEachEventProvider(
                [](spi::EventProvider const& p, void* raw)
                {
                    auto* c = static_cast<Ctx*>(raw);
                    struct SinkCtx
                    {
                        CommandOutput* out;
                        size_t* n;
                        std::string_view provider;
                    } sc{c->out, c->n, p.name};
                    p.list(
                        &sc,
                        [](void* rawSink, PierStr s)
                        {
                            auto* k = static_cast<SinkCtx*>(rawSink);
                            k->out->success(
                                toString(s) + "  (synthetic, provider " + std::string(k->provider) + ")"
                            );
                            (*k->n)++;
                        }
                    );
                    return false; // Visit every provider
                },
                &ctx
            );
            output.success("total " + std::to_string(n) + " event(s)");
        }

        void cmdAbi(CommandOutput& output)
        {
            auto const* api = bridgeApi();
            output.success(
                "Pier ABI v" + std::to_string(PIER_ABI_VERSION) + " (minimum v"
                + std::to_string(PIER_ABI_MIN_SUPPORTED) + "), api table "
                + std::to_string(api->struct_size) + " bytes, target "
                + ((api->host_flags & PIER_FLAG_CLIENT) ? "client" : "server")
            );
        }
    } // namespace

    void registerCommand()
    {
        using namespace ll::command;
        auto& handle = CommandRegistrar::getServerInstance().getOrCreateCommand(
            "pier",
            "pier mod control: list / load / unload / reload / events / abi",
            CommandPermissionLevel::Host
        );
        handle.runtimeOverload().optional("args", ParamKind::RawText).execute(
            [](CommandOrigin const&, CommandOutput& output, RuntimeCommand const& rt)
            {
                std::string raw;
                if (auto const& p = rt["args"]; p.hold(ParamKind::RawText))
                {
                    raw = p.get<ParamKind::RawText>().mText;
                }
                auto args = parseArgs(raw);

                if (args.unknownFlag)
                {
                    output.error("unrecognized argument " + args.unknownFlagText + " (only --cascade is supported)");
                    return;
                }

                auto needName = [&output, &args](char const* verb)
                {
                    if (args.name.empty())
                    {
                        output.error(std::string{"usage: /pier "} + verb + " <mod_name>");
                        return false;
                    }
                    return true;
                };

                if (args.sub == "list" || args.sub.empty())
                {
                    cmdList(output);
                    if (args.sub.empty())
                    {
                        output.success(
                            "usage: /pier list | load <n> | unload <n> [--cascade] | "
                            "reload <n> [--cascade] | events | abi"
                        );
                    }
                }
                else if (args.sub == "load")
                {
                    if (needName("load")) cmdLoad(output, args.name);
                }
                else if (args.sub == "unload")
                {
                    if (needName("unload")) cmdUnload(output, args.name, args.cascade);
                }
                else if (args.sub == "reload")
                {
                    if (needName("reload")) cmdReload(output, args.name, args.cascade);
                }
                else if (args.sub == "events")
                {
                    cmdEvents(output);
                }
                else if (args.sub == "abi")
                {
                    cmdAbi(output);
                }
                else
                {
                    output.error(
                        "unknown subcommand '" + args.sub
                        + "'. usage: /pier list | load <n> | unload <n> [--cascade] | "
                          "reload <n> [--cascade] | events | abi"
                    );
                }
            }
        );
        hostLogger().debug("[host] /pier command registered");
    }
#else
    void registerCommand() {}
#endif // !PIER_BUILD_CLIENT
} // namespace pier::mod_control

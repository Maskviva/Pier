/** runtime/data/KvDbApi.cpp: the key-value database.
 *
 * The only resource handle in the ABI, so the ownership rules are stated explicitly.
 * The host allocates each KeyValueDB into a registry accounted per mod, kvdb_close or
 * a Drop on the SDK side closes it, and unload force-closes whatever is left and
 * warns. The whole family shares one mutex and is thread safe as a whole per the
 * contract, since a mod may open its database from a background task. Paths are
 * confined to the mod's own data directory. */
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ll/api/data/KeyValueDB.h"
#include "ll/api/utils/ErrorUtils.h"

#include "sdk/abi.h"

#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        struct KvEntry
        {
            HostedMod* mod = nullptr;
            std::unique_ptr<ll::data::KeyValueDB> db;
        };

        std::mutex gKvMutex;
        std::unordered_map<uint64_t, KvEntry> gKvDbs;
        uint64_t gNextKvId = 1;

        KvEntry* entryOf(PierKvDbHandle h)
        {
            auto id = reinterpret_cast<uint64_t>(h);
            auto it = gKvDbs.find(id);
            return it == gKvDbs.end() ? nullptr : &it->second;
        }

        /** Confines `rel` to the mod data directory. An escape attempt yields an
         *  empty path. */
        std::filesystem::path confinedPath(HostedMod* mod, std::string_view rel)
        {
            if (rel.empty()) return {};
            std::filesystem::path p{std::u8string{rel.begin(), rel.end()}};
            // Refusing only is_absolute() is not enough on Windows. Both `\evil`,
            // which has a root directory but no drive, and `D:evil`, which has a drive
            // but no root directory, count as relative, while operator/ discards
            // everything after the left root for the first and replaces the path
            // entirely for the second, so dataDir / "\\evil" yields C:\evil. Any
            // relative path carrying a root name or a root directory is refused.
            if (p.is_absolute() || p.has_root_name() || p.has_root_directory()) return {};
            for (auto const& part : p)
            {
                if (part == "..") return {};
            }
            // A prefix check as a backstop. After normalization it must still lie
            // inside the data directory.
            auto const base = mod->getDataDir().lexically_normal();
            auto const full = (mod->getDataDir() / p).lexically_normal();
            auto const baseStr = base.generic_u8string();
            auto const fullStr = full.generic_u8string();
            if (fullStr.size() <= baseStr.size() || fullStr.compare(0, baseStr.size(), baseStr) != 0)
            {
                return {};
            }
            if (fullStr[baseStr.size()] != u8'/' && !baseStr.empty() && baseStr.back() != u8'/') return {};
            return full;
        }

        PierKvDbHandle api_kvdb_open(PierModHandle modHandle, PierStr path, bool createIfMissing)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod) return nullptr;
                auto full = confinedPath(mod, sv(path));
                if (full.empty())
                {
                    mod->getLogger().error("[kvdb] open refused, the path must be relative and stay inside the mod data directory");
                    return nullptr;
                }
                try
                {
                    std::error_code ec;
                    std::filesystem::create_directories(full.parent_path(), ec);
                    // Four-argument constructor: (path, createIfMiss, fixIfError,
                    // bloomFilterBit), where 0 builds no bloom filter.
                    auto db = std::make_unique<ll::data::KeyValueDB>(full, createIfMissing, false, 0);
                    std::lock_guard lock(gKvMutex);
                    uint64_t id = gNextKvId++;
                    gKvDbs[id] = KvEntry{mod, std::move(db)};
                    return reinterpret_cast<PierKvDbHandle>(id);
                }
                catch (...)
                {
                    mod->getLogger().error("[kvdb] open failed for '{}'", sv(path));
                    ll::error_utils::printCurrentException(hostLogger());
                    return nullptr;
                }
            PIER_API_GUARD_END
        }

        void api_kvdb_close(PierKvDbHandle h)
        {
            PIER_API_GUARD_BEGIN
                // The database object is destroyed outside the lock. Closing LevelDB
                // can take time and must not hold the registry lock.
                std::unique_ptr<ll::data::KeyValueDB> dying;
                {
                    std::lock_guard lock(gKvMutex);
                    auto it = gKvDbs.find(reinterpret_cast<uint64_t>(h));
                    if (it == gKvDbs.end()) return;
                    dying = std::move(it->second.db);
                    gKvDbs.erase(it);
                }
            PIER_API_GUARD_END_VOID
        }

        bool api_kvdb_get(PierKvDbHandle h, PierStr key, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return false;
                // The value is copied out under the lock and the sink is called after
                // it is released. The sink is mod code and may call any kvdb_* again,
                // which would self-deadlock a non-recursive mutex.
                std::optional<std::string> value;
                {
                    std::lock_guard lock(gKvMutex);
                    auto* e = entryOf(h);
                    if (!e) return false;
                    value = e->db->get(sv(key));
                }
                if (!value) return false;
                sink(ctx, ps(*value));
                return true;
            PIER_API_GUARD_END
        }

        bool api_kvdb_set(PierKvDbHandle h, PierStr key, PierStr value)
        {
            PIER_API_GUARD_BEGIN
                std::lock_guard lock(gKvMutex);
                auto* e = entryOf(h);
                if (!e) return false;
                return e->db->set(sv(key), sv(value));
            PIER_API_GUARD_END
        }

        bool api_kvdb_del(PierKvDbHandle h, PierStr key)
        {
            PIER_API_GUARD_BEGIN
                std::lock_guard lock(gKvMutex);
                auto* e = entryOf(h);
                if (!e) return false;
                return e->db->del(sv(key));
            PIER_API_GUARD_END
        }

        bool api_kvdb_has(PierKvDbHandle h, PierStr key)
        {
            PIER_API_GUARD_BEGIN
                std::lock_guard lock(gKvMutex);
                auto* e = entryOf(h);
                if (!e) return false;
                return e->db->has(sv(key));
            PIER_API_GUARD_END
        }

        bool api_kvdb_is_empty(PierKvDbHandle h)
        {
            PIER_API_GUARD_BEGIN
                std::lock_guard lock(gKvMutex);
                auto* e = entryOf(h);
                if (!e) return true;
                return e->db->empty();
            PIER_API_GUARD_END
        }

        void api_kvdb_iter(PierKvDbHandle h, void* ctx, PierKvSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return;
                // Snapshot first, then call back, for the reason kvdb_get gives. The
                // snapshot costs one full copy of the database, which is what lets mod
                // code such as iterating and clearing expired keys exist safely.
                std::vector<std::pair<std::string, std::string>> snapshot;
                {
                    std::lock_guard lock(gKvMutex);
                    auto* e = entryOf(h);
                    if (!e) return;
                    for (auto&& [key, value] : e->db->iter())
                    {
                        snapshot.emplace_back(std::string{key}, std::string{value});
                    }
                }
                for (auto const& [key, value] : snapshot)
                {
                    sink(ctx, ps(key), ps(value));
                }
            PIER_API_GUARD_END_VOID
        }

        /** Teardown at stage 70. Force-closes the databases this mod left open and
         *  warns. */
        void teardown(HostedMod* mod)
        {
            std::lock_guard lock(gKvMutex);
            size_t leaked = 0;
            for (auto it = gKvDbs.begin(); it != gKvDbs.end();)
            {
                if (it->second.mod == mod)
                {
                    it = gKvDbs.erase(it);
                    ++leaked;
                }
                else
                {
                    ++it;
                }
            }
            if (leaked > 0)
            {
                mod->getLogger().warn("[kvdb] force-closed {} database(s) still open at unload", leaked);
            }
        }

        void fill(PierApi& api)
        {
            api.kvdb_open = &api_kvdb_open;
            api.kvdb_close = &api_kvdb_close;
            api.kvdb_get = &api_kvdb_get;
            api.kvdb_set = &api_kvdb_set;
            api.kvdb_del = &api_kvdb_del;
            api.kvdb_has = &api_kvdb_has;
            api.kvdb_is_empty = &api_kvdb_is_empty;
            api.kvdb_iter = &api_kvdb_iter;
        }

        spi::SlotPackReg regSlots{{"kvdb", &fill}};
        spi::TeardownReg regDown{{70, "kvdb", &teardown}};
    } // namespace
} // namespace pier::api_impl

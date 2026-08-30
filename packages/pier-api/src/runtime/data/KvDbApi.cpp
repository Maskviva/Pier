/** runtime/data/KvDbApi.cpp —— 键值数据库。
 *
 * ABI 里唯一的资源型句柄，所以所有权规则写明白：宿主把每个 KeyValueDB
 * new 进按模组记账的注册表；kvdb_close（或 SDK 侧的 Drop）关闭它；卸载
 * 时强制关掉剩下的并告警。整族操作共一把互斥锁，按契约整体线程安全 ——
 * 模组可能从后台任务里打自己的库。路径被圈禁在模组自己的数据目录里。 */
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

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

        /** 把 `rel` 圈在模组数据目录里；逃逸企图一律给空。 */
        std::filesystem::path confinedPath(HostedMod* mod, std::string_view rel)
        {
            if (rel.empty()) return {};
            std::filesystem::path p{std::u8string{rel.begin(), rel.end()}};
            if (p.is_absolute()) return {};
            for (auto const& part : p)
            {
                if (part == "..") return {};
            }
            return mod->getDataDir() / p;
        }

        PierKvDbHandle api_kvdb_open(PierModHandle modHandle, PierStr path, bool createIfMissing)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod) return nullptr;
                auto full = confinedPath(mod, sv(path));
                if (full.empty())
                {
                    mod->getLogger().error("kvdb_open：路径必须是相对路径，且不许逃出模组数据目录");
                    return nullptr;
                }
                try
                {
                    std::error_code ec;
                    std::filesystem::create_directories(full.parent_path(), ec);
                    // 四参构造：(path, createIfMiss, fixIfError, bloomFilterBit)；
                    // 0 = 不建布隆过滤器。
                    auto db = std::make_unique<ll::data::KeyValueDB>(full, createIfMissing, false, 0);
                    std::lock_guard lock(gKvMutex);
                    uint64_t id = gNextKvId++;
                    gKvDbs[id] = KvEntry{mod, std::move(db)};
                    return reinterpret_cast<PierKvDbHandle>(id);
                }
                catch (...)
                {
                    mod->getLogger().error("kvdb_open：打不开 '{}'", sv(path));
                    ll::error_utils::printCurrentException(hostLogger());
                    return nullptr;
                }
            PIER_API_GUARD_END
        }

        void api_kvdb_close(PierKvDbHandle h)
        {
            PIER_API_GUARD_BEGIN
                std::lock_guard lock(gKvMutex);
                gKvDbs.erase(reinterpret_cast<uint64_t>(h));
            PIER_API_GUARD_END_VOID
        }

        bool api_kvdb_get(PierKvDbHandle h, PierStr key, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return false;
                std::lock_guard lock(gKvMutex);
                auto* e = entryOf(h);
                if (!e) return false;
                auto value = e->db->get(sv(key));
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
                std::lock_guard lock(gKvMutex);
                auto* e = entryOf(h);
                if (!e) return;
                for (auto&& [key, value] : e->db->iter())
                {
                    sink(ctx, ps(key), ps(value));
                }
            PIER_API_GUARD_END_VOID
        }

        /** 拆除（stage 70）：强制关掉该模组名下没关的库并告警。 */
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
                mod->getLogger().warn("kvdb：卸载时强制关闭了 {} 个仍开着的数据库", leaked);
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

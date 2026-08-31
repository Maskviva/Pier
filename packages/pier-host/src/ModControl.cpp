/**
 * ModControl.cpp —— pier 模组的运行期装 / 卸 / 重载，以及驱动它的 /pier 命令。
 *
 * 三个约束塑造了这份文件。一、LeviLamina 没有公开的运行期装载 API，
 * ModManagerRegistry 的 loadMod/unloadMod/enableMod/disableMod 全是 private，够得
 * 着的只有继承来的 protected ModManager 接口（包在 ModHost::controlLoad/
 * controlUnload 里）；因此这里拉起来的模组活在本宿主管理器自己的表里、不在
 * LeviLamina 的依赖图里，依赖检查只能自己翻 manifest 做。
 *
 * 二、不拷 dll。reload 是状态重置（重跑 pier_main、重读配置），不是代码热替换；换
 * 新编译的 dll 走 unload → 重编 → load，而这条路还受 Windows FreeLibrary 引用计数
 * 的摆布，见 checkImageSwapped()。
 *
 * 三、reload 只对声明了 "reload_safe": true 的模组开放；unload 在有依赖方时拒绝，
 * 除非给 --cascade。
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

#include "ll/api/Config.h" // 带进 nlohmann/json.hpp
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

        /** 把一份 manifest.json 解析成 Candidate；失败写 `problem`。 */
        Candidate parseCandidate(fs::path const& file, std::string const& dirName)
        {
            Candidate c;
            c.name = dirName;

            auto text = ll::file_utils::readFile(file);
            if (!text)
            {
                c.problem = "manifest.json 读不出来";
                return c;
            }
            nlohmann::json j;
            try
            {
                // (text, cb, allow_exceptions, ignore_comments) —— 和
                // LeviLamina 自己的配置装载器同一种调用形状。
                j = nlohmann::json::parse(*text, nullptr, true, true);
            }
            catch (std::exception const& e)
            {
                c.problem = std::string{"manifest.json 不是合法 JSON: "} + e.what();
                return c;
            }
            if (!j.is_object())
            {
                c.problem = "manifest.json 顶层不是对象";
                return c;
            }
            if (!j.contains("type") || !j["type"].is_string()
                || j["type"].get<std::string>() != ModHostName)
            {
                c.problem = "not-pier"; // 哨兵值：静默过滤，不当错误报
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
                c.problem = "manifest.json 缺少 entry";
                return c;
            }
            if (j.contains("version") && j["version"].is_string())
            {
                c.version = j["version"].get<std::string>();
            }
            // reload_safe：顶层的普通 bool。LeviLamina 的反序列化器只遍历
            // Manifest 结构体的成员去 JSON 里找，从不遍历 JSON 的键 ——
            // 所以这个额外字段对它不可见，不会影响正常启动装载。
            if (j.contains("reload_safe"))
            {
                if (j["reload_safe"].is_boolean())
                {
                    c.reloadSafe = j["reload_safe"].get<bool>();
                }
                else
                {
                    c.problem = "reload_safe 必须是 true/false";
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
            // getModsRoot()/<目录>/entry 是按目录名解析的，manifest 里的
            // "name" 和目录不一致的模组按路径根本装不起来。在这里抓住它，
            // 别让它变成后面一句摸不着头脑的「文件不存在」。
            if (c.name != dirName)
            {
                c.problem =
                    "manifest 里的 name (\"" + c.name + "\") 和目录名 (\"" + dirName + "\") 不一致";
            }
            return c;
        }

        /** 每个已装载模组的名字 → 它声明的依赖。 */
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

            // 注册表覆盖 LeviLamina 启动时装的一切，原生 C++ 模组也在内 ——
            // 这很要紧：C++ 模组依赖一个 pier 模组和别的 pier 模组一样容易。
            for (auto& mod : ll::mod::ModManagerRegistry::getInstance().mods())
            {
                absorb(mod);
            }
            // 和本宿主自己的表求并集：/pier load 拉起来的模组可能不在注册表
            // 的视野里，这里绝不能把它弄丢。
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
            if (c.problem == "not-pier") continue; // 别的管理器的事
            found.push_back(std::move(c));
        }
        std::sort(found.begin(), found.end(),
                  [](Candidate const& a, Candidate const& b) { return a.name < b.name; });
        return found;
    }

    ll::Expected<Candidate> readCandidate(std::string_view name)
    {
        // 刻意绕过任何缓存：`/pier load` 必须看到磁盘上此刻的 manifest，
        // 而不是上一次 `/pier list` 时的样子。
        auto file = manifestPathOf(name);
        std::error_code ec;
        if (!fs::is_regular_file(file, ec))
        {
            return ll::makeStringError("找不到 mods/" + std::string(name) + "/manifest.json");
        }
        auto c = parseCandidate(file, std::string(name));
        if (c.problem == "not-pier")
        {
            return ll::makeStringError(
                "'" + std::string(name) + "' 不是 \"type\": \"" + std::string(ModHostName) + "\" 的模组"
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
            return ll::makeStringError("读不出 mods/" + std::string(name) + "/manifest.json");
        }
        nlohmann::json j;
        try
        {
            j = nlohmann::json::parse(*text, nullptr, true, true);
        }
        catch (std::exception const& e)
        {
            return ll::makeStringError(std::string{"manifest.json 解析失败: "} + e.what());
        }
        ll::mod::Manifest manifest;
        // 复用 LeviLamina 自己的反射反序列化：热装载的模组拿到的 manifest
        // 语义和启动装载的逐字节一致。
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

        // 反向边：被依赖者 → 依赖它的
        std::unordered_map<std::string, std::vector<std::string>> dependedBy;
        for (auto const& [mod, list] : deps)
        {
            for (auto const& d : list)
            {
                dependedBy[d].push_back(mod);
            }
        }

        // 一切（间接）需要 target 的传递闭包
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

        // 排成「模组永远在它依赖的东西之前」的顺序 —— 也就是卸载必须遵守的
        // 顺序。对诱导子图跑 Kahn；朴素 BFS 深度在菱形依赖上会排错。
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
        std::sort(ready.begin(), ready.end()); // 输出确定性

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
        // 依赖环会留下没吐出来的节点。补在队尾而不是静默丢掉 ——
        // 调用方仍然需要知道它们的存在。
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
         * 每个模组上一次被卸载时的基址。
         *
         * 开发工作流是 `unload` → 重编 → `load`，横跨两条命令，reload 路径上
         * 的检查看不见它。把地址存在这里，`/pier load` 才能回答重编之后唯一
         * 要紧的问题：新 dll 真被映射进来了，还是 Windows 又把旧映像递了
         * 回来？
         */
        std::unordered_map<std::string, void const*> gLastUnloadBase;

        /**
         * Windows 的 FreeLibrary 是引用计数，不是硬 unmap。TLS 析构没跑完、
         * COM 对象还活着、CRT 留着指进映像的指针 —— 任何一个都会让 dll 保持
         * 映射，下一次 LoadLibrary 递回同一个基址：磁盘上新编译的 dll
         * 根本没被读进来，服务器还在静默地跑旧代码。
         *
         * 跨一次 unload/load 比较基址是抓住这件事唯一便宜的办法，而它正是
         * 「重编然后 /pier load」工作流的确切故障模式。
         */
        void checkImageSwapped(CommandOutput& output, std::string const& name, void const* before)
        {
            auto* mgr = ModHost::instance();
            if (!mgr || before == nullptr) return;
            void const* after = mgr->moduleBase(name);
            if (after != nullptr && after == before)
            {
                output.error(
                    "⚠ '" + name + "' 重新加载后 dll 基址没变 —— Windows 没有真正卸载这个映像"
                    "（FreeLibrary 是引用计数的）。如果你刚重新编译过，服务器现在跑的**还是旧代码**。"
                    "常见原因：模组自己 spawn 的线程没 join、TLS 析构没跑完、还有 handle 没关。"
                    "确认新代码生效的唯一可靠办法是重启服务器。"
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
            // 明确是磁盘重扫，不是缓存倾倒：新加的 / 刚编好的模组目录
            // 就是靠这条命令变得可见的。
            auto found = rescan();
            auto* mgr = ModHost::instance();

            output.success("── pier 模组（已重新扫描 mods/ 目录）──");
            if (found.empty())
            {
                output.success("(没有找到任何 \"type\": \"" + std::string(ModHostName) + "\" 的模组)");
                return;
            }
            size_t loaded = 0;
            for (auto const& c : found)
            {
                bool const isLoaded = mgr && mgr->hasMod(c.name);
                if (isLoaded) loaded++;

                std::string line = c.name;
                if (!c.version.empty()) line += " v" + c.version;
                line += isLoaded ? "  [已加载" : "  [未加载";
                if (isLoaded)
                {
                    auto mod = mgr->getMod(c.name);
                    line += (mod && mod->isEnabled()) ? "/已启用" : "/已禁用";
                }
                line += "]";
                line += c.reloadSafe ? "  [reload_safe]" : "  [不可 reload]";
                if (!c.problem.empty()) line += "  ⚠ " + c.problem;
                output.success(line);
            }
            output.success(
                "共 " + std::to_string(found.size()) + " 个，已加载 " + std::to_string(loaded) + " 个"
            );
        }

        void cmdLoad(CommandOutput& output, std::string const& name)
        {
            auto* mgr = ModHost::instance();
            if (!mgr)
            {
                output.error("pier 宿主还没就绪");
                return;
            }
            if (mgr->hasMod(name))
            {
                output.error("'" + name + "' 已经加载了。要换代码请先 /pier unload " + name);
                return;
            }
            if (isLoadedAnywhere(name))
            {
                output.error("'" + name + "' 已被别的模组管理器加载，这里不接管");
                return;
            }

            auto cand = readCandidate(name);
            if (!cand)
            {
                output.error(cand.error().message());
                return;
            }
            // 依赖缺失就拒绝，而不是装上等它炸 —— 到时候错误从模组内部冒出
            // 来，可读性远不如现在这一句。
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
                output.error("'" + name + "' 的依赖还没加载: " + list);
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
                output.error("加载 '" + name + "' 失败: " + e.error().message());
                return;
            }
            output.success("已加载并启用 '" + name + "'");
            // 这个名字要是在本次会话里卸载过，基址能判断重编的 dll 是
            // 真换上了，还是 Windows 把旧的又递了回来。
            if (auto it = gLastUnloadBase.find(name); it != gLastUnloadBase.end())
            {
                checkImageSwapped(output, name, it->second);
                gLastUnloadBase.erase(it);
            }
            if (!cand->reloadSafe)
            {
                output.success(
                    "提示：'" + name + "' 的 manifest 没写 \"reload_safe\": true，"
                    "所以 /pier reload 对它不开放，只能 unload + load"
                );
            }
        }

        /** 共享卸载路径。什么都没做返回 false。 */
        bool doUnload(CommandOutput& output, std::string const& name, bool cascade,
                      std::vector<std::string>* unloadedOut)
        {
            auto* mgr = ModHost::instance();
            if (!mgr || !mgr->hasMod(name))
            {
                output.error("'" + name + "' 没有被加载（pier 模组）");
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
                    // 默认是拒绝：从依赖方脚下把它卸掉，依赖方手里攥着指向
                    // 已释放 dylib 的函数指针，要到下次调进来才会发现。
                    output.error(
                        "拒绝卸载 '" + name + "'：还有模组依赖它 —— " + list
                        + "。确认要一起卸载就加 --cascade。"
                    );
                    return false;
                }
                // 级联只能放倒本宿主管的模组。原生 C++ 模组归别的管理器，
                // 这里没有任何途径卸载它。
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
                        "--cascade 也做不了：依赖方里有不归本宿主管的模组（" + flist
                        + "），无权卸载它们。请先手动处理，或重启服务器。"
                    );
                    return false;
                }
            }

            // 先依赖方、后目标：dependentsOf() 给出的顺序已经保证
            // 模组在它依赖的一切之前。
            std::vector<std::string> order = dependents;
            order.push_back(name);

            for (auto const& m : order)
            {
                if (!mgr->hasMod(m)) continue;
                gLastUnloadBase[m] = mgr->moduleBase(m);
                if (auto e = mgr->controlUnload(m); !e)
                {
                    output.error("卸载 '" + m + "' 失败: " + e.error().message());
                    output.error("已经卸载的部分不会自动恢复，请检查服务器状态。");
                    return false;
                }
                output.success("已卸载 '" + m + "'");
                if (unloadedOut) unloadedOut->push_back(m);
            }
            return true;
        }

        void cmdUnload(CommandOutput& output, std::string const& name, bool cascade)
        {
            if (doUnload(output, name, cascade, nullptr))
            {
                output.success(
                    "提示：要换成新编译的 dll，现在把文件替换掉，然后 /pier list 刷新列表，"
                    "再 /pier load " + name
                );
            }
        }

        void cmdReload(CommandOutput& output, std::string const& name, bool cascade)
        {
            auto* mgr = ModHost::instance();
            if (!mgr || !mgr->hasMod(name))
            {
                output.error("'" + name + "' 没有被加载（pier 模组）");
                return;
            }

            auto cand = readCandidate(name);
            if (!cand)
            {
                output.error(cand.error().message());
                return;
            }
            // 闸门：reload 只给声明了自己扛得住它的模组。其余走冷路径。
            if (!cand->reloadSafe)
            {
                output.error(
                    "'" + name + "' 没有在 manifest.json 里声明 \"reload_safe\": true，不允许 reload。"
                );
                output.error(
                    "reload 会重跑 " PIER_MAIN_SYMBOL "，模组必须自己保证：on_unload 里 join 掉所有"
                    "线程、关掉所有 handle、取消所有定时器（schedule_cancel），全局状态可重入。"
                    "确认做到了再加这个字段。在那之前请用 /pier unload " + name
                    + " 然后 /pier load " + name + "。"
                );
                return;
            }

            // 级联会把依赖方一起 reload，它们经历的正是上面那道闸门要守的
            // 同一种 reload —— 所以也得逐个查，别让没标记的模组从后门溜进来。
            if (cascade)
            {
                for (auto const& dep : dependentsOf(name))
                {
                    if (!isHostedMod(dep)) continue; // 稍后由 doUnload 报告
                    auto dc = readCandidate(dep);
                    if (!dc || !dc->reloadSafe)
                    {
                        output.error(
                            "级联里的 '" + dep + "' 没有声明 \"reload_safe\": true，"
                            "不能跟着一起 reload。请改用 /pier unload " + name
                            + " --cascade 再逐个 load。"
                        );
                        return;
                    }
                }
            }

            std::vector<std::string> unloaded;
            if (!doUnload(output, name, cascade, &unloaded)) return;

            // 按放倒顺序的逆序拉回来。
            std::reverse(unloaded.begin(), unloaded.end());
            bool allOk = true;
            for (auto const& m : unloaded)
            {
                auto manifest = readManifest(m);
                if (!manifest)
                {
                    output.error("重新加载 '" + m + "' 失败: " + manifest.error().message());
                    allOk = false;
                    break;
                }
                if (auto e = mgr->controlLoad(std::move(*manifest)); !e)
                {
                    output.error("重新加载 '" + m + "' 失败: " + e.error().message());
                    output.error("'" + m + "' 现在处于未加载状态，修好后用 /pier load " + m + " 拉起来。");
                    allOk = false;
                    break;
                }
                // reload 本来就是状态重置，这条路上基址不变是预期结果、
                // 不是症状 —— 在这里跑 checkImageSwapped 会每次都告警。
                // 那个检查只属于 unload → 重编 → load 的路径：在那条路上，
                // 基址不变才真的意味着新 dll 没被读进来。把存的地址丢掉，
                // 免得之后对同一个模组的 /pier load 拿它来比。
                gLastUnloadBase.erase(m);
                output.success("已重新加载 '" + m + "'");
            }
            if (allOk)
            {
                output.success(
                    "'" + name + "' reload 完成（状态已重置、配置已重读；dll 代码没有更换）"
                );
            }
        }

        /** /pier events —— 一站列出所有可订阅的事件 id：LeviLamina 动态
         *  注册表里的，加上每个事件提供方（hooks、命令事件）合成的。旧版把
         *  这两半拆在两条命令里，合成事件甚至列不出来 —— 排查「这个事件叫
         *  什么」的人要的是一张完整的单子。 */
        void cmdEvents(CommandOutput& output)
        {
            size_t n = 0;
            for (auto&& [modName, id] : ll::event::EventBus::getInstance().events())
            {
                output.success(std::string{id.name} + "  (来自 " + std::string{modName} + ")");
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
                                toString(s) + "  (合成，提供方 " + std::string(k->provider) + ")"
                            );
                            (*k->n)++;
                        }
                    );
                    return false; // 走完全部提供方
                },
                &ctx
            );
            output.success("共 " + std::to_string(n) + " 个事件");
        }

        void cmdAbi(CommandOutput& output)
        {
            auto const* api = bridgeApi();
            output.success(
                "Pier ABI v" + std::to_string(PIER_ABI_VERSION) + "（下限 v"
                + std::to_string(PIER_ABI_MIN_SUPPORTED) + "），表 "
                + std::to_string(api->struct_size) + " 字节，目标："
                + ((api->host_flags & PIER_FLAG_CLIENT) ? "客户端" : "服务端")
            );
        }
    } // namespace

    void registerCommand()
    {
        using namespace ll::command;
        auto& handle = CommandRegistrar::getServerInstance().getOrCreateCommand(
            "pier",
            "pier 模组控制：list / load / unload / reload / events / abi",
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
                    output.error("不认识的参数 " + args.unknownFlagText + "（只支持 --cascade）");
                    return;
                }

                auto needName = [&output, &args](char const* verb)
                {
                    if (args.name.empty())
                    {
                        output.error(std::string{"用法: /pier "} + verb + " <mod_name>");
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
                            "用法: /pier list | load <n> | unload <n> [--cascade] | "
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
                        "未知子命令 '" + args.sub
                        + "'。用法: /pier list | load <n> | unload <n> [--cascade] | "
                          "reload <n> [--cascade] | events | abi"
                    );
                }
            }
        );
        hostLogger().debug("/pier 已注册");
    }
#else
    void registerCommand() {}
#endif // !PIER_BUILD_CLIENT
} // namespace pier::mod_control

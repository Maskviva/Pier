/** Config.cpp: reading config.json, and the one place the defaults are written.
 *
 * The seed text and the field defaults in config.h are two spellings of the same values,
 * and read() takes its fallbacks from the defaults, so there are no more than two. A
 * drift between them means the file an operator is handed disagrees with the behavior
 * they get when they delete a line from it.
 *
 * A key this host does not read is dropped without a word. Nothing here carries a former
 * spelling of a setting or a note about one that is gone.
 *
 * A value of the wrong type or outside its range is reported, because there the operator
 * wrote something and got something else.
 */
#include "pier/support/config.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

#include "ll/api/Config.h" // Pulls in nlohmann/json.hpp

namespace pier
{
    namespace
    {
        /** What lands in a directory that has no config.json.
         *
         *  No comments in it. The meaning of each setting is in the documentation, and
         *  a file carrying both explanations is a file where the two drift apart while
         *  the operator believes the one in front of them. */
        constexpr char const* kSeed = R"({
  "language": "auto",
  "mods": {
    "disabled": []
  },
  "hooks": {
    "disabled": [],
    "decision_ttl_ms": 250
  },
  "watchdog": {
    "enabled": true,
    "warn_ms": 2000,
    "hang_ms": 30000,
    "shutdown_ms": 10000
  }
}
)";

        Config& installed()
        {
            static Config c;
            return c;
        }

        void fail(std::vector<ConfigProblem>& out, std::string_view key, std::vector<std::string> args)
        {
            out.push_back(ConfigProblem{ConfigProblem::Level::Error, key, std::move(args)});
        }

        void note(std::vector<ConfigProblem>& out, std::string_view key, std::vector<std::string> args)
        {
            out.push_back(ConfigProblem{ConfigProblem::Level::Note, key, std::move(args)});
        }

        nlohmann::json const* at(nlohmann::json const& root, std::string const& path)
        {
            nlohmann::json const* node = &root;
            std::size_t from = 0;
            while (from <= path.size())
            {
                auto dot = path.find('.', from);
                auto part = path.substr(from, dot == std::string::npos ? std::string::npos : dot - from);
                if (!node->is_object() || !node->contains(part)) return nullptr;
                node = &(*node)[part];
                if (dot == std::string::npos) break;
                from = dot + 1;
            }
            return node;
        }

        std::string text(nlohmann::json const& root, std::string const& path, std::string fallback,
                         std::vector<ConfigProblem>& out)
        {
            auto const* node = at(root, path);
            if (!node) return fallback;
            if (!node->is_string())
            {
                fail(out, "conf.bad_type", {path, "text"});
                return fallback;
            }
            return node->get<std::string>();
        }

        /** A list of strings. A non-string entry is dropped and named individually
         *  rather than voiding the list: an operator with ten mods in mods.disabled and
         *  one typo among them should keep the other nine disabled. */
        std::vector<std::string> list(nlohmann::json const& root, std::string const& path,
                                      std::vector<ConfigProblem>& out)
        {
            std::vector<std::string> v;
            auto const* node = at(root, path);
            if (!node) return v;
            if (!node->is_array())
            {
                fail(out, "conf.bad_type", {path, "list"});
                return v;
            }
            for (auto const& entry : *node)
            {
                if (!entry.is_string())
                {
                    fail(out, "conf.bad_entry", {path});
                    continue;
                }
                auto s = entry.get<std::string>();
                if (!s.empty()) v.push_back(std::move(s));
            }
            return v;
        }

        bool flag(nlohmann::json const& root, std::string const& path, bool fallback,
                  std::vector<ConfigProblem>& out)
        {
            auto const* node = at(root, path);
            if (!node) return fallback;
            if (!node->is_boolean())
            {
                fail(out, "conf.bad_type", {path, "true/false"});
                return fallback;
            }
            return node->get<bool>();
        }

        std::int64_t number(nlohmann::json const& root, std::string const& path, std::int64_t fallback,
                            std::int64_t low, std::int64_t high, std::vector<ConfigProblem>& out)
        {
            auto const* node = at(root, path);
            if (!node) return fallback;
            if (!node->is_number_integer())
            {
                fail(out, "conf.bad_type", {path, "number"});
                return fallback;
            }
            auto raw = node->get<std::int64_t>();
            auto clamped = std::clamp(raw, low, high);
            if (clamped != raw)
            {
                fail(out, "conf.clamped",
                     {path, std::to_string(raw), std::to_string(low), std::to_string(high),
                      std::to_string(clamped)});
            }
            return clamped;
        }

        Config read(nlohmann::json const& root, std::vector<ConfigProblem>& out)
        {
            // Every fallback is the field's own default, so config.h is the one place a
            // default is spelled; kSeed above is the other, which an operator reads.
            Config const d{};
            Config c;
            c.language = text(root, "language", d.language, out);
            c.disabledMods = list(root, "mods.disabled", out);
            c.disabledEvents = list(root, "hooks.disabled", out);
            c.decisionTtlMs = number(root, "hooks.decision_ttl_ms", d.decisionTtlMs, 0, 5000, out);
            // A limit of an hour is already no limit in practice; the ceiling only keeps a
            // typo with extra digits from overflowing the millisecond count.
            c.watchdogEnabled = flag(root, "watchdog.enabled", d.watchdogEnabled, out);
            c.watchdogWarnMs =
                static_cast<std::uint32_t>(number(root, "watchdog.warn_ms", d.watchdogWarnMs, 0, 3600000, out));
            c.watchdogHangMs =
                static_cast<std::uint32_t>(number(root, "watchdog.hang_ms", d.watchdogHangMs, 0, 3600000, out));
            c.watchdogShutdownMs = static_cast<std::uint32_t>(
                number(root, "watchdog.shutdown_ms", d.watchdogShutdownMs, 0, 3600000, out));
            return c;
        }
    } // namespace

    std::vector<ConfigProblem> loadConfig(std::string const& modDir)
    {
        std::vector<ConfigProblem> problems;
        std::filesystem::path const path = std::filesystem::path{modDir} / "config.json";
        auto const label = path.string();

        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
        {
            std::filesystem::create_directories(path.parent_path(), ec);
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (file)
            {
                file << kSeed;
                note(problems, "conf.seeded", {label});
            }
            else
            {
                // Not fatal. The defaults below are the same values the file would have
                // held, so this run behaves identically; the next start tries again.
                fail(problems, "conf.seed_failed", {label});
            }
            installed() = Config{};
            return problems;
        }

        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            fail(problems, "conf.unreadable", {label});
            installed() = Config{};
            return problems;
        }
        std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Comments are accepted although none are written. An operator annotating their
        // own file is not an error, and a parser that stopped accepting them would turn
        // an annotated file into a refused one on the next start.
        auto parsed = nlohmann::json::parse(body, nullptr, false, true);
        if (parsed.is_discarded() || !parsed.is_object())
        {
            fail(problems, "conf.parse_failed", {label});
            installed() = Config{};
            return problems;
        }

        installed() = read(parsed, problems);
        return problems;
    }

    Config const& config() { return installed(); }

    bool modDisabled(std::string_view name)
    {
        auto const& v = installed().disabledMods;
        return std::find(v.begin(), v.end(), name) != v.end();
    }

    bool eventDisabled(std::string_view name)
    {
        auto const& v = installed().disabledEvents;
        return std::find(v.begin(), v.end(), name) != v.end();
    }
} // namespace pier

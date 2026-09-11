/**
 * I18n.cpp: the key table and the two files behind it.
 *
 * The built-in English is a static table compiled in, so a host with no lang directory
 * still says everything it has to say. A file on disk adds to it and overrides it per
 * key -- an operator translating half the lines gets half translated, not a file that
 * has to be complete before it is useful.
 */
#include "pier/support/i18n.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>

#include "ll/api/i18n/I18n.h"

namespace pier
{
    namespace
    {
        struct Table
        {
            std::mutex lock;
            /// locale -> key -> line.
            std::map<std::string, std::map<std::string, std::string, std::less<>>, std::less<>> byLocale;
            std::string locale = "en_US";
        };

        Table& table()
        {
            static Table t;
            return t;
        }

        /** The lines this host ships with. English, because it is the one language the
         *  format strings were written in and the one every operator can fall back to. */
        std::map<std::string, std::string, std::less<>> const& builtinEnglish()
        {
            static std::map<std::string, std::string, std::less<>> const m = {
                {"dim.height.generator_mismatch",
                 "'{}': the spec asks for height {}..{} with the engine's {} generator, which is written for "
                 "{}..{}; the generator's own range is used, and blocks saved under the old range are not reachable"},
                {"dim.structures.symbol_missing",
                 "{} not found; custom {} dimensions generate no structures"},
                {"dim.structures.none",
                 "'{}': the engine functions that fill a generator's structure feature registry are not in this "
                 "build, so this dimension generates its terrain without structures. The alternative is a "
                 "generator holding every structure set in the game with nothing behind any of them to place"},
                {"dim.resolve.host_registry",
                 "resolving '{}' (id {}) out of the host registry; the engine name table has no entry for it, "
                 "which is why a teleport used to land in place"},
                {"dim.factory.building", "the factory for '{}' is building the dimension for id {}"},
                {"dim.supplied.by", "'{}': terrain supplied by '{}'"},
                {"dim.supplied.missing",
                 "'{}' has no terrain this session: its spec says the terrain comes from a mod and no mod "
                 "registered it. The chunks already generated stay where they are; the world is void until "
                 "that mod is loaded again"},
                {"lang.loaded", "language {}: {} key(s) from {}"},
            };
            return m;
        }

        std::string trim(std::string s)
        {
            while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
            std::size_t at = 0;
            while (at < s.size() && (s[at] == ' ' || s[at] == '\t')) ++at;
            return s.substr(at);
        }

        /** `key=value` per line, `#` starts a comment. The same shape as Minecraft's own
         *  .lang files, so an operator who has edited one of those knows this one. */
        std::size_t readFile(std::filesystem::path const& path, std::map<std::string, std::string, std::less<>>& into)
        {
            std::ifstream in(path);
            if (!in) return 0;
            std::size_t n = 0;
            std::string line;
            while (std::getline(in, line))
            {
                line = trim(std::move(line));
                if (line.empty() || line[0] == '#') continue;
                auto eq = line.find('=');
                if (eq == std::string::npos) continue;
                auto key = trim(line.substr(0, eq));
                auto value = trim(line.substr(eq + 1));
                if (key.empty()) continue;
                into[key] = value;
                ++n;
            }
            return n;
        }
    } // namespace

    std::size_t loadLanguages(std::string const& langDir)
    {
        auto& t = table();
        std::lock_guard<std::mutex> g(t.lock);
        t.locale = std::string{ll::i18n::getDefaultLocaleCode()};
        t.byLocale["en_US"] = builtinEnglish();

        std::size_t fromDisk = 0;
        std::error_code ec;
        std::filesystem::path dir{langDir};
        if (!std::filesystem::is_directory(dir, ec)) return 0;
        for (auto const& entry : std::filesystem::directory_iterator(dir, ec))
        {
            if (entry.path().extension() != ".lang") continue;
            auto code = entry.path().stem().string();
            fromDisk += readFile(entry.path(), t.byLocale[code]);
        }
        return fromDisk;
    }

    std::string_view localeCode()
    {
        auto& t = table();
        std::lock_guard<std::mutex> g(t.lock);
        static std::string cached;
        cached = t.locale;
        return cached;
    }

    std::string_view tr(std::string_view key)
    {
        auto& t = table();
        std::lock_guard<std::mutex> g(t.lock);
        // The chain, in order. Each step is a whole locale, not a merge: a half-finished
        // translation falls back per key, which is what makes it usable while it is being
        // written.
        for (auto const& locale : {std::string_view{t.locale}, std::string_view{"en_US"}})
        {
            auto l = t.byLocale.find(locale);
            if (l == t.byLocale.end()) continue;
            auto it = l->second.find(key);
            if (it != l->second.end()) return it->second;
        }
        // The key itself, never an empty string: an empty warning reads as nothing having
        // happened.
        return key;
    }
} // namespace pier

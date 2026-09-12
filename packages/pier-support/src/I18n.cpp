/**
 * I18n.cpp: the key table and the two files behind it.
 *
 * Three layers, each overriding the one under it: built-in English, the shipped
 * translations compiled in from lang/*.lang by tools/embed-lang.py, then a .lang file
 * on disk.
 *
 * The middle layer is compiled rather than read because it used to be the top one and
 * never arrived: packaging carried the target and the manifest, lang/ stayed in the
 * repository, and every server ran on English with nothing reporting why. A build step
 * copying the directory fixes that instance and keeps its shape -- translations only as
 * reliable as a packaging rule nobody tests.
 *
 * Disk stays, per key rather than per file: an operator translating half the lines gets
 * half overridden, not a file that must be complete before it is useful.
 */
#include "pier/support/i18n.h"

#include <cctype>
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

        /** A locale code as a map key: lowercased, and `-` folded to `_`.
         *
         *  The engine reports `zh-CN` and a .lang file is called `zh_CN.lang`, after
         *  Minecraft's own. Keying the table on either spelling makes the other miss,
         *  and the miss is silent: the lookup falls through to English and the startup
         *  line reports zero keys for a language whose file is sitting right there. */
        std::string localeKey(std::string_view code)
        {
            std::string out;
            out.reserve(code.size());
            for (char c : code)
            {
                out.push_back(c == '-' ? '_' : static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }
            return out;
        }

        Table& table()
        {
            static Table t;
            return t;
        }

        /** The translations this host ships with, generated from lang/*.lang.
         *
         *  Returned by value: it is built once into the table at load and never read
         *  again, and a static would keep a second copy alive for the process. */
        std::map<std::string, std::map<std::string, std::string, std::less<>>, std::less<>>
        shippedTranslations()
        {
            return
#include "LangShipped.inc"
                ;
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
                {"lang.source.builtin", "the lines shipped with this host"},
                // Every boot prints these once per custom dimension, and an operator
                // reads them to answer "did my world come up the way I wrote it".
                // That is the bar in i18n.h, so they carry keys.
                {"dim.height.set", "'{}': definition set to height {}..{}"},
                {"dim.registered", "'{}' built through the factory and registered as id {}, registry id {}"},
                {"dim.ready", "'{}' ready with id {}"},
                {"dim.terrain.layers", "'{}': layered terrain, {} layer(s){}"},
                {"dim.terrain.layers.grid", ", on a grid"},
                {"dim.terrain.end", "'{}': end generator"},
                {"host.ready", "ready, ABI v{}, api table {} bytes, built {} {}"},
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
        // Kept as the engine spells it, for the startup line; the map is keyed on
        // localeKey() so the two spellings cannot miss each other.
        t.locale = std::string{ll::i18n::getDefaultLocaleCode()};
        t.byLocale[localeKey("en_US")] = builtinEnglish();
        // Compiled-in translations go on before the disk pass, so a file on disk
        // overrides them key by key rather than having to restate the whole language.
        for (auto& [code, lines] : shippedTranslations())
        {
            auto& into = t.byLocale[localeKey(code)];
            for (auto& [key, value] : lines) into[key] = value;
        }

        std::size_t fromDisk = 0;
        // Counted after the disk pass, below: what an operator wants from the startup
        // line is how many lines his server actually has, not how many a file added.
        std::error_code ec;
        std::filesystem::path dir{langDir};
        // No directory is the normal case now: the shipped translations are already in
        // the table above, and 0 only means "nothing came off disk".
        if (!std::filesystem::is_directory(dir, ec)) return 0;
        for (auto const& entry : std::filesystem::directory_iterator(dir, ec))
        {
            if (entry.path().extension() != ".lang") continue;
            auto code = entry.path().stem().string();
            fromDisk += readFile(entry.path(), t.byLocale[localeKey(code)]);
        }
        return fromDisk;
    }

    std::size_t activeKeyCount()
    {
        auto& t = table();
        std::lock_guard<std::mutex> g(t.lock);
        auto it = t.byLocale.find(localeKey(t.locale));
        return it == t.byLocale.end() ? 0 : it->second.size();
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
        for (auto const& locale : {localeKey(t.locale), localeKey("en_US")})
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

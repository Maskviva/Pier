/**
 * I18n.cpp: the key table and the two layers behind it.
 *
 * Built-in English answers every key, and lang/<code>.lang under the mod directory
 * overrides it. Nothing is generated, so a translation is a file an operator drops in
 * and edits, and adding a language needs no rebuild.
 *
 * The override is per key rather than per file: an operator translating half the lines
 * gets half overridden, not a file that must be complete before it is useful.
 *
 * Which locale is active comes from config.json and not from this layer. "auto" is
 * resolved here against the engine, because this is the only place that knows what the
 * engine answers and the config layer must not depend on it.
 */
#include "pier/support/i18n.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>

#include "ll/api/i18n/I18n.h"

#include "pier/support/lang_files.h"

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
            /// Whether `locale` was named in config.json rather than reported by the engine.
            bool fromConfig = false;
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

        std::size_t parseInto(std::string const& text,
                              std::map<std::string, std::string, std::less<>>& into);

        /** The English this host answers with when nothing on disk has the key.
         *
         *  Parsed from `lang/en_US.lang` itself, embedded at build time. Not a second
         *  table: the file the build ships and the lines compiled in are the same text,
         *  and `lang-embedded` keeps them that way. */
        std::map<std::string, std::string, std::less<>> const& builtinEnglish()
        {
            static std::map<std::string, std::string, std::less<>> const m = [] {
                std::map<std::string, std::string, std::less<>> out;
                parseInto(builtinEnglishText(), out);
                return out;
            }();
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
        std::size_t parseInto(std::string const& text, std::map<std::string, std::string, std::less<>>& into)
        {
            std::size_t n = 0;
            std::istringstream in(text);
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

        std::size_t readFile(std::filesystem::path const& path, std::map<std::string, std::string, std::less<>>& into)
        {
            std::ifstream in(path);
            if (!in) return 0;
            std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            return parseInto(text, into);
        }
    } // namespace

    std::size_t loadLanguages(std::string const& langDir, std::string const& language)
    {
        auto& t = table();
        std::lock_guard<std::mutex> g(t.lock);
        // The engine is asked only for "auto". An explicit code is kept exactly as the
        // operator spelled it, so the startup line echoes their own value back and a
        // code with no file on disk is visible instead of being rewritten into one that
        // has.
        t.fromConfig = !language.empty() && language != "auto";
        t.locale = t.fromConfig ? language : std::string{ll::i18n::getDefaultLocaleCode()};
        t.byLocale.clear();
        t.byLocale[localeKey("en_US")] = builtinEnglish();

        std::error_code ec;
        std::filesystem::path dir{langDir};
        // An absent directory is not an error: the built-in English above already
        // answers every key, and 0 says only that nothing came off disk.
        if (!std::filesystem::is_directory(dir, ec)) return 0;
        // Only the active locale's own file is counted, although every file is read.
        // Summing all of them would grow the startup number with each language
        // installed, while the question that line answers is whether the language this
        // server actually speaks has lines behind it.
        std::size_t active = 0;
        auto const wanted = localeKey(t.locale);
        for (auto const& entry : std::filesystem::directory_iterator(dir, ec))
        {
            if (entry.path().extension() != ".lang") continue;
            auto code = localeKey(entry.path().stem().string());
            auto n = readFile(entry.path(), t.byLocale[code]);
            if (code == wanted) active += n;
        }
        return active;
    }

    std::size_t activeKeyCount()
    {
        auto& t = table();
        std::lock_guard<std::mutex> g(t.lock);
        // The chain and not one map. A locale with no file of its own still answers every
        // key through the English underneath it, and reporting 0 there reads as a host
        // with no lines at all while every line is in fact about to print.
        std::set<std::string_view> keys;
        for (auto const& locale : {localeKey(t.locale), localeKey("en_US")})
        {
            auto l = t.byLocale.find(locale);
            if (l == t.byLocale.end()) continue;
            for (auto const& [key, _] : l->second) keys.insert(key);
        }
        return keys.size();
    }

    std::string localeCode()
    {
        auto& t = table();
        std::lock_guard<std::mutex> g(t.lock);
        // By value. A view into a shared buffer refilled under this lock is a dangling
        // read the moment a second thread asks, and the callers all format it anyway.
        return t.locale;
    }

    bool localeFromConfig()
    {
        auto& t = table();
        std::lock_guard<std::mutex> g(t.lock);
        return t.fromConfig;
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

    std::string trv(std::string_view key, std::vector<std::string> const& args)
    {
        auto pattern = tr(key);
        // Substituted here rather than through fmt, because every argument is already
        // text and a runtime arg store would add a header this package does not
        // otherwise need. The placeholder count is checked for the same reason trf
        // catches: a .lang line with the wrong number of `{}` is a data bug, and
        // printing the key beats printing a line with an argument missing from it.
        std::string out;
        out.reserve(pattern.size());
        std::size_t used = 0;
        for (std::size_t i = 0; i < pattern.size(); ++i)
        {
            if (pattern[i] == '{' && i + 1 < pattern.size() && pattern[i + 1] == '}')
            {
                if (used >= args.size()) return std::string{key} + " [bad translation]";
                out += args[used++];
                ++i;
                continue;
            }
            out.push_back(pattern[i]);
        }
        if (used != args.size()) return std::string{key} + " [bad translation]";
        return out;
    }
} // namespace pier

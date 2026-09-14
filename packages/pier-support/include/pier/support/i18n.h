/** i18n.h: the host's own log lines, in the language config.json names.
 *
 * The lines live in lang/<code>.lang next to the dll and the keys live in the code, so
 * adding a language is a file an operator drops in and needs no rebuild.
 *
 * "auto" follows the locale the engine reports, which is right for a server with one
 * language; an operator who reads a different language than their players names the
 * code instead, and before that setting existed they had no way to ask for it.
 *
 * A key is added when an operator sees the line on a normal boot or after a normal
 * action, and can act on it. Deep failure diagnostics stay in English on purpose: a
 * mistranslated one destroys the search trail.
 *
 * The chain is the selected language, then en_US, then the key itself. A missing key
 * prints the key, because an empty warning reads as nothing having happened.
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "fmt/format.h"

namespace pier
{
    /** Loads lang/<code>.lang from the mod directory and selects `language`.
     *
     *  `language` is config.json's value: "auto" resolves to the engine's locale, and
     *  any other value is used as written, spelled either zh_CN or zh-CN. An absent
     *  directory is not an error, since the built-in English is compiled in and always
     *  answers. Returns how many keys came off disk, for the one startup line that
     *  reports it. */
    std::size_t loadLanguages(std::string const& langDir, std::string const& language);

    /** How many lines the active locale has, counting the built-in English and anything
     *  a file on disk added. This is what the startup line reports: an operator asking
     *  whether the server is translated is asking about this number, not about how many
     *  a file contributed. */
    std::size_t activeKeyCount();

    /** The active locale code, as it was selected. */
    std::string localeCode();

    /** Whether the active locale came from config.json rather than from the engine.
     *  The startup line says which, so an operator who edited the setting and sees no
     *  change can tell a value that did not apply from a file that is not translated. */
    bool localeFromConfig();

    /** The line for `key`, or the key itself when nothing has it. */
    std::string_view tr(std::string_view key);

    /** `tr` plus formatting. The arguments must match the placeholders of the English
     *  line; a translation that changes their number or order is a bug in the .lang
     *  file, and this catches it rather than letting fmt throw into a log call. */
    template <typename... Args>
    std::string trf(std::string_view key, Args&&... args)
    {
        auto pattern = tr(key);
        try
        {
            return fmt::vformat(pattern, fmt::make_format_args(args...));
        }
        catch (std::exception const&)
        {
            // The translation's placeholders do not match the call. Falling back to the
            // key plus the raw arguments keeps the information; throwing here would take
            // down whatever was being logged about.
            return std::string{key} + " [bad translation]";
        }
    }

    /** `trf` with the arguments in a vector, all of them already text.
     *
     *  For the config layer, which collects its problems before the catalog exists and
     *  cannot name their types at the call site. */
    std::string trv(std::string_view key, std::vector<std::string> const& args);
} // namespace pier

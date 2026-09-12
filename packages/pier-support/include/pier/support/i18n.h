/** i18n.h: the host's own log lines, in the server's language.
 *
 * The engine already knows what language this server runs in
 * (`ll::i18n::getDefaultLocaleCode`), so this layer does not ask again and adds no
 * setting for it: one less thing that can disagree with the rest of the server.
 *
 * A key is added when an operator sees the line on a normal boot or after a normal
 * action, and can act on it. Deep failure diagnostics stay in English on purpose: a
 * mistranslated one destroys the search trail, and the person reading it is already
 * comparing it against the source or against a report someone else filed.
 *
 * The chain is server language, then en_US, then the key itself. That last step matters:
 * a missing key prints the key, and an empty warning reads as nothing having happened.
 */
#pragma once

#include <string>
#include <string_view>

#include "fmt/format.h"

namespace pier
{
    /** Loads `lang/<code>.lang` beside the host. Missing files are not an error: the
     *  built-in English is compiled in and always answers. Returns how many keys came
     *  off disk, for the one startup line that reports it. */
    std::size_t loadLanguages(std::string const& langDir);

    /** How many lines the active locale has, counting the compiled-in translations and
     *  anything a file on disk added. This is what the startup line reports: an operator
     *  asking "is my server translated" is asking about this number, not about how many
     *  a file contributed. */
    std::size_t activeKeyCount();

    /** The current locale code, as the engine reports it. */
    std::string_view localeCode();

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
} // namespace pier

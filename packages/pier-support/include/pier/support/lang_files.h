/** lang_files.h: the English lines this host answers with, as the file that holds them.
 *
 * `lang/en_US.lang` is the fallback of the whole chain, so its text has to be inside the
 * binary: a mod directory with no lang directory still has to print something. Every
 * other language is a file the release ships and this host only reads.
 *
 * Held as the file rather than as a key table, so there is one spelling of the lines
 * instead of two that a reader has to keep level by hand.
 */
#pragma once

#include <string>

namespace pier
{
    /** `lang/en_US.lang`, byte for byte, newline-terminated. */
    std::string builtinEnglishText();
} // namespace pier

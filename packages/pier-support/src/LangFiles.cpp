/** LangFiles.cpp: `lang/en_US.lang`, as data.
 *
 * The fallback of the language chain has to answer without a file on disk, so the English
 * is compiled in. It is the file itself and not a key table written a second time, which
 * is the arrangement that cannot drift out of step with what the release ships.
 *
 * One string per source line. A line stays readable in a diff, and no literal approaches
 * the length a compiler will refuse.
 *
 * `tools/checks/lang_embedded.py` compares this against `lang/en_US.lang` byte for byte.
 * Editing one and not the other is the whole failure mode of holding the text twice, and
 * it is the only thing keeping that from landing.
 */
#include "pier/support/lang_files.h"

#include <string_view>

namespace pier
{
    namespace
    {
        /// `lang/en_US.lang`, one entry per line, 42 of them.
        constexpr std::string_view kEnUS[] = {
        "# Pier - English (en_US)",
        "#",
        "# One entry per line, `key = value`. `#` starts a comment and a blank line is ignored.",
        "# `{}` is a placeholder the host fills in, positionally: a line whose placeholder count",
        "# differs from this file prints `[bad translation]` instead, and their order is not",
        "# checkable, so a translation that reorders them lands the arguments in the wrong slots.",
        "#",
        "# The host reads `plugins/Pier/lang/<code>.lang`. Which code is read comes from",
        "# `config.json`: `\"language\": \"auto\"` follows the locale the engine reports, and any other",
        "# value names the file directly, spelled either zh_CN or zh-CN.",
        "#",
        "# Nothing here is generated: copy this file to a new code, translate the values, and the",
        "# untranslated keys fall back to these lines. Adding a language needs no rebuild.",
        "",
        "dim.height.generator_mismatch='{}': the spec asks for height {}..{} with the engine's {} generator, which is written for {}..{}; the generator's own range is used, and blocks saved under the old range are not reachable",
        "dim.structures.symbol_missing={} not found; custom {} dimensions generate no structures",
        "dim.structures.none='{}': the engine functions that fill a generator's structure feature registry are not in this build, so this dimension generates its terrain without structures. The alternative is a generator holding every structure set in the game with nothing behind any of them to place",
        "dim.resolve.host_registry=resolving '{}' (id {}) out of the host registry; the engine name table has no entry for it, which is why a teleport used to land in place",
        "dim.factory.building=the factory for '{}' is building the dimension for id {}",
        "dim.supplied.by='{}': terrain supplied by '{}'",
        "dim.supplied.missing='{}' has no terrain this session: its spec says the terrain comes from a mod and no mod registered it. The chunks already generated stay where they are; the world is void until that mod is loaded again",
        "lang.loaded=language {} ({}): {} key(s) from {}",
        "lang.source.builtin=the lines shipped with this host",
        "lang.pick.config=named in config.json",
        "lang.pick.engine=auto, following the engine",
        "lang.no_file=there is no lang/{}.lang, so every line falls back to English; copy en_US.lang to that name to start a translation",
        "dim.height.set='{}': definition set to height {}..{}",
        "dim.registered='{}' built through the factory and registered as id {}, registry id {}",
        "dim.ready='{}' ready with id {}",
        "dim.terrain.layers='{}': layered terrain, {} layer(s){}",
        "dim.terrain.layers.grid=, on a grid",
        "dim.terrain.end='{}': end generator",
        "host.ready=ready, ABI v{}, api table {} bytes, built {} {}",
        "conf.seeded={} did not exist; a default one has been written",
        "conf.seed_failed={} could not be written; the defaults apply this run and the next start tries again",
        "conf.unreadable={} cannot be opened. Every setting is at its default this run, so a mod named in mods.disabled loads as usual",
        "conf.parse_failed={} is not valid JSON. Every setting is at its default this run, so a mod named in mods.disabled loads as usual",
        "conf.bad_type='{}' wants {}, and the value written there is ignored",
        "conf.bad_entry='{}' holds an entry that is not text, and that entry alone is ignored",
        "conf.clamped='{}' was {}, which is outside {}..{}; {} is used",
        "mod.disabled='{}' is named in mods.disabled in config.json, so this host will not load it",
        "hooks.disabled=synthetic event '{}' is named in hooks.disabled, so the subscription from '{}' is refused rather than answered with a handle that never fires",
        };
    } // namespace

    std::string builtinEnglishText()
    {
        std::string out;
        for (auto line : kEnUS)
        {
            out.append(line);
            out.push_back('\n');
        }
        return out;
    }
} // namespace pier

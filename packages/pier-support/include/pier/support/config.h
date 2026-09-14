/** config.h: the host's settings, read from the mod directory's config.json.
 *
 * Written once when absent and never rewritten. Topping up a missing key would
 * overwrite whatever an operator wrote around it, and an absent key already has a
 * defined answer, which is the default on the field below.
 *
 * A key this host does not read is dropped without a word. There is no compatibility
 * layer here: nothing remembers a former spelling of a setting or one that is gone.
 *
 * A file that does not parse leaves every setting at its default, reported at error
 * level because that is the permissive direction: mods.disabled goes empty while the
 * mods it names load as usual.
 *
 * Read once inside load(). Nothing rereads it, so the getters take no lock.
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pier
{
    /** One thing the config layer has to say, as a key and its positional arguments.
     *
     *  Not a finished sentence: the config is read before the language is known,
     *  because the language is one of its settings. The caller renders these through
     *  trv() once the catalog is up, so the first thing an operator reads about their
     *  config file is already in their own language. */
    struct ConfigProblem
    {
        /** Error means a setting the operator wrote is not in effect. Note means the
         *  layer did something on its own, such as writing the file, and nothing they
         *  asked for was lost. The two print at different levels, so a first start does
         *  not open with a red line. */
        enum class Level
        {
            Note,
            Error,
        };

        Level level = Level::Error;
        std::string_view key;
        std::vector<std::string> args;
    };

    struct Config
    {
        /** Which lang/<code>.lang the host speaks. "auto" follows the locale the engine
         *  reports, which is what a server with one language wants; any other value is
         *  the code itself, spelled either zh_CN or zh-CN. */
        std::string language = "auto";

        /** Pier mods this host refuses to load, by manifest name. The refusal happens
         *  before the dylib is mapped, so a mod named here cannot run any code. */
        std::vector<std::string> disabledMods;

        /** Synthetic events no mod may subscribe to, by event name. A subscription is
         *  refused rather than silently dropped, so a mod that needs the event fails
         *  where the operator can see it instead of believing it is protected. */
        std::vector<std::string> disabledEvents;

        /** How long a pressure plate or push decision stays cached, in milliseconds.
         *  0 dispatches every tick per actor, which is what the cache exists to avoid;
         *  the ceiling is 5000 because a stale allow outlives the claim it was read
         *  from. */
        std::int64_t decisionTtlMs = 250;
    };

    /** Reads modDir/config.json, seeding it when absent, and installs the result.
     *
     *  Returns what was wrong with the file, in the order found. An empty vector means
     *  the file was read and every key in it was understood. Call once, from load(),
     *  before loadLanguages: the language it returns is the one the catalog uses. */
    std::vector<ConfigProblem> loadConfig(std::string const& modDir);

    /** The installed settings. Before loadConfig this is the defaults, which is what
     *  the few call sites that can run that early should see. */
    Config const& config();

    /** Whether a mod name appears in mods.disabled. Case sensitive, matching the
     *  manifest name exactly, because a manifest name is an identifier and a fuzzy
     *  match here would silently refuse a mod the operator did not name. */
    bool modDisabled(std::string_view name);

    /** Whether an event name appears in hooks.disabled. */
    bool eventDisabled(std::string_view name);
} // namespace pier

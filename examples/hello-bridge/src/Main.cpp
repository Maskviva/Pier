/** Main.cpp: a native LeviLamina mod asking a Pier mod a question.
 *
 * `type` in manifest.json is `native`, not `pier`. LeviLamina loads this the way it loads
 * any C++ mod, and Pier neither knows about it nor loads it. The only thing shared is one
 * header and three exported symbols.
 *
 * The bridge reaches the JSON service registry and nothing else: no events, no hooks, no
 * lane. A mod that wants those is a Pier mod and includes `sdk/abi.h`.
 *
 * The work happens in enable() and not load(). `Client::open()` binds to a Pier that is
 * already in the process and never brings in a second copy, so Pier has to be up first;
 * the manifest declares it as a dependency, which is what orders the two.
 */
#include <string>

#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"

#include "pier-bridge.h"

namespace hello_bridge
{
    class Mod
    {
    public:
        static Mod& getInstance()
        {
            static Mod instance;
            return instance;
        }

        [[nodiscard]] ll::mod::NativeMod& getSelf() const { return *ll::mod::NativeMod::current(); }

        bool load() { return true; }

        bool enable()
        {
            auto& logger = getSelf().getLogger();

            mPier = pier::bridge::Client::open();
            if (!mPier)
            {
                // Absent Pier is a normal deployment and not a failure to enable. The
                // alternative, refusing to come up, makes an optional integration into a
                // hard dependency of a mod that has its own reasons to exist.
                logger.warn("Pier is not loaded, or speaks another bridge ABI; running without it");
                return true;
            }

            // Asked once here rather than discovered by a NotFound later. A log line at
            // startup saying which services exist is readable; one at the moment somebody
            // needed an answer is not.
            auto listed = mPier->services();
            if (listed.ok()) logger.info("services registered with Pier: {}", listed.body);

            // A name no mod registers, so a clean server takes the NotFound branch and
            // shows what an absent provider looks like. Replace it with one from the
            // documentation of whatever mod is being asked.
            constexpr char const* kService = "example:economy.balance";
            auto reply = mPier->call(kService, R"({"player":"2535470000000000"})");
            switch (reply.status)
            {
            case pier::bridge::Status::Ok:
                logger.info("{} answered {}", kService, reply.body);
                break;
            case pier::bridge::Status::NotFound:
                logger.info("no mod provides {}; nothing to do", kService);
                break;
            default:
                logger.warn("{} failed: {}", kService, reply.body);
                break;
            }
            return true;
        }

        /** Dropped here and not in the destructor. The client holds a reference to
         *  Pier.dll for its whole lifetime, and a static that outlives shutdown would hold
         *  that reference past the point where the loader is taking modules down. */
        bool disable()
        {
            mPier.reset();
            return true;
        }

    private:
        std::unique_ptr<pier::bridge::Client> mPier;
    };
} // namespace hello_bridge

LL_REGISTER_MOD(hello_bridge::Mod, hello_bridge::Mod::getInstance());

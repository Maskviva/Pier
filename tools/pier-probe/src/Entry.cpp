/**
 * Entry.cpp — pier_main and the three lifecycle callbacks.
 *
 * What this mod is for: pointing it at a host and reading what comes back. It reports,
 * per slot, whether the host has it and what it answered. Nothing here asserts, because
 * on 26.32 a documented set of slots answers "no" and an assert would stop the run at
 * the first of them instead of producing the list.
 *
 * Where the work happens matters. Registration order is not a good time to call
 * anything: the level is not up, so every world slot would report failure for a reason
 * that has nothing to do with the host's capabilities. The probe runs from on_enable
 * instead, which the host calls after the server is running.
 */
#include <exception>

#include "probe.h"

void probeCensus();
void probeReadOnly();

namespace
{
    /** True once the probe has run, so a second enable does not append a second report. */
    bool gReported = false;

    bool onEnable(void*)
    {
        if (gReported) return true;
        gReported = true;

        probe::log(3, "pier-probe: walking the host table");
        try
        {
            probeCensus();
            probeReadOnly();
        }
        catch (std::exception const& e)
        {
            // The probe itself failed, which is not the same as a slot failing and must
            // not read as one. Reporting it here keeps the partial results.
            probe::log(1, std::string{"pier-probe stopped early: "} + e.what());
        }
        catch (...)
        {
            probe::log(1, "pier-probe stopped early on a non-std exception");
        }

        auto const t = probe::report();

        // Enabling succeeds either way. A refusing slot is a reading, and refusing to
        // enable over one would make this mod useless on exactly the host it exists to
        // describe. A slot that threw is a defect and says so at error level, which is
        // what the count above already carries.
        (void)t;
        return true;
    }

    bool onDisable(void*) { return true; }
    bool onUnload(void*) { return true; }
} // namespace

// PIER_MAIN_EXPORT and not a bare extern "C": on Windows the name has to be exported as
// well as declared, and a DLL that only declares it loads far enough to be refused.
PIER_MAIN_EXPORT bool pier_main(PierApi const* api, PierModHandle self, PierModVTable* out)
{
    if (api == nullptr || out == nullptr) return false;

    // The header scalars are read before anything else: struct_size is what every later
    // slot check is measured against, and a table that does not even carry the four of
    // them is not one this mod can walk.
    if (api->struct_size < sizeof(uint32_t) * 4) return false;

    probe::bind(api, self);

    // The mod fills its own four header scalars; the host reads struct_size to know how
    // much of this table it may touch, which is the mirror of what this mod does to the
    // host table.
    out->struct_size = sizeof(PierModVTable);
    out->abi_version = PIER_ABI_VERSION;
    out->mod_flags = 0;
    out->_reserved0 = 0;

    out->instance = nullptr;
    out->on_enable = &onEnable;
    out->on_disable = &onDisable;
    out->on_unload = &onUnload;
    return true;
}

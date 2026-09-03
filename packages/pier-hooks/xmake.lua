-- pier-hooks: events synthesized with native detours.
--
-- Neither LeviLamina nor vanilla offers a subscribable counterpart for these. One event
-- per .cpp, each self-registering as an entry of one EventProvider, so adding an event
-- changes no table. Server only.
target("pier-hooks")
    pier_common()
    set_kind("object")
    -- object and not static. Every TU in this package registers itself into the host
    -- SPI through a file-level static object and no external symbol references them.
    -- Inside a static library the linker drops such an object entirely, and the
    -- symptom is a capability that disappears silently (contract §1 rule 4). The
    -- `object-kind` check guards this.
    -- No dependency on pier-api. The two are sibling capability packages with no edge
    -- between them (contract §1 rule 1). Synthetic events are spliced into
    -- subscribe_event resolution through the host's spi::EventProvider, so the direction
    -- is registering into the host and not calling api, and pier-api does not appear
    -- here.
    add_deps("pier-abi", "pier-support", "pier-host")
    add_includedirs("include", {public = true})
    -- Detours use ll/api/memory/Hook.h and event payloads read mc/ types, and this
    -- package brings in the includedirs for both. Without it not one TU here compiles.
    add_packages("levilamina")
    if not is_config("target_type", "client") then
        add_files("src/engine/*.cpp", "src/player/*.cpp",
                  "src/protect/*.cpp", "src/world/*.cpp")
    end
target_end()

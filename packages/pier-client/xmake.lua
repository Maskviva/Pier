-- pier-client: the capability group for client targets only, covering hotkey
-- bindings and the local player. It is a separate package rather than #ifdef blocks
-- inside pier-api because these are the only slots that exist on the client alone.
-- The reverse case, server-only slots, is expressed by not compiling a package in and
-- leaving NULL slots. This direction goes into its own package, so the client loader
-- target links one more package and the server target never sees it.
target("pier-client")
    set_kind("object")
    -- object and not static. Every TU in this package registers itself into the host
    -- SPI through a file-level static object and no external symbol references them.
    -- Inside a static library the linker drops such an object entirely, and the
    -- symptom is a capability that disappears silently (contract §1 rule 4). The
    -- `object-kind` check guards this.
    set_languages("c++20")
    add_defines("PIER_BUILD_CLIENT")
    add_files("src/**.cpp")
    add_deps("pier-abi", "pier-support", "pier-host")
    -- The package name is `levilamina`. The client and server difference lives in
    -- the configs of the root add_requires, not in the package name. No package named
    -- "levilamina-client" exists and naming one fails at the xmake configure step.
    add_packages("levilamina")
target_end()

-- Pier root build. It does three things: the global compile environment, the
-- external requirements, and gluing the object packages into one artifact. Anything
-- internal to a package belongs in that package's own xmake.lua.
add_rules("mode.debug", "mode.release")
add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

option("target_type")
    set_default("server")
    set_values("server", "client")
    set_description("Build target: server (BDS) or client (the MC client)")
option_end()

local is_client = (get_config("target_type") or "server") == "client"

-- The compile environment every package shares.
--
-- It is a function called from each package rather than root-scope `add_defines`,
-- `add_cxflags` and `set_languages`. xmake folds root-scope build settings into the
-- install hash it computes for a required package: with them at root scope,
-- `levilamina 26.20.4` resolved to a different hash than the identical requirement in
-- a project without them, no prebuilt install matched, and xmake cloned and compiled
-- LeviLamina from source on every clean machine. That build does not finish inside a
-- CI timeout, so the failure presents as a hang rather than as a configuration
-- problem.
--
-- The final target aggregates object packages and compiles no TU of its own, so
-- attaching these to it would reach nothing. Each package calls this instead.
--
-- `/utf-8` tells MSVC that sources are UTF-8, so neither parsing nor string encoding
-- depends on the machine's active code page. Without it, a file holding a byte outside
-- that code page raises C4819 once per file, which buries the real warnings and turns
-- into a hard error the moment anyone adds `/WX`. It sets both `/source-charset` and
-- `/execution-charset`; setting only the first leaves non-ASCII literals mangled at
-- runtime, which is harder to trace because it appears only once the server runs.
function pier_common()
    add_defines("NOMINMAX", "UNICODE", "_HAS_CXX23=1")
    add_cxflags("/utf-8", {tools = {"cl"}})
    set_languages("c++20")
    if is_client then
        -- The implementation-side target macro. abi.h does not know it, since the
        -- layout is identical on every target (contract §2.1). It only selects the
        -- server or client implementation branch inside the few shared TUs.
        add_defines("PIER_BUILD_CLIENT")
    end
end

if is_client then
    add_requires("levilamina 26.20.4", {configs = {target_type = "client"}})
else
    add_requires("levilamina 26.20.4", {configs = {target_type = "server"}})
    add_requires("legacymoney 0.19.0", {configs = {target_type = "server"}})
    add_requires("bedrockdata v26.20.5-server.4")
    add_requires("snappy")
    add_requires("magic_enum")
end
add_requires("prelink v0.7.1")
add_requires("levibuildscript")
add_requires("zlib 1.3.1")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

-- The packages. They are object targets (contract §1 rule 4), so every TU reaches
-- the final artifact and the linker does not discard a self-registering static
-- object as unreferenced.
includes("packages/pier-abi")
includes("packages/pier-support")
includes("packages/pier-host")
includes("packages/pier-api")
if not is_client then
    includes("packages/pier-hooks")
    includes("packages/pier-dimensions")
end
includes("packages/pier-lane")
if is_client then
    includes("packages/pier-client")
end

-- The only artifact. Glues the object packages above into one native LeviLamina mod.
--
-- The name is pier on both targets and does not follow target_type. Deriving it, as
-- in `is_client and "pier-client" or "pier"`, collides with the target of the
-- packages/pier-client capability package and xmake reports a duplicate definition.
-- The name should not drift with build configuration in any case. Server and client
-- are two targets of one host, and mods, commands and logs all say pier
-- (contract §7).
target("Pier")
    pier_common()
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    set_kind("shared")
    add_deps("pier-abi", "pier-support", "pier-host", "pier-api", "pier-lane")
    if not is_client then
        add_deps("pier-hooks", "pier-dimensions")
        add_packages("levilamina", "legacymoney", "bedrockdata", "snappy", "magic_enum")

        -- LegacyMoney is an optional backend. pier must come up without it and the
        -- economy family degrades to failing reads and failing writes (contract §2.1,
        -- an absent capability is an empty slot and not a different build). The
        -- runtime degradation is in place through `moneyBackendReady()`, which checks
        -- the mod table and the exported symbols. The linker half is what needs care:
        -- `add_packages("legacymoney")` links the import library directly and creates
        -- a hard DLL dependency, so the loader fails while pier.dll itself is being
        -- loaded, none of the runtime checks ever run, and the error reads
        -- `0x7E The specified module could not be found` with nothing to connect it
        -- to the economy.
        --
        -- /DELAYLOAD defers symbol resolution to the first real call. The matching
        -- delayimp.lib supplies the resolver stub, and without it the linker reports
        -- __delayLoadHelper2 as undefined.
        --
        -- shflags rather than ldflags: in xmake `add_ldflags` applies to
        -- `kind("binary")` only, and a shared library links through `add_shflags`.
        -- This target is `set_kind("shared")`, so an ldflags spelling is ignored
        -- silently. The build still succeeds and the artifact still carries a hard
        -- DLL dependency, with the same symptom as omitting the flag entirely. Both
        -- are written so that changing this target to binary later cannot reintroduce
        -- the problem.
        add_shflags("/DELAYLOAD:LegacyMoney.dll", { force = true })
        add_ldflags("/DELAYLOAD:LegacyMoney.dll", { force = true })
        add_syslinks("delayimp")
    else
        add_deps("pier-client")
        add_packages("levilamina")
    end
    add_packages("prelink", "zlib")
target_end()

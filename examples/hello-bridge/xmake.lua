-- hello-bridge: a native LeviLamina mod, built the way any native mod is built.
--
-- Pass -P . every time:
--
--   xmake f -P . -a x64 -m release -p windows
--   xmake -P .
--
-- Without -P, xmake walks up to the nearest parent project file, finds the Pier root and
-- builds Pier instead of this example.
--
-- There is no dependency on Pier here, and that is the point: pier-bridge.h is header-only
-- and binds its three symbols at runtime, so nothing links against Pier.dll. Declaring one
-- would make the loader fail before any of the runtime checks in the header could run.
add_rules("mode.debug", "mode.release")
add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

add_requires("levilamina 26.40.0", {configs = {target_type = "server"}})
add_requires("levibuildscript 0.6.1")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("hello-bridge")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    set_kind("shared")
    set_languages("c++20")
    add_defines("NOMINMAX", "UNICODE")
    add_cxflags("/utf-8", {tools = {"cl"}})
    add_files("src/*.cpp")
    add_packages("levilamina")

    -- The one include the bridge needs. Copy pier-bridge.h into a project of your own
    -- instead of pointing at this path; it is header-only and has no other dependency.
    add_includedirs("../../bindings/bridge")

    -- Has to match the "entry" field of manifest.json.
    set_basename("hello_bridge")
target_end()

-- pier-probe builds on its own, the way a third-party mod does.
--
-- That is the point of it: a mod that lived inside the Pier build could pick up an
-- include path or a define the real consumer does not have, and the first thing this
-- example is meant to prove is that abi.h alone is enough. It links against nothing from
-- Pier and includes exactly one header.
--
-- Two build systems are not better than one, so read build-msvc.bat first: four files
-- against one header need no project file at all, and that is the claim this example
-- exists to make. This xmake.lua is here for someone whose own mod already uses xmake.
--
-- If you do use it, pass -P . every time:
--
--   xmake f -P . -a x64 -m release -p windows
--   xmake -P .
--
-- Without -P, xmake walks up to the nearest parent project file, finds the Pier root and
-- builds Pier instead of this example. It prints a warning saying so and then does it
-- anyway, so a build that takes eighty seconds and produces Pier.dll is the symptom.

add_rules("mode.debug", "mode.release")

target("pier-probe")
    set_kind("shared")
    set_languages("c++20")
    add_files("src/*.cpp")
    add_includedirs("include")

    -- The one dependency. A relative path rather than a package, so the example builds
    -- against the abi.h sitting next to it in the repository and a change to the
    -- contract is visible here on the next build instead of on the next release.
    add_includedirs("../../packages/pier-abi/include")

    set_basename("pier_probe")

    if is_plat("windows") then
        add_cxflags("/EHsc", "/utf-8")
    end

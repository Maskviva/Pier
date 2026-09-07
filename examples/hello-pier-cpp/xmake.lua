-- xmake.lua — one of four build files that produce the same DLL.
--
-- Here for someone whose own mod already uses xmake. If you are choosing, build-msvc.bat
-- is the shorter answer: one header and no library needs no project file.
--
-- Pass -P . every time:
--
--   xmake f -P . -a x64 -m release -p windows
--   xmake -P .
--
-- Without -P, xmake walks up to the nearest parent project file, finds the Pier root and
-- builds Pier instead of this example. It warns and then does it anyway, so a build that
-- takes over a minute and produces Pier.dll is the symptom.

add_rules("mode.debug", "mode.release")

target("hello-pier-cpp")
    set_kind("shared")
    set_languages("c++20")
    add_files("src/*.cpp")

    -- The one dependency.
    add_includedirs("../../packages/pier-abi/include")

    -- Has to match the "entry" field of manifest.json.
    set_basename("hello_pier_cpp")

    if is_plat("windows") then
        add_cxflags("/EHsc", "/utf-8")
    end

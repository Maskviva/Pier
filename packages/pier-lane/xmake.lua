-- pier-lane: the same-toolchain fast lane. An optional capability.
--
-- The host interprets no byte of the data, vtable or fingerprint carried over a lane
-- and only compares for equality and tracks liveness. It is therefore a direct link
-- between two cdylibs built by the same toolchain, not a direct link between two mods
-- in one language. Remove this package and service_call, whose shape is
-- (name, UTF-8) -> UTF-8 and is the greatest common divisor across languages, keeps
-- working. An absent slot is NULL and the SDK degrades under the empty-slot rule.
target("pier-lane")
    set_kind("object")
    -- object and not static. Every TU in this package registers itself into the host
    -- SPI through a file-level static object and no external symbol references them.
    -- Inside a static library the linker drops such an object entirely, and the
    -- symptom is a capability that disappears silently (contract §1 rule 4). The
    -- `object-kind` check guards this.
    set_languages("c++20")
    add_deps("pier-abi", "pier-support", "pier-host")
    -- Lane.cpp writes no ll/ include of its own, but `pier/host/hosted_mod.h` does.
    -- The compiler expands the closure of includes, not the first level. Omitting this
    -- line produces `fatal error C1083: cannot open include file` naming a header this
    -- package never mentions. The `build-config` check computes the closure.
    add_packages("levilamina")
    add_files("src/*.cpp")
target_end()

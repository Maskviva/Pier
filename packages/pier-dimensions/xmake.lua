-- pier-dimensions: custom dimensions along the real-dimension route. An optional
-- capability, server only.
--
-- It is exposed through the ABI while its implementation stands on its own. It lives
-- here rather than in pier-api because opening an interface and implementing a
-- capability are two jobs, and mixing them blurs the boundary.
--
-- It is also the only implementation of spi::DimensionBridge. blockSourceOf and
-- dimensionSelector on the api side know nothing about custom dimensions and reach
-- src/rt/Bridge.cpp entirely through that bridge. Without this package the bridge is
-- absent, the api side recognizes only the three vanilla dimensions and warns once per
-- function, which is a deliberate degradation and not a failure.
target("pier-dimensions")
    pier_common()
    set_kind("object")
    -- object and not static. Every TU in this package registers itself into the host
    -- SPI through a file-level static object and no external symbol references them.
    -- Inside a static library the linker drops such an object entirely, and the
    -- symptom is a capability that disappears silently (contract §1 rule 4). The
    -- `object-kind` check guards this.
    -- No dependency on pier-api. This package is its sibling capability package. When
    -- the api side needs to ask about a custom dimension it goes through the host's
    -- spi::DimensionBridge, which this package registers in rt/Bridge.cpp. The
    -- direction is registering into the host and not calling each other, so pier-api
    -- does not appear here.
    add_deps("pier-abi", "pier-support", "pier-host")
    add_includedirs("include", {public = true})
    add_packages("levilamina", "snappy", "magic_enum")
    if not is_config("target_type", "client") then
        add_files("src/base/*.cpp", "src/dim/*.cpp", "src/plot/*.cpp", "src/rt/*.cpp")
    end
target_end()

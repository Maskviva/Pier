-- pier-host: mod lifecycle, the SPI registration point, and owner of the PierApi
-- table. All it knows about is a cdylib, the pier_main it exports, the PierModVTable
-- it fills back in, and whatever capability packages register through spi.h. It knows
-- no concrete capability.
target("pier-host")
    pier_common()
    set_kind("object")
    add_deps("pier-abi", "pier-support")
    add_includedirs("include", {public = true})
    add_files("src/**.cpp")
    add_packages("levilamina")
target_end()

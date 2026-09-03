-- pier-api: the core domain implementations, covering core, runtime, actors, world
-- and net. It is one capability package among others (contract §1 rule 1) and holds
-- no privilege. Its includes are private and every sideways collaboration goes
-- through the pier-host SPI.
target("pier-api")
    pier_common()
    set_kind("object")
    add_deps("pier-abi", "pier-support", "pier-host")
    add_includedirs("include")            -- Private. No other package may include it.
    add_files("src/**.cpp")
    add_packages("levilamina")
    if not is_config("target_type", "client") then
        add_packages("legacymoney", "bedrockdata")
    end
target_end()

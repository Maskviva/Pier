-- pier-support: cross-package helpers. PierStr and string_view conversion, SNBT
-- escaping, and the logging entry point. Depends only on pier-abi and LeviLamina.
-- It knows nothing about the host and nothing about any capability package.
target("pier-support")
    pier_common()
    set_kind("object")
    add_deps("pier-abi")
    add_includedirs("include", {public = true})
    add_files("src/**.cpp")
    add_packages("levilamina")
target_end()

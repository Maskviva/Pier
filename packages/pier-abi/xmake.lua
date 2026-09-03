-- pier-abi: the contract package. Pure C headers, no source files, no dependencies
-- (contract §0 and §1). Another language needs only this package to reach Pier. It
-- may not depend on any other package and may not contain a .cpp.
target("pier-abi")
    pier_common()
    set_kind("headeronly")
    add_includedirs("include", {public = true})
    add_headerfiles("include/(sdk/*.h)")
target_end()

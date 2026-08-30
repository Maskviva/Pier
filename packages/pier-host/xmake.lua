-- pier-host —— 模组生命周期 + SPI 注册处 + PierApi 表的所有者。
-- 它认识的只有：一个 cdylib、它导出的 pier_main、它填回来的 PierModVTable，
-- 以及各能力包通过 spi.h 注册进来的东西。不认识任何具体能力。
target("pier-host")
    set_kind("object")
    add_deps("pier-abi", "pier-support")
    add_includedirs("include", {public = true})
    add_files("src/**.cpp")
    add_packages("levilamina")
target_end()

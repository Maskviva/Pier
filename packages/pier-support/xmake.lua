-- pier-support —— 跨包小工具：PierStr↔string_view、SNBT 转义、日志入口。
-- 只依赖 abi 和 LeviLamina；不认识宿主，更不认识任何能力包。
target("pier-support")
    set_kind("object")
    add_deps("pier-abi")
    add_includedirs("include", {public = true})
    add_files("src/**.cpp")
    add_packages("levilamina")
target_end()

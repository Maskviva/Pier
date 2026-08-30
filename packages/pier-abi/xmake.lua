-- pier-abi —— 契约包。纯 C 头，零源文件，零依赖（契约 §〇 §一）。
-- 别的语言接入 Pier 只需要这一个包。它不许依赖任何其它包，也不许有 .cpp。
target("pier-abi")
    set_kind("headeronly")
    add_includedirs("include", {public = true})
    add_headerfiles("include/(sdk/*.h)")
target_end()

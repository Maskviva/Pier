-- pier-api —— 核心域实现（core / runtime / actors / world / net）。
-- 它也是能力包之一（契约 §一 规则一），不享有特权：include 私有，
-- 横向协作一律走 pier-host 的 SPI。
target("pier-api")
    set_kind("object")
    add_deps("pier-abi", "pier-support", "pier-host")
    add_includedirs("include")            -- 私有：别的包不许 include 我
    add_files("src/**.cpp")
    add_packages("levilamina")
    if not is_config("target_type", "client") then
        add_packages("legacymoney", "bedrockdata")
    end
target_end()

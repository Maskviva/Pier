-- pier-client：仅客户端目标的能力组（热键绑定、本地玩家）。
-- 单独成包而不是塞在 pier-api 里带 #ifdef：这是唯一一组"只在客户端存在"
-- 的槽位，反向的（服务端专属）靠"不编入 + NULL 槽"表达；正向的放进
-- 独立包，客户端 loader 目标多链一个包即可，服务端目标完全看不见它。
target("pier-client")
    set_kind("object")
    -- object 而不是 static：本包的每个 TU 都靠**文件级静态对象**把自己注册进
    -- 宿主的 SPI，没有任何外部符号引用它们。静态库里这种 obj 会被链接器整个
    -- 丢掉，症状是功能**静默消失**（契约 §一 规则四）。`object-kind` 机检守着。
    set_languages("c++20")
    add_defines("PIER_BUILD_CLIENT")
    add_files("src/**.cpp")
    add_deps("pier-abi", "pier-support", "pier-host")
    -- 包名是 `levilamina`，客户端/服务端的差别在根 add_requires 的 configs
    -- 里，不在包名里。这里曾写成 "levilamina-client" —— 那个包不存在，
    -- xmake 配置阶段就会失败。
    add_packages("levilamina")
target_end()

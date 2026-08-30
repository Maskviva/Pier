-- pier-hooks —— 用原生 detour 合成的事件。
--
-- 这些事件在 LeviLamina 和原版里都没有对应的可订阅项；每个 .cpp 一个事件，
-- 自注册成一个 EventProvider 的条目，加一个事件不用改任何表。全部服务端专属。
target("pier-hooks")
    set_kind("object")
    -- object 而不是 static：本包的每个 TU 都靠**文件级静态对象**把自己注册进
    -- 宿主的 SPI，没有任何外部符号引用它们。静态库里这种 obj 会被链接器整个
    -- 丢掉，症状是功能**静默消失**（契约 §一 规则四）。`object-kind` 机检守着。
    set_languages("c++20")
    -- 不依赖 pier-api：本包和它是**兄弟能力包**，之间零边（契约 §一 规则一）。
    -- 合成事件接进 subscribe_event 的解析，走的是宿主的 spi::EventProvider，
    -- 方向是「注册进宿主」而不是「调用 api」—— 所以这里没有 pier-api。
    add_deps("pier-abi", "pier-support", "pier-host")
    add_includedirs("include", {public = true})
    -- detour 用 ll/api/memory/Hook.h，事件载荷读的是 mc/ 的类型 —— 两者的
    -- includedirs 都由这个包带进来。漏了它这个包一个 TU 都编不过。
    add_packages("levilamina")
    if not is_config("target_type", "client") then
        add_files("src/engine/*.cpp", "src/player/*.cpp",
                  "src/protect/*.cpp", "src/world/*.cpp")
    end
target_end()

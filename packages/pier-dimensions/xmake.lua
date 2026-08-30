-- pier-dimensions —— 自定义维度（真维度路线）。**可选能力**，服务端专属。
--
-- 它通过 ABI 暴露，但实现自成一体。放在这里而不是 pier-api 里，是因为
-- 「开放接口」和「实现某个能力」是两件事，混在一起界线会糊掉。
--
-- 它还是 spi::DimensionBridge 的**唯一实现方**：api 侧的 blockSourceOf /
-- dimensionSelector 对自定义维度一无所知，全部经这个桥落到 src/rt/Bridge.cpp。
-- 不编入这个包时桥缺席，api 侧只认原版三维度并各打一次告警 —— 那是刻意的
-- 降级，不是失败。
target("pier-dimensions")
    set_kind("object")
    -- object 而不是 static：本包的每个 TU 都靠**文件级静态对象**把自己注册进
    -- 宿主的 SPI，没有任何外部符号引用它们。静态库里这种 obj 会被链接器整个
    -- 丢掉，症状是功能**静默消失**（契约 §一 规则四）。`object-kind` 机检守着。
    set_languages("c++20")
    -- 不依赖 pier-api：本包是它的**兄弟能力包**。api 侧要问自定义维度的事，
    -- 走的是宿主的 spi::DimensionBridge（本包在 rt/Bridge.cpp 里注册进去），
    -- 方向是「注册进宿主」而不是「互相调用」—— 所以这里没有 pier-api。
    add_deps("pier-abi", "pier-support", "pier-host")
    add_includedirs("include", {public = true})
    add_packages("levilamina", "snappy", "magic_enum")
    if not is_config("target_type", "client") then
        add_files("src/base/*.cpp", "src/dim/*.cpp", "src/plot/*.cpp", "src/rt/*.cpp")
    end
target_end()

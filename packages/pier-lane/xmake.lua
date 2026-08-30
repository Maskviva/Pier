-- pier-lane —— 同工具链快车道。**可选能力**。
--
-- 宿主对车道里传的 data / vtable / 指纹一个字节都不解释，只做相等比较和
-- 存活标记。所以它不是「某语言直连」，是「同一次工具链编出来的两个 cdylib
-- 直连」。删掉这个包，service_call（(名字, UTF-8) -> UTF-8，跨语言最大公约
-- 数）照常工作 —— 槽位缺席即 NULL，SDK 按空槽纪律降级。
target("pier-lane")
    set_kind("object")
    -- object 而不是 static：本包的每个 TU 都靠**文件级静态对象**把自己注册进
    -- 宿主的 SPI，没有任何外部符号引用它们。静态库里这种 obj 会被链接器整个
    -- 丢掉，症状是功能**静默消失**（契约 §一 规则四）。`object-kind` 机检守着。
    set_languages("c++20")
    add_deps("pier-abi", "pier-support", "pier-host")
    -- Lane.cpp 自己一行 ll/ 的 include 都没写，但 `pier/host/hosted_mod.h`
    -- 里有 —— 编译器展开的是 include 的**闭包**，不是第一层。漏了这一行的
    -- 症状是 `fatal error C1083: 无法打开包括文件`，而报出来的文件名是
    -- 一个这个包里根本没提过的头。`build-config` 机检现在按闭包算。
    add_packages("levilamina")
    add_files("src/*.cpp")
target_end()

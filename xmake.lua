-- Pier —— 根构建。只做三件事：全局编译环境、外部依赖、把 object 包粘成产物。
-- 任何和某个包内部相关的东西都属于那个包自己的 xmake.lua。
add_rules("mode.debug", "mode.release")
add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

option("target_type")
    set_default("server")
    set_values("server", "client")
    set_description("构建目标：server（BDS）或 client（MC 客户端）")
option_end()

local is_client = (get_config("target_type") or "server") == "client"

-- 全局编译宏放**根作用域**，不放任何 target 里。
-- v1 的事故：这些宏挂在只做聚合、不编译任何 TU 的最终 target 上，
-- 等于一个 TU 都没收到 —— 症状是各包各自缺 NOMINMAX / 目标宏。
add_defines("NOMINMAX", "UNICODE", "_HAS_CXX23=1")

-- 告诉 MSVC「源文件是 UTF-8」。
--
-- 不加这一行的后果：中文注释在非 UTF-8 代码页（简中默认 936）下，每个文件
-- 报一条 `C4819: 该文件包含不能在当前代码页中表示的字符`。它是警告不是错误，
-- 但危害是实的 —— 一次全量构建会刷出上百条，把**真正的**警告淹掉；而一旦
-- 有人加了 `/WX`，它立刻变成硬错。
--
-- 两个都要：`/source-charset` 管读入，`/execution-charset` 管字符串字面量
-- 编码成什么。只设前者的话，日志里的中文在运行期会变成乱码 —— 那比警告
-- 更难查，因为它要跑起来才显形。`/utf-8` 是这两个的合写。
add_cxflags("/utf-8", {tools = {"cl"}})
if is_client then
    -- 实现侧的目标宏。abi.h **不认识它** —— 布局在所有目标下相同（契约 §2.1），
    -- 它只用于个别共享 TU 里挑选服务端/客户端的实现分支。
    add_defines("PIER_BUILD_CLIENT")
end
set_languages("c++20")

if is_client then
    add_requires("levilamina 26.20.4", {configs = {target_type = "client"}})
else
    add_requires("levilamina 26.20.4", {configs = {target_type = "server"}})
    add_requires("legacymoney 0.19.0", {configs = {target_type = "server"}})
    add_requires("bedrockdata v26.20.5-server.4")
    add_requires("snappy")
    add_requires("magic_enum")
end
add_requires("prelink v0.7.1")
add_requires("levibuildscript")
add_requires("zlib 1.3.1")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

-- 分包。object 类型（契约 §一 规则四）：每个 TU 必然进入最终产物，
-- 自注册的静态对象不会被链接器当作「无人引用」丢弃。
includes("packages/pier-abi")
includes("packages/pier-support")
includes("packages/pier-host")
includes("packages/pier-api")
if not is_client then
    includes("packages/pier-hooks")
    includes("packages/pier-dimensions")
end
includes("packages/pier-lane")
if is_client then
    includes("packages/pier-client")
end

-- 唯一的产物：把上面的 object 包粘成一个 LeviLamina 原生 mod。
--
-- 名字**两个目标下都叫 pier**，不随 target_type 改。旧仓这里写的是
-- `is_client and "pier-client" or "pier"`，那时候没有 packages/pier-client；
-- 新树把客户端槽位拆成独立能力包之后，这个表达式会和那个包的 target 重名，
-- xmake 直接报重复定义。而且名字本来就不该随构建配置漂 —— 服务端和客户端
-- 是同一个宿主的两个目标，模组、命令、日志里说的都是「pier」（契约 §七）。
target("pier")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    set_kind("shared")
    add_deps("pier-abi", "pier-support", "pier-host", "pier-api", "pier-lane")
    if not is_client then
        add_deps("pier-hooks", "pier-dimensions")
        add_packages("levilamina", "legacymoney", "bedrockdata", "snappy", "magic_enum")
    else
        add_deps("pier-client")
        add_packages("levilamina")
    end
    add_packages("prelink", "zlib")
target_end()

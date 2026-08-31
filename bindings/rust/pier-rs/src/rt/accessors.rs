//! 从属性常量表生成访问器。
//!
//! `PierApi` 的属性族是「一个数值槽 + 一张常量表」的形状：读玩家的饥饿值和
//! 读他在不在飞，走的是同一个 `player_get_num`，只差一个常量。手写出来就是
//! 几十个长得一样的一行函数，而抄错常量编译器不会说话 —— 类型全一样。
//!
//! 表格把常量和方法名并排放，抄错看得见。它消掉的是**源码里的重复**，
//! 不是 API 面：`Player` 上仍然有几十个方法，那是 ABI 的形状。

/// 按 `种类 方法名 = 常量;` 逐行生成访问器。
///
/// 种类决定返回类型与取值路径：`f64`/`i32`/`bool` 走 `num`，`str` 走 `text`。
/// 目标类型只需提供这两个方法 —— `i32` 的截断和 `bool` 的非零判定由下面的
/// `@get` 分支做，那是它们**唯一**的定义处。三个域各自留一份 `num_i32` /
/// `num_bool` 私有 helper 曾经存在，属性墙搬进表格之后它们就成了死代码。
///
/// 文档可选 —— `///` 在宏匹配前已脱糖成属性。给 `hunger()` 补一句「饥饿值」
/// 只是把方法名再说一遍；只在名字说不清时写（单位、取值域、和邻近属性的区别）。
macro_rules! accessors {
    ($ty:ty; $( $(#[$m:meta])* $kind:ident $name:ident = $konst:ident; )*) => {
        impl $ty {
            $(
                $(#[$m])*
                pub fn $name(&self) -> $crate::rt::error::Result<accessors!(@ret $kind)> {
                    accessors!(@get self, $kind, $crate::sys::$konst)
                }
            )*
        }
    };

    (@ret f64) => { f64 };
    (@ret i32) => { i32 };
    (@ret bool) => { bool };
    (@ret str) => { ::std::string::String };

    (@get $s:expr, f64, $k:expr) => { $s.num($k) };
    (@get $s:expr, i32, $k:expr) => { $s.num($k).map(|v| v as i32) };
    (@get $s:expr, bool, $k:expr) => { $s.num($k).map(|v| v != 0.0) };
    (@get $s:expr, str, $k:expr) => { $s.text($k) };
}

pub(crate) use accessors;

#pragma once
// SNBT 组装的最小助手。所有跨边界的复合载荷（事件数据、玩家摘要）都是
// SNBT 文本 —— 助手只此一份，两处转义规则不一致就是一处注入口。
#include <format>
#include <string>
#include <string_view>
#include <type_traits>

namespace pier
{
    /** SNBT 字符串字面量内容转义：\ " 原样转义，\n \r \t 写成转义序列。
     *  玩家能往聊天/表单里粘贴任何东西；裸控制字符进了字面量，解析端
     *  要么读错要么报错 —— 这里是唯一的防线，别在调用点手拼引号。 */
    [[nodiscard]] std::string snbtEscape(std::string_view s);

    /** 数字 → SNBT 文本。整数走 std::to_string（有全套有/无符号重载）；
     *  浮点走 std::format 的最短往返形式 —— %g 丢精度，%.17g 把 0.1 打成
     *  17 位，都不要。做成模板而不是一对重载：调用点混着 int / size_t /
     *  int64 / float，重载版对 size_t 会在 long long 和 double 之间歧义。 */
    template <class T>
    [[nodiscard]] std::string snbtNum(T v)
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            return std::format("{}", static_cast<double>(v));
        }
        else
        {
            static_assert(std::is_integral_v<T>, "snbtNum 只收数值");
            return std::to_string(v);
        }
    }

    /** 带引号的完整字符串字面量："..."（内容已转义）。 */
    [[nodiscard]] std::string snbtStr(std::string_view s);
} // namespace pier

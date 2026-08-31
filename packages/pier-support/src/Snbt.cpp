#include "pier/support/snbt.h"

namespace pier
{
    namespace
    {
        /** 返回从 `i` 起一个合法 UTF-8 序列的长度（1..4）；非法返回 0。 */
        size_t utf8SeqLen(std::string_view s, size_t i)
        {
            auto const b0 = static_cast<unsigned char>(s[i]);
            size_t need = 0;
            if (b0 < 0x80) return 1;
            if (b0 >= 0xC2 && b0 <= 0xDF) need = 1;
            else if (b0 >= 0xE0 && b0 <= 0xEF) need = 2;
            else if (b0 >= 0xF0 && b0 <= 0xF4) need = 3;
            else return 0;
            for (size_t k = 1; k <= need; ++k)
            {
                if (i + k >= s.size()) return 0;
                auto const bk = static_cast<unsigned char>(s[i + k]);
                if ((bk & 0xC0) != 0x80) return 0;
            }
            // 拒绝过长编码与代理区
            auto const b1 = static_cast<unsigned char>(s[i + 1]);
            if (b0 == 0xE0 && b1 < 0xA0) return 0;
            if (b0 == 0xED && b1 >= 0xA0) return 0;
            if (b0 == 0xF0 && b1 < 0x90) return 0;
            if (b0 == 0xF4 && b1 >= 0x90) return 0;
            return need + 1;
        }
    } // namespace

    std::string snbtEscape(std::string_view s)
    {
        std::string out;
        out.reserve(s.size() + 2);
        for (size_t i = 0; i < s.size();)
        {
            char const c = s[i];
            auto const uc = static_cast<unsigned char>(c);
            if (uc >= 0x80)
            {
                // 非法 UTF-8 不能原样透传 —— SDK 侧 from_utf8 会失败，整条
                // 载荷在这里被截断，其后的 dim/取消位全部丢失。改写成 U+FFFD。
                size_t const n = utf8SeqLen(s, i);
                if (n == 0)
                {
                    out += "\xEF\xBF\xBD";
                    ++i;
                }
                else
                {
                    out.append(s.data() + i, n);
                    i += n;
                }
                continue;
            }
            switch (c)
            {
            case '"':
            case '\\':
                out.push_back('\\');
                out.push_back(c);
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                // 其余控制字符按 LL 自己的 toSnbt 规则写成 \uXXXX，
                // 别让 0x01/0x08/0x0C 之类原样进入字符串字面量。
                if (uc < 0x20 || uc == 0x7F)
                {
                    static constexpr char hex[] = "0123456789abcdef";
                    out += "\\u00";
                    out.push_back(hex[(uc >> 4) & 0xF]);
                    out.push_back(hex[uc & 0xF]);
                }
                else
                {
                    out.push_back(c);
                }
                break;
            }
            ++i;
        }
        return out;
    }

    std::string snbtStr(std::string_view s) { return '"' + snbtEscape(s) + '"'; }
} // namespace pier

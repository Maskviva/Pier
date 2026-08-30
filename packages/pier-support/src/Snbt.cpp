#include "pier/support/snbt.h"

namespace pier
{
    std::string snbtEscape(std::string_view s)
    {
        std::string out;
        out.reserve(s.size() + 2);
        for (char c : s)
        {
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
                out.push_back(c);
                break;
            }
        }
        return out;
    }

    std::string snbtStr(std::string_view s) { return '"' + snbtEscape(s) + '"'; }
} // namespace pier

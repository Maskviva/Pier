/** Expr.cpp: validation and evaluation of the EXPR node table. */
#include "pier/dimensions/pack/expr.h"

#include <cstdlib>

namespace pier::dimensions::pack
{
    namespace
    {
        std::int64_t floorDiv(std::int64_t a, std::int64_t b)
        {
            std::int64_t q = a / b;
            std::int64_t r = a % b;
            if (r != 0 && ((r < 0) != (b < 0))) --q;
            return q;
        }

        std::int64_t floorMod(std::int64_t a, std::int64_t b)
        {
            std::int64_t r = a % b;
            if (r != 0 && ((r < 0) != (b < 0))) r += b;
            return r;
        }
    } // namespace

    bool validateExpr(std::vector<ExprNode> const& nodes, std::size_t paramCount, std::vector<std::string>& problems)
    {
        bool ok = true;
        for (std::size_t i = 0; i < nodes.size(); ++i)
        {
            auto const& n = nodes[i];
            if (n.op >= static_cast<std::uint16_t>(ExprOp::Count_))
            {
                problems.push_back("EXPR node " + std::to_string(i) + ": unknown op " + std::to_string(n.op));
                ok = false;
                continue;
            }
            auto op = static_cast<ExprOp>(n.op);
            if (op == ExprOp::Param && (n.imm < 0 || static_cast<std::size_t>(n.imm) >= paramCount))
            {
                problems.push_back("EXPR node " + std::to_string(i) + ": parameter index out of range");
                ok = false;
            }
            bool unary = op == ExprOp::Neg || op == ExprOp::Abs || op == ExprOp::Not;
            bool binary = op != ExprOp::Const && op != ExprOp::Param && !unary;
            if ((unary || binary) && (n.a == kNone || n.a >= i))
            {
                problems.push_back("EXPR node " + std::to_string(i) + ": operand a is not an earlier node");
                ok = false;
            }
            if (binary && (n.b == kNone || n.b >= i))
            {
                problems.push_back("EXPR node " + std::to_string(i) + ": operand b is not an earlier node");
                ok = false;
            }
        }
        return ok;
    }

    bool evaluateExpr(std::vector<ExprNode> const& nodes, std::vector<std::int64_t> const& params,
                      std::vector<std::int64_t>& out, std::vector<std::string>& problems)
    {
        out.assign(nodes.size(), 0);
        for (std::size_t i = 0; i < nodes.size(); ++i)
        {
            auto const& n = nodes[i];
            std::int64_t a = n.a == kNone ? 0 : out[n.a];
            std::int64_t b = n.b == kNone ? 0 : out[n.b];
            std::int64_t v = 0;
            switch (static_cast<ExprOp>(n.op))
            {
            case ExprOp::Const: v = n.imm; break;
            case ExprOp::Param: v = params[static_cast<std::size_t>(n.imm)]; break;
            case ExprOp::Add: v = a + b; break;
            case ExprOp::Sub: v = a - b; break;
            case ExprOp::Mul: v = a * b; break;
            case ExprOp::Div:
            case ExprOp::Mod:
                if (b == 0)
                {
                    problems.push_back("EXPR node " + std::to_string(i) + ": division by zero with these parameters");
                    return false;
                }
                v = static_cast<ExprOp>(n.op) == ExprOp::Div ? floorDiv(a, b) : floorMod(a, b);
                break;
            case ExprOp::Min: v = a < b ? a : b; break;
            case ExprOp::Max: v = a > b ? a : b; break;
            case ExprOp::Neg: v = -a; break;
            case ExprOp::Abs: v = a < 0 ? -a : a; break;
            case ExprOp::Lt: v = a < b ? 1 : 0; break;
            case ExprOp::Le: v = a <= b ? 1 : 0; break;
            case ExprOp::Eq: v = a == b ? 1 : 0; break;
            case ExprOp::And: v = (a != 0 && b != 0) ? 1 : 0; break;
            case ExprOp::Or: v = (a != 0 || b != 0) ? 1 : 0; break;
            case ExprOp::Not: v = a == 0 ? 1 : 0; break;
            default:
                problems.push_back("EXPR node " + std::to_string(i) + ": unknown op");
                return false;
            }
            out[i] = v;
        }
        return true;
    }
} // namespace pier::dimensions::pack

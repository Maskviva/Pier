/** expr.h: evaluation of a pack's EXPR table against bound parameter values. */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pier/dimensions/pack/pack_format.h"

namespace pier::dimensions::pack
{
    /** Checks every node once: known op, operands strictly before the node, parameter
     *  index inside paramCount. Messages go to problems; false when any node fails. */
    bool validateExpr(std::vector<ExprNode> const& nodes, std::size_t paramCount, std::vector<std::string>& problems);

    /** Evaluates every node in index order into out. Division and modulo floor toward
     *  negative infinity, as in the tool; a zero divisor makes the call fail with a
     *  message. Values are 64-bit so intermediate products do not wrap. */
    bool evaluateExpr(std::vector<ExprNode> const& nodes, std::vector<std::int64_t> const& params,
                      std::vector<std::int64_t>& out, std::vector<std::string>& problems);
} // namespace pier::dimensions::pack

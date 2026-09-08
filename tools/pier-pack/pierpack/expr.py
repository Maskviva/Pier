"""expr.py: integer expressions over pack parameters, compiled into EXPR nodes.

The source syntax is the usual infix one: + - * / % with floor semantics, unary -,
comparisons < <= > >= == !=, && || !, and the functions min(a,b), max(a,b), abs(a),
clamp(v,lo,hi). Identifiers are parameter names. A compiled expression is an index into a
shared node table where every operand index is smaller than the node's own, constants
are folded and identical subexpressions are shared, so the C++ side evaluates the table
in one forward pass.
"""
from __future__ import annotations

import re
from dataclasses import dataclass, field

from . import format as F

_TOKEN = re.compile(r"\s*(?:(\d+)|([A-Za-z_][A-Za-z0-9_]*)|(<=|>=|==|!=|&&|\|\||[-+*/%(),<>!]))")

_BINARY = {
    "+": "add", "-": "sub", "*": "mul", "/": "div", "%": "mod",
    "<": "lt", "<=": "le", "==": "eq",
    "&&": "and", "||": "or",
}


@dataclass
class ExprTable:
    """The shared node table one pack compiles into."""
    params: dict            # name -> index
    nodes: list = field(default_factory=list)
    _memo: dict = field(default_factory=dict)

    def _emit(self, op: str, a: int = F.NONE, b: int = F.NONE, imm: int = 0) -> int:
        key = (op, a, b, imm)
        if key in self._memo:
            return self._memo[key]
        idx = len(self.nodes)
        self.nodes.append({"op": F.EXPR_OP[op], "reserved": 0, "a": a, "b": b, "imm": imm})
        self._memo[key] = idx
        return idx

    def const(self, v: int) -> int:
        return self._emit("const", imm=int(v))

    def param(self, name: str) -> int:
        if name not in self.params:
            raise ValueError(f"unknown parameter '{name}'")
        return self._emit("param", imm=self.params[name])

    def is_const(self, idx: int):
        n = self.nodes[idx]
        return n["op"] == F.EXPR_OP["const"]

    def const_value(self, idx: int) -> int:
        return self.nodes[idx]["imm"]

    def binop(self, op: str, a: int, b: int) -> int:
        if self.is_const(a) and self.is_const(b):
            return self.const(eval_op(op, self.const_value(a), self.const_value(b)))
        # a*1, a+0 and the like are left alone: they are rare in a spec and folding them
        # would make the emitted table harder to read back against the source.
        return self._emit(op, a, b)

    def unop(self, op: str, a: int) -> int:
        if self.is_const(a):
            return self.const(eval_op(op, self.const_value(a), 0))
        return self._emit(op, a)

    def compile(self, text) -> int:
        if isinstance(text, bool):
            return self.const(1 if text else 0)
        if isinstance(text, int):
            return self.const(text)
        return _Parser(str(text), self).parse()


def floordiv(a: int, b: int) -> int:
    if b == 0:
        raise ZeroDivisionError("division by zero in an expression")
    return a // b


def floormod(a: int, b: int) -> int:
    if b == 0:
        raise ZeroDivisionError("modulo by zero in an expression")
    return a % b


def eval_op(op: str, a: int, b: int) -> int:
    if op == "add":
        return a + b
    if op == "sub":
        return a - b
    if op == "mul":
        return a * b
    if op == "div":
        return floordiv(a, b)
    if op == "mod":
        return floormod(a, b)
    if op == "min":
        return min(a, b)
    if op == "max":
        return max(a, b)
    if op == "neg":
        return -a
    if op == "abs":
        return abs(a)
    if op == "lt":
        return 1 if a < b else 0
    if op == "le":
        return 1 if a <= b else 0
    if op == "eq":
        return 1 if a == b else 0
    if op == "and":
        return 1 if (a != 0 and b != 0) else 0
    if op == "or":
        return 1 if (a != 0 or b != 0) else 0
    if op == "not":
        return 1 if a == 0 else 0
    raise ValueError(op)


def evaluate(nodes: list, values: list) -> list:
    """Evaluate every node in order. `values` is the bound parameter list; returns the
    value of every node, in index order. This is the reference the C++ evaluator must
    match."""
    out = [0] * len(nodes)
    for i, n in enumerate(nodes):
        op = F.EXPR_OPS[n["op"]]
        if op == "const":
            out[i] = n["imm"]
        elif op == "param":
            out[i] = values[n["imm"]]
        else:
            a = out[n["a"]] if n["a"] != F.NONE else 0
            b = out[n["b"]] if n["b"] != F.NONE else 0
            if n["a"] != F.NONE and n["a"] >= i:
                raise ValueError(f"node {i} refers forward to {n['a']}")
            if n["b"] != F.NONE and n["b"] >= i:
                raise ValueError(f"node {i} refers forward to {n['b']}")
            out[i] = eval_op(op, a, b)
    return out


class _Parser:
    """Precedence climbing over the token stream; binds to the table on the way out."""

    def __init__(self, text: str, table: ExprTable):
        self.text = text
        self.t = table
        self.tokens = []
        pos = 0
        while pos < len(text):
            m = _TOKEN.match(text, pos)
            if not m or m.end() == pos:
                if text[pos:].strip() == "":
                    break
                raise ValueError(f"cannot read expression at '{text[pos:]}'")
            num, ident, sym = m.groups()
            if num is not None:
                self.tokens.append(("num", int(num)))
            elif ident is not None:
                self.tokens.append(("id", ident))
            else:
                self.tokens.append(("sym", sym))
            pos = m.end()
        self.i = 0

    def peek(self):
        return self.tokens[self.i] if self.i < len(self.tokens) else (None, None)

    def take(self, kind=None, val=None):
        tok = self.peek()
        if tok[0] is None:
            raise ValueError(f"unexpected end of expression '{self.text}'")
        if (kind and tok[0] != kind) or (val is not None and tok[1] != val):
            raise ValueError(f"expected {val or kind} in '{self.text}', got {tok[1]}")
        self.i += 1
        return tok

    def parse(self) -> int:
        v = self.p_or()
        if self.peek()[0] is not None:
            raise ValueError(f"trailing tokens in '{self.text}'")
        return v

    def p_or(self):
        v = self.p_and()
        while self.peek() == ("sym", "||"):
            self.take()
            v = self.t.binop("or", v, self.p_and())
        return v

    def p_and(self):
        v = self.p_cmp()
        while self.peek() == ("sym", "&&"):
            self.take()
            v = self.t.binop("and", v, self.p_cmp())
        return v

    def p_cmp(self):
        v = self.p_add()
        while self.peek()[0] == "sym" and self.peek()[1] in ("<", "<=", ">", ">=", "==", "!="):
            op = self.take()[1]
            r = self.p_add()
            if op == ">":
                v = self.t.binop("lt", r, v)
            elif op == ">=":
                v = self.t.binop("le", r, v)
            elif op == "!=":
                v = self.t.unop("not", self.t.binop("eq", v, r))
            else:
                v = self.t.binop(_BINARY[op], v, r)
        return v

    def p_add(self):
        v = self.p_mul()
        while self.peek()[0] == "sym" and self.peek()[1] in ("+", "-"):
            op = self.take()[1]
            v = self.t.binop(_BINARY[op], v, self.p_mul())
        return v

    def p_mul(self):
        v = self.p_unary()
        while self.peek()[0] == "sym" and self.peek()[1] in ("*", "/", "%"):
            op = self.take()[1]
            v = self.t.binop(_BINARY[op], v, self.p_unary())
        return v

    def p_unary(self):
        tok = self.peek()
        if tok == ("sym", "-"):
            self.take()
            return self.t.unop("neg", self.p_unary())
        if tok == ("sym", "!"):
            self.take()
            return self.t.unop("not", self.p_unary())
        if tok == ("sym", "+"):
            self.take()
            return self.p_unary()
        return self.p_atom()

    def p_atom(self):
        kind, val = self.take()
        if kind == "num":
            return self.t.const(val)
        if kind == "sym" and val == "(":
            v = self.p_or()
            self.take("sym", ")")
            return v
        if kind == "id":
            if self.peek() == ("sym", "("):
                self.take()
                args = [self.p_or()]
                while self.peek() == ("sym", ","):
                    self.take()
                    args.append(self.p_or())
                self.take("sym", ")")
                return self.call(val, args)
            return self.t.param(val)
        raise ValueError(f"unexpected token {val!r} in '{self.text}'")

    def call(self, name, args):
        if name in ("min", "max") and len(args) == 2:
            return self.t.binop(name, args[0], args[1])
        if name == "abs" and len(args) == 1:
            return self.t.unop("abs", args[0])
        if name == "clamp" and len(args) == 3:
            return self.t.binop("min", self.t.binop("max", args[0], args[1]), args[2])
        raise ValueError(f"unknown function {name}/{len(args)} in '{self.text}'")

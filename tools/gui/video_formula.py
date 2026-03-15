#!/usr/bin/env python3
from __future__ import annotations

import ast
import math
import re
from typing import Dict, Set, Tuple


def float_text(v: float) -> str:
    return f"{v:.10g}"


def sanitize_frontend_expr(text: str) -> str:
    t = text.strip()
    t = t.replace("^", "**")
    t = re.sub(r"\s+", " ", t)
    return t


class ExpressionError(Exception):
    pass


class ExpressionEvaluator:
    def __init__(self) -> None:
        self._funcs = {
            "sqrt": math.sqrt,
            "abs": abs,
            "int": int,
            "floor": math.floor,
            "ceil": math.ceil,
            "round": round,
            "sin": math.sin,
            "cos": math.cos,
            "tan": math.tan,
            "asin": math.asin,
            "acos": math.acos,
            "atan": math.atan,
            "sinh": math.sinh,
            "cosh": math.cosh,
            "tanh": math.tanh,
            "asinh": math.asinh,
            "acosh": math.acosh,
            "atanh": math.atanh,
            "arsinh": math.asinh,
            "arcosh": math.acosh,
            "artanh": math.atanh,
            "log": math.log,
            "log10": math.log10,
            "exp": math.exp,
            "min": min,
            "max": max,
        }
        self._consts = {"pi": math.pi, "e": math.e}

    def constant_names(self) -> Set[str]:
        return set(self._consts.keys())

    def referenced_symbols(self, expr: str) -> Set[str]:
        src = self.sanitize(expr)
        if not src:
            return set()
        try:
            tree = ast.parse(src, mode="eval")
        except SyntaxError as exc:
            raise ExpressionError(f"Syntax error: {exc}") from exc
        names: Set[str] = set()

        def visit(node: ast.AST) -> None:
            if isinstance(node, ast.Name):
                names.add(node.id)
                return
            if isinstance(node, ast.Call):
                for arg in node.args:
                    visit(arg)
                for keyword in node.keywords:
                    visit(keyword.value)
                return
            for child in ast.iter_child_nodes(node):
                visit(child)

        visit(tree.body)
        return {name for name in names if name not in self._consts}

    def sanitize(self, expr: str) -> str:
        s = expr.strip()
        s = s.replace("^", "**")
        s = re.sub(r"\barcosh\s*\(", "acosh(", s)
        s = re.sub(r"\barsinh\s*\(", "asinh(", s)
        s = re.sub(r"\bartanh\s*\(", "atanh(", s)
        return s

    def eval(self, expr: str, env: Dict[str, float]) -> float:
        src = self.sanitize(expr)
        if not src:
            raise ExpressionError("Empty expression")
        try:
            tree = ast.parse(src, mode="eval")
        except SyntaxError as exc:
            raise ExpressionError(f"Syntax error: {exc}") from exc
        return float(self._eval_node(tree.body, env))

    def _eval_node(self, node: ast.AST, env: Dict[str, float]) -> float:
        if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
            return float(node.value)
        if isinstance(node, ast.Name):
            if node.id in env:
                return float(env[node.id])
            if node.id in self._consts:
                return float(self._consts[node.id])
            raise ExpressionError(f"Unknown symbol: {node.id}")
        if isinstance(node, ast.UnaryOp):
            v = self._eval_node(node.operand, env)
            if isinstance(node.op, ast.UAdd):
                return +v
            if isinstance(node.op, ast.USub):
                return -v
            raise ExpressionError("Unsupported unary operator")
        if isinstance(node, ast.BinOp):
            a = self._eval_node(node.left, env)
            b = self._eval_node(node.right, env)
            if isinstance(node.op, ast.Add):
                return a + b
            if isinstance(node.op, ast.Sub):
                return a - b
            if isinstance(node.op, ast.Mult):
                return a * b
            if isinstance(node.op, ast.Div):
                return a / b
            if isinstance(node.op, ast.FloorDiv):
                return a // b
            if isinstance(node.op, ast.Mod):
                return a % b
            if isinstance(node.op, ast.Pow):
                return a**b
            raise ExpressionError("Unsupported binary operator")
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
            fn_name = node.func.id
            if fn_name not in self._funcs:
                raise ExpressionError(f"Unsupported function: {fn_name}")
            fn = self._funcs[fn_name]
            args = [self._eval_node(arg, env) for arg in node.args]
            try:
                return float(fn(*args))
            except Exception as exc:
                raise ExpressionError(f"Function call failed: {fn_name}") from exc
        raise ExpressionError("Unsupported expression")


def velocity_vector_to_turns(vx: float, vy: float, vz: float) -> Tuple[float, float, float]:
    speed = math.sqrt(vx * vx + vy * vy + vz * vz)
    if speed <= 1e-12:
        return 0.0, 0.0, 0.0
    d_x = vx / speed
    d_y = vy / speed
    d_z = vz / speed
    pitch_turns = math.asin(max(-1.0, min(1.0, d_y))) / (2.0 * math.pi)
    yaw_turns = math.atan2(d_x, -d_z) / (2.0 * math.pi)
    yaw_turns = yaw_turns - math.floor(yaw_turns)
    return speed, yaw_turns, pitch_turns

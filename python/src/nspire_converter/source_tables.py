"""Compare generated C++ tables independently of comments and formatting."""

from __future__ import annotations

import re

_TOKEN = re.compile(
    r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_][A-Za-z_0-9]*|[0-9]+(?:\.[0-9]+)?|[^\s]',
    re.S,
)


def cpp_tokens(source: str) -> tuple[str, ...]:
    return tuple(
        token
        for token in _TOKEN.findall(re.sub(r"(?m)^\s*#pragma once\s*$", "", source))
        if not token.startswith(("//", "/*"))
    )

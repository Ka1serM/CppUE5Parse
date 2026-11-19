#!/usr/bin/env python3
"""resolve_has_include.py <project Source dir> <engine dir>

MSVC before VS2017 15.3 (the v140 toolset the 4.19-and-older engines use here) has no __has_include. Replace every
`__has_include("X")` / `__has_include(<X>)` in the project's (copied) sources with 1 or 0, depending on whether a
file whose path ends in X exists in the engine's Source/Plugins trees or the project's Source dir. The resulting
`#if 0` / `#if 1` blocks are then folded away, because old UnrealHeaderTools mis-handle `#else` after `#if 0`."""
import os, pathlib, re, sys

src, engine = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
by_name = {}
for root in (engine / "Engine" / "Source", engine / "Engine" / "Plugins", src):
    for dirpath, _dirs, files in os.walk(root):
        for f in files:
            if f.endswith((".h", ".inl")):
                by_name.setdefault(f.lower(), []).append((dirpath.replace("\\", "/") + "/" + f).lower())


def exists(inc):
    inc = inc.replace("\\", "/").lower()
    return any(p.endswith("/" + inc) for p in by_name.get(inc.rsplit("/", 1)[-1], ()))


directive = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$")


def const(cond):
    """0/1 for a condition that is just a literal 0 or 1 (ignoring a trailing comment), else None."""
    cond = re.sub(r"//.*$", "", cond).strip()
    return int(cond) if cond in ("0", "1") else None


def fold(lines):
    """Fold `#if 0|1 ... [#elif ...] [#else ...] #endif` blocks; other conditionals are kept as they are."""
    out, i = [], 0
    while i < len(lines):
        m = directive.match(lines[i])
        if not m or m.group(1) != "if" or const(m.group(2)) is None:
            out.append(lines[i])
            i += 1
            continue
        # split the block into branches at depth 0: [(kind, cond, body lines)]
        branches, depth, j = [("if", m.group(2), [])], 0, i + 1
        while j < len(lines):
            d = directive.match(lines[j])
            if d and d.group(1) in ("if", "ifdef", "ifndef"):
                depth += 1
            elif d and d.group(1) == "endif":
                if depth == 0:
                    break
                depth -= 1
            elif d and depth == 0 and d.group(1) in ("elif", "else"):
                branches.append((d.group(1), d.group(2), []))
                j += 1
                continue
            branches[-1][2].append(lines[j])
            j += 1
        # false constant branches vanish, a true one ends the chain, other conditions stay as #if/#elif
        kept, open_chain = [], False
        for kind, cond, body in branches:
            c = 1 if kind == "else" else const(cond)
            if c == 0:
                continue
            if c == 1:
                kept += (["#else"] + fold(body) + ["#endif"]) if open_chain else fold(body)
                open_chain = False
                break
            kept += [f"#{'elif' if open_chain else 'if'}{cond}"] + fold(body)
            open_chain = True
        if open_chain:
            kept.append("#endif")
        out.extend(kept)
        i = j + 1
    return out


pattern = re.compile(r'__has_include\s*\(\s*["<]([^">]+)[">]\s*\)')
changed = []
for f in list(src.rglob("*.h")) + list(src.rglob("*.cpp")) + list(src.rglob("*.inl")):
    s = f.read_text()
    t = pattern.sub(lambda m: "1" if exists(m.group(1)) else "0", s)
    if t != s:
        t = "\n".join(fold(t.split("\n")))
        f.write_text(t)
        changed.append(f.name)
print(f"resolve_has_include: rewrote {', '.join(changed) or 'nothing'}")

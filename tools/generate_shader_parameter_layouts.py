#!/usr/bin/env python3
import argparse
import collections
import hashlib
import os
import pathlib
import re
import subprocess
import sys

RELEASES = ["5.0", "5.1", "5.2", "5.3", "5.4", "5.5", "5.6", "5.7", "5.8"]

BUFFERS = [
    ("View", "FViewUniformShaderParameters", "5.0"),
    ("Primitive", "FPrimitiveUniformShaderParameters", "5.0"),
    ("TranslucentBasePass", "FTranslucentBasePassUniformParameters", "5.0"),
    ("MobileDirectionalLight", "FMobileDirectionalLightShaderParameters", "5.0"),
    ("Scene", "FSceneUniformParameters", "5.3"),
]

SCENE_MEMBER = ("FGPUSceneResourceParameters", "GPUScene")

KNOWN_SYMBOLS = {
    "WITH_EDITOR": 0, "WITH_EDITORONLY_DATA": 0, "UE_BUILD_SHIPPING": 1, "UE_BUILD_TEST": 0,
    "UE_BUILD_DEVELOPMENT": 0, "UE_BUILD_DEBUG": 0, "DO_CHECK": 0, "PLATFORM_WINDOWS": 1,
    "PLATFORM_DESKTOP": 1, "WITH_ENGINE": 1, "WITH_DEV_AUTOMATION_TESTS": 0, "IS_PROGRAM": 0,
    "WITH_UNREAL_DEVELOPER_TOOLS": 0, "UE_BUILD_MINIMAL": 0, "WITH_DEBUG_VIEW_MODES": 0,
    "RHI_RAYTRACING": 1, "WITH_MGPU": 1, "NV_GEFORCE_GAME_READY_API": 0,
}

class GeneratorError(Exception):
    pass


class SourceTree:

    def __init__(self, unreal, cache, version, ref):
        self.version, self.unreal, self.ref = version, unreal, ref
        self.root = pathlib.Path(cache) / version
        marker = self.root / ".extracted"
        if not marker.exists():
            self.root.mkdir(parents=True, exist_ok=True)
            archive = subprocess.Popen(["git", "-C", str(unreal), "archive", ref, "Engine/Source/Runtime", "Engine/Shaders"],
                                       stdout=subprocess.PIPE)
            subprocess.run(["tar", "-x", "-C", str(self.root), "--wildcards", "*.h", "*.ush", "*.usf"],
                           stdin=archive.stdout, check=True)
            if archive.wait() != 0:
                raise GeneratorError(f"cannot read {ref} from {unreal}")
            marker.touch()
        self._headers = None
        self._constants = {}

    def read(self, relative):
        return (self.root / relative).read_text(encoding="utf-8", errors="replace")

    def headers(self):
        if self._headers is None:
            self._headers = sorted((self.root / "Engine/Source/Runtime").rglob("*.h"))
        return self._headers

    def shared_macros(self):
        macros = Macros()
        for path in sorted((self.root / "Engine/Shaders/Shared").glob("*.h")):
            for _ in active_lines(path.read_text(encoding="utf-8", errors="replace"), macros):
                pass
        return macros

    def grep(self, pattern):
        result = subprocess.run(["grep", "-rlE", "--include=*.h", pattern, str(self.root / "Engine/Source/Runtime")],
                                capture_output=True, text=True)
        return [pathlib.Path(line) for line in result.stdout.splitlines()]


def strip_comments(text):
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            j = i + 1
            while j < n and text[j] != '"':
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            out.extend("\n" * text.count("\n", i, j + 2))
            i = n if j < 0 else j + 2
        else:
            out.append(c)
            i += 1
    return "".join(out)

def logical_lines(text):
    text = strip_comments(text).replace("\r\n", "\n")
    lines, pending = [], ""
    for line in text.split("\n"):
        if line.endswith("\\"):
            pending += line[:-1] + " "
            continue
        lines.append(pending + line)
        pending = ""
    if pending:
        lines.append(pending)
    return lines

class Macros:

    IDENT = re.compile(r"[A-Za-z_]\w*")

    def __init__(self):
        self.table = {}

    def define(self, line):
        m = re.match(r"#\s*define\s+(\w+)(\(([^)]*)\))?\s*(.*)$", line)
        if not m:
            raise GeneratorError(f"cannot parse {line!r}")
        if m.group(1) in MEMBER_MACROS or "INTERNAL_" in m.group(4) or "#" in m.group(4):
            return
        params = None if m.group(2) is None else [p.strip() for p in m.group(3).split(",") if p.strip()]
        self.table[m.group(1)] = (params, m.group(4).strip())

    def undef(self, name):
        self.table.pop(name, None)

    def expand(self, text, hide=frozenset()):
        out, i = [], 0
        while i < len(text):
            m = self.IDENT.search(text, i)
            if not m:
                out.append(text[i:])
                break
            out.append(text[i:m.start()])
            name = m.group(0)
            entry = self.table.get(name)
            if entry is None or name in hide:
                out.append(name)
                i = m.end()
                continue
            params, body = entry
            if params is None:
                out.append(self.expand(body, hide | {name}))
                i = m.end()
                continue
            j = m.end()
            while j < len(text) and text[j] == " ":
                j += 1
            if j >= len(text) or text[j] != "(":
                out.append(name)
                i = m.end()
                continue
            args, end = split_arguments(text, j)
            if len(args) != len(params):
                raise GeneratorError(f"{name} takes {len(params)} arguments, got {len(args)}: {text[j:end]!r}")
            if "#" in body:
                raise GeneratorError(f"macro {name} uses # or ##")
            expanded_args = [self.expand(arg, hide) for arg in args]
            substituted = re.sub(r"[A-Za-z_]\w*", lambda t: expanded_args[params.index(t.group(0))]
                                 if t.group(0) in params else t.group(0), body)
            out.append(self.expand(substituted, hide | {name}))
            i = end
        return "".join(out)

def split_arguments(text, open_paren):
    depth, args, start = 0, [], open_paren + 1
    for i in range(open_paren, len(text)):
        c = text[i]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
            if depth == 0:
                args.append(text[start:i].strip())
                return ([] if args == [""] else args), i + 1
        elif c == "," and depth == 1:
            args.append(text[start:i].strip())
            start = i + 1
    raise GeneratorError(f"unbalanced arguments in {text[open_paren:open_paren + 80]!r}")

class UnknownCondition(GeneratorError):
    pass

class Conditions:

    def __init__(self, macros):
        self.macros = macros

    def value(self, expression):
        expression = re.sub(r"defined\s*\(\s*(\w+)\s*\)|defined\s+(\w+)",
                            lambda m: "1" if (m.group(1) or m.group(2)) in self.macros.table
                            or (m.group(1) or m.group(2)) in KNOWN_SYMBOLS else "0", expression)
        expression = self.macros.expand(expression)

        def symbol(m):
            name = m.group(0)
            if name in KNOWN_SYMBOLS:
                return str(KNOWN_SYMBOLS[name])
            if name in ("true", "false"):
                return "1" if name == "true" else "0"
            raise UnknownCondition(f"unknown preprocessor symbol {name} in #if {expression}")
        python = re.sub(r"[A-Za-z_]\w*", symbol, expression)
        python = python.replace("&&", " and ").replace("||", " or ")
        python = re.sub(r"!(?!=)", " not ", python)
        python = re.sub(r"(\d)[uUlL]+\b", r"\1", python)
        try:
            return bool(eval(python, {"__builtins__": {}}))
        except Exception as error:
            raise GeneratorError(f"cannot evaluate #if {expression!r}: {error}")

def active_lines(text, macros):
    conditions = Conditions(macros)
    stack = []
    active, unknown = True, False

    def evaluate(expression):
        try:
            return conditions.value(expression), False
        except UnknownCondition:
            return False, True

    for line in logical_lines(text):
        stripped = line.strip()
        if stripped.startswith("#"):
            directive = re.match(r"#\s*(\w+)\s*(.*)", stripped)
            word, rest = (directive.group(1), directive.group(2)) if directive else ("", "")
            if word in ("if", "ifdef", "ifndef"):
                if not active:
                    stack.append([False, True, False, unknown])
                    continue
                if word == "if":
                    taken, frame_unknown = evaluate(rest)
                elif word == "ifdef":
                    taken, frame_unknown = rest.split()[0] in macros.table or rest.split()[0] in KNOWN_SYMBOLS, False
                else:
                    taken, frame_unknown = not (rest.split()[0] in macros.table or rest.split()[0] in KNOWN_SYMBOLS), False
                stack.append([True, taken, taken, frame_unknown])
                active, unknown = taken, frame_unknown
            elif word == "elif":
                top = stack[-1]
                if not top[0] or top[1]:
                    top[2] = False
                else:
                    top[2], top[3] = evaluate(rest)
                    top[1] = top[2]
                active, unknown = top[2], top[3]
            elif word == "else":
                top = stack[-1]
                top[2] = top[0] and not top[1] and not top[3]
                top[1] = True
                active, unknown = top[2], top[3]
            elif word == "endif":
                stack.pop()
                active = stack[-1][2] if stack else True
                unknown = stack[-1][3] if stack else False
            elif active and word == "define":
                macros.define(stripped)
            elif active and word == "undef":
                macros.undef(rest.split()[0])
            continue
        if active:
            yield line, False
        elif unknown:
            yield line, True
    if stack:
        raise GeneratorError("unterminated #if")


class Constants:

    DEFINITIONS = [
        r"#\s*define\s+{name}\s+(?P<value>[^\n]+)",
        r"(?:static\s+)?(?:const|constexpr)\s+(?:u?int(?:8|16|32|64)?|uint|size_t|SIZE_T)\s+{name}\s*=\s*(?P<value>[^;]+);",
        r"\b{name}\s*=\s*(?P<value>[^,;}}]+)[,}}\n]",
    ]

    def __init__(self, tree):
        self.tree = tree
        self.cache = {}

    def value(self, expression, local=None):
        expression = expression.strip()
        expression = re.sub(r"\(\s*(?:u?int(?:8|16|32|64)?|uint|size_t)\s*\)", "", expression)
        expression = re.sub(r"\b(?:static_cast<[^>]+>)\(([^()]*)\)", r"(\1)", expression)
        expression = re.sub(r"(\d)[uUlL]+\b", r"\1", expression)

        def substitute(m):
            name = m.group(0)
            if name == "sizeof":
                return name
            return str(self.name(name.split("::")[-1], local))
        replaced = re.sub(r"[A-Za-z_]\w*(?:::\w+)*", substitute, expression)
        replaced = replaced.replace("/", "//")
        try:
            return int(eval(replaced, {"__builtins__": {}}))
        except Exception as error:
            raise GeneratorError(f"cannot evaluate constant {expression!r}: {error}")

    def name(self, name, local=None):
        if local is not None and name in local.table:
            params, body = local.table[name]
            if params is None:
                return self.value(body, local)
        if name in self.cache:
            return self.cache[name]
        if name == "sizeof":
            raise GeneratorError("sizeof in a constant")
        found = self.find(name)
        if found is None:
            raise GeneratorError(f"unknown constant {name} (release {self.tree.version})")
        self.cache[name] = found
        return found

    def find(self, name):
        files = self.tree.grep(r"\b" + name + r"\b")
        values = set()
        for path in files:
            text = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
            for pattern in self.DEFINITIONS[:2]:
                for m in re.finditer(pattern.format(name=name), text):
                    values.add(m.group("value").strip())
            for enum in re.finditer(r"\benum\s+(?:class\s+)?\w*\s*(?::\s*\w+\s*)?\{([^}]*)\}", text):
                position = 0
                for entry in enum.group(1).split(","):
                    entry = entry.strip()
                    if not entry:
                        continue
                    entry_name, _, entry_value = entry.partition("=")
                    if entry_value.strip():
                        position = self.value(entry_value) if not re.search(r"[A-Za-z_]", entry_value) else None
                        if position is None:
                            break
                    if entry_name.strip() == name:
                        values.add(f"@{position}")
                    position += 1
        values = {value for value in values if value}
        if not values:
            return None
        resolved = set()
        for value in values:
            resolved.add(int(value[1:]) if value.startswith("@") else self.value(value))
        if len(resolved) != 1:
            raise GeneratorError(f"constant {name} has several values {sorted(resolved)} in {self.tree.version}")
        return resolved.pop()


CONSTANT_MACROS = {"SHADER_PARAMETER", "SHADER_PARAMETER_EX", "SHADER_PARAMETER_ARRAY", "SHADER_PARAMETER_ARRAY_EX",
                   "SHADER_PARAMETER_SCALAR_ARRAY"}
RESOURCE_MACROS = {
    "SHADER_PARAMETER_TEXTURE", "SHADER_PARAMETER_SAMPLER", "SHADER_PARAMETER_SRV", "SHADER_PARAMETER_UAV",
    "SHADER_PARAMETER_RDG_TEXTURE", "SHADER_PARAMETER_RDG_TEXTURE_SRV", "SHADER_PARAMETER_RDG_TEXTURE_UAV",
    "SHADER_PARAMETER_RDG_TEXTURE_NON_PIXEL_SRV", "SHADER_PARAMETER_RDG_BUFFER", "SHADER_PARAMETER_RDG_BUFFER_SRV",
    "SHADER_PARAMETER_RDG_BUFFER_UAV", "SHADER_PARAMETER_RDG_BUFFER_UPLOAD", "SHADER_PARAMETER_STRUCT_REF",
    "SHADER_PARAMETER_RDG_UNIFORM_BUFFER", "SHADER_PARAMETER_RESOURCE_COLLECTION",
}
RESOURCE_ARRAY_MACROS = {
    "SHADER_PARAMETER_TEXTURE_ARRAY", "SHADER_PARAMETER_SAMPLER_ARRAY", "SHADER_PARAMETER_SRV_ARRAY",
    "SHADER_PARAMETER_RDG_TEXTURE_ARRAY", "SHADER_PARAMETER_RDG_TEXTURE_SRV_ARRAY",
    "SHADER_PARAMETER_RDG_TEXTURE_UAV_ARRAY", "SHADER_PARAMETER_RDG_TEXTURE_NON_PIXEL_SRV_ARRAY",
    "SHADER_PARAMETER_RDG_BUFFER_ARRAY", "SHADER_PARAMETER_RDG_BUFFER_SRV_ARRAY", "SHADER_PARAMETER_RDG_BUFFER_UAV_ARRAY",
}
STRUCT_MACROS = {"SHADER_PARAMETER_STRUCT", "SHADER_PARAMETER_STRUCT_INCLUDE", "SHADER_PARAMETER_STRUCT_ARRAY"}
BEGIN = re.compile(r"\b(?:BEGIN_(?:GLOBAL_SHADER_PARAMETER|SHADER_PARAMETER|UNIFORM_BUFFER)_STRUCT(?:_WITH_CONSTRUCTOR)?)\s*\(\s*(\w+)")
MEMBER_MACROS = CONSTANT_MACROS | RESOURCE_MACROS | RESOURCE_ARRAY_MACROS | STRUCT_MACROS
END = re.compile(r"\bEND_(?:GLOBAL_SHADER_PARAMETER|SHADER_PARAMETER|UNIFORM_BUFFER)_STRUCT\s*\(")
MEMBER = re.compile(r"\b([A-Z][A-Z_0-9]*)\s*\(")

class Member:
    def __init__(self, macro, args):
        self.macro, self.args = macro, args

class Struct:

    def __init__(self, name, text, source):
        self.name, self.text, self.source = name, text, source
        self._members = None

    @property
    def members(self):
        if self.text is None:
            raise GeneratorError(f"struct {self.name} in {self.source} is declared under a condition the generator cannot evaluate")
        if self._members is None:
            self._members = members_of(self.text, self.source)
        return self._members

def members_of(text, source):
    members, i = [], 0
    while True:
        m = MEMBER.search(text, i)
        if not m:
            break
        name = m.group(1)
        args, end = split_arguments(text, m.end() - 1)
        i = end
        if name in MEMBER_MACROS:
            members.append(Member(name, args))
        else:
            raise GeneratorError(f"unknown member macro {name} in {source}")
    return members

class StructIndex:

    def __init__(self, tree):
        self.tree = tree
        self.structs = collections.defaultdict(list)
        self.shared = tree.shared_macros()
        for path in tree.grep(r"BEGIN_(GLOBAL_SHADER_PARAMETER|SHADER_PARAMETER|UNIFORM_BUFFER)_STRUCT"):
            self.scan(path)

    def scan(self, path):
        macros = Macros()
        macros.table.update(self.shared.table)
        text = path.read_text(encoding="utf-8", errors="replace")
        capturing, name, body = False, None, []
        for line, tainted in active_lines(text, macros):
            relative = str(path.relative_to(self.tree.root))
            if tainted:
                m = BEGIN.search(line)
                if m:
                    self.structs[m.group(1)].append(Struct(m.group(1), None, relative))
                continue
            if not capturing:
                m = BEGIN.search(line)
                if m:
                    capturing, name, body = True, m.group(1), []
                    line = line[m.end():]
                    close = line.find(")")
                    line = line[close + 1:] if close >= 0 else ""
                    body.append(line)
                continue
            if END.search(line):
                self.structs[name].append(Struct(name, macros.expand("\n".join(body)), relative))
                capturing = False
                continue
            body.append(line)

    def get(self, name):
        name = name.split("::")[-1]
        found = self.structs.get(name)
        if not found:
            raise GeneratorError(f"struct {name} not found in {self.tree.version}")
        found = [s for s in found if s.text is not None] or found
        if len({tuple((m.macro, tuple(m.args)) for m in s.members) for s in found}) > 1:
            raise GeneratorError(f"struct {name} is declared differently in {[s.source for s in found]}")
        return found[0]


class TypeInfo:

    def __init__(self, tree, constants):
        self.tree, self.constants, self.cache = tree, constants, {}
        text = strip_comments(tree.read("Engine/Source/Runtime/RenderCore/Public/ShaderParameterMacros.h"))
        self.alignments = {}
        for name in ("SHADER_PARAMETER_POINTER_ALIGNMENT", "SHADER_PARAMETER_STRUCT_ALIGNMENT",
                     "SHADER_PARAMETER_ARRAY_ELEMENT_ALIGNMENT"):
            values = set()
            for path in tree.grep(r"#\s*define\s+" + name + r"\s"):
                for m in re.finditer(r"#\s*define\s+" + name + r"\s+(.+)", path.read_text(encoding="utf-8", errors="replace")):
                    values.add(m.group(1).strip())
            if len(values) != 1:
                raise GeneratorError(f"{name} has {len(values)} definitions in {tree.version}")
            value = values.pop()
            self.alignments[name] = 8 if value == "sizeof(uint64)" else constants.value(value)
        self.text = text

    def scalar(self, type_name):
        type_name = {"int": "int32", "uint": "uint32"}.get(type_name, type_name)
        if type_name in self.cache:
            return self.cache[type_name]
        pattern = r"struct\s+TShaderParameterTypeInfo<\s*" + re.escape(type_name) + r"\s*>\s*\{(.*?)\n\};"
        m = re.search(pattern, self.text, re.S)
        if not m:
            for path in self.tree.grep(r"TShaderParameterTypeInfo<\s*" + re.escape(type_name) + r"\s*>"):
                m = re.search(pattern, strip_comments(path.read_text(encoding="utf-8", errors="replace")), re.S)
                if m:
                    break
        if not m:
            raise GeneratorError(f"no TShaderParameterTypeInfo for {type_name} in {self.tree.version}")
        body = m.group(1)
        rows = int(re.search(r"NumRows\s*=\s*(\d+)", body).group(1))
        columns = int(re.search(r"NumColumns\s*=\s*(\d+)", body).group(1))
        alignment = re.search(r"Alignment\s*=\s*(\w+)", body).group(1)
        alignment = int(alignment) if alignment.isdigit() else self.alignments[alignment]
        self.cache[type_name] = (alignment, rows * columns * 4)
        return self.cache[type_name]

def round_up(value, alignment):
    return (value + alignment - 1) // alignment * alignment

class Layout:
    def __init__(self, structs, types, constants, macros_for):
        self.structs, self.types, self.constants = structs, types, constants
        self.pointer = types.alignments["SHADER_PARAMETER_POINTER_ALIGNMENT"]
        self.struct_alignment = types.alignments["SHADER_PARAMETER_STRUCT_ALIGNMENT"]
        self.element_alignment = types.alignments["SHADER_PARAMETER_ARRAY_ELEMENT_ALIGNMENT"]

    def array_size(self, declaration):
        m = re.fullmatch(r"\[\s*(.+?)\s*\]", declaration.strip())
        if not m:
            raise GeneratorError(f"unexpected array declaration {declaration!r}")
        return self.constants.value(m.group(1))

    def struct(self, struct, base, prefix, constants, resources):
        offset = 0
        for member in struct.members:
            macro, args = member.macro, member.args
            if macro in CONSTANT_MACROS:
                type_name, name = args[0].strip(), args[1].strip()
                alignment, size = self.types.scalar(type_name)
                if macro == "SHADER_PARAMETER_SCALAR_ARRAY":
                    count = (self.array_size(args[2]) + 3) // 4
                    offset = round_up(offset, self.element_alignment)
                    constants.append((prefix + name, base + offset))
                    offset += count * 16
                elif macro in ("SHADER_PARAMETER_ARRAY", "SHADER_PARAMETER_ARRAY_EX"):
                    count = self.array_size(args[2])
                    offset = round_up(offset, self.element_alignment)
                    constants.append((prefix + name, base + offset))
                    offset += count * round_up(size, self.element_alignment)
                else:
                    offset = round_up(offset, alignment)
                    constants.append((prefix + name, base + offset))
                    offset += size
            elif macro in RESOURCE_MACROS | RESOURCE_ARRAY_MACROS:
                name = args[1].strip()
                count = self.array_size(args[2]) if macro in RESOURCE_ARRAY_MACROS else 1
                offset = round_up(offset, self.pointer)
                resources.append(prefix + name)
                offset += count * self.pointer
            else:
                type_name, name = args[0].strip(), args[1].strip()
                child = self.structs.get(type_name)
                count = self.array_size(args[2]) if macro == "SHADER_PARAMETER_STRUCT_ARRAY" else None
                offset = round_up(offset, self.struct_alignment)
                for element in range(count or 1):
                    child_prefix = prefix if macro == "SHADER_PARAMETER_STRUCT_INCLUDE" else (
                        f"{prefix}{name}_" if count is None else f"{prefix}{name}_{element}_")
                    offset += self.struct(child, base + offset, child_prefix, constants, resources)
        return round_up(offset, self.struct_alignment)


LOAD = re.compile(r"LoadPrimitivePrimitiveSceneDataElement\(\s*PrimitiveIndex\s*,\s*(\d+)\s*\)(?:\s*\.\s*([xyzw]+))?")

def gpu_scene_record(tree):
    text = strip_comments(tree.read("Engine/Shaders/Private/SceneData.ush"))
    start = text.find("GetPrimitiveData(uint PrimitiveId)")
    while start >= 0 and not re.match(r"GetPrimitiveData\(uint PrimitiveId\)\s*\{", text[start:]):
        start = text.find("GetPrimitiveData(uint PrimitiveId)", start + 1)
    if start < 0:
        return []
    open_brace = text.index("{", start)
    depth, i = 0, open_brace
    while True:
        depth += text[i] == "{"
        depth -= text[i] == "}"
        if depth == 0:
            break
        i += 1
    body = text[open_brace:i]
    fields = {}
    for statement in re.split(r";", body):
        loads = list(LOAD.finditer(statement))
        if not loads:
            continue
        target = re.match(r"\s*(?:UNROLL\s*)?(?:\w+\s+)*?(?:PrimitiveData\.)?(\w+)\s*(\[\s*\d+\s*\])?\s*=", statement)
        if not target:
            continue
        name = target.group(1)
        row = re.search(r"\d+", target.group(2)).group(0) if target.group(2) else None
        if "transpose(" in statement:
            elements = [int(load.group(1)) for load in loads]
            if elements != list(range(elements[0], elements[0] + len(elements))) or any(load.group(2) for load in loads):
                raise GeneratorError(f"transposed GPUScene matrix {name} is not consecutive float4s")
            fields[(name + "_Transposed", None)] = [(elements[0], 0, 4)]
            continue
        for load in loads:
            element, swizzle = int(load.group(1)), load.group(2) or "xyzw"
            components = ["xyzw".index(c) for c in swizzle]
            if components != list(range(components[0], components[0] + len(components))):
                raise GeneratorError(f"swizzle .{swizzle} of GPUScene element {element}")
            fields.setdefault((name, row), []).append((element, components[0], len(components)))
    record = []
    for (name, row), loads in fields.items():
        if len(loads) != 1 and row is None:
            raise GeneratorError(f"GPUScene field {name} is loaded {len(loads)} times")
        record.append((name, row, loads[0]))
    members = {}
    for name, row, (element, first, count) in record:
        offset = element * 16 + first * 4
        if row is None:
            members[name] = offset
            continue
        if int(row) == 0:
            members[name] = offset
        elif members.get(name) is None or members[name] + int(row) * 16 != offset:
            raise GeneratorError(f"GPUScene rows of {name} are not consecutive float4s")
    return sorted(members.items(), key=lambda item: item[1])


def cpp_string(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'

def read_release(unreal, cache, version, ref):
    tree = SourceTree(unreal, cache, version, ref)
    constants = Constants(tree)
    types = TypeInfo(tree, constants)
    index = StructIndex(tree)
    layout = Layout(index, types, constants, None)
    buffers = {}
    for shader_name, struct_name, since in BUFFERS:
        if tuple(map(int, version.split("."))) < tuple(map(int, since.split("."))):
            continue
        members, resources = [], []
        if shader_name == "Scene":
            check_scene_member_sorts_first(tree)
            struct = Struct(struct_name, f"SHADER_PARAMETER_STRUCT({SCENE_MEMBER[0]}, {SCENE_MEMBER[1]})", "synthesized")
        else:
            struct = index.get(struct_name)
        layout.struct(struct, 0, "", members, resources)
        buffers[shader_name] = (tuple(members), tuple(resources))
    return buffers, tuple(gpu_scene_record(tree))

def check_scene_member_sorts_first(tree):
    found = subprocess.run(["git", "-C", str(tree.unreal), "grep", "-h", "IMPLEMENT_SCENE_UB_STRUCT", tree.ref, "--", "Engine"],
                           capture_output=True, text=True, check=True).stdout
    names = set(re.findall(r"^IMPLEMENT_SCENE_UB_STRUCT(?:_EX)?\(\s*[\w:]+\s*,\s*(\w+)", found, re.M))
    if SCENE_MEMBER[1] not in names or min(names, key=str.lower) != SCENE_MEMBER[1]:
        raise GeneratorError(f"Scene members {sorted(names)} in {tree.version}: {SCENE_MEMBER[1]} no longer sorts first")

def emit(releases):
    shared, order = {}, []

    def share(kind, value):
        key = (kind, value)
        if key not in shared:
            shared[key] = len(order)
            order.append(key)
        return shared[key]

    table = []
    for version, (buffers, record) in releases.items():
        entries = [(name, share("buffer", layout)) for name, layout in buffers.items()]
        table.append((version, entries, share("record", record)))

    out = ["//! Generated by tools/generate_shader_parameter_layouts.py from the Unreal source. Do not edit.", ""]
    for number, (kind, value) in enumerate(order):
        if kind == "buffer":
            members, resources = value
            out.append(f"constexpr ShaderParameterLayoutMember Members{number}[] = {{")
            out.extend(f"    {{{cpp_string(name)}, {offset}}}," for name, offset in members)
            out.append("};")
            if resources:
                out.append(f"constexpr std::string_view Resources{number}[] = {{")
                out.extend(f"    {cpp_string(name)}," for name in resources)
                out.append("};")
            resource_span = f"Resources{number}" if resources else "{}"
            out.append(f"constexpr ShaderParameterBufferLayout Buffer{number} = {{Members{number}, {resource_span}}};")
        else:
            out.append(f"constexpr ShaderParameterLayoutMember Record{number}[] = {{")
            out.extend(f"    {{{cpp_string(name)}, {offset}}}," for name, offset in value)
            out.append("};")
        out.append("")
    for version, entries, record in table:
        major, minor = version.split(".")
        out.append(f"constexpr EngineUniformBuffer Buffers{major}{minor}[] = {{")
        out.extend(f"    {{{cpp_string(name)}, Buffer{index}}}," for name, index in entries)
        out.append("};")
    out.append("")
    out.append("constexpr EngineShaderParameterLayouts Releases[] = {")
    for version, entries, record in table:
        major, minor = version.split(".")
        out.append(f"    {{EGame::GAME_UE{major}_{minor}, Buffers{major}{minor}, Record{record}}},")
    out.append("};")
    return "\n".join(out) + "\n"

def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--unreal", required=True, type=pathlib.Path, help="Unreal Engine git clone")
    parser.add_argument("--output", type=pathlib.Path, help="EngineShaderParameterLayouts.inc to write")
    parser.add_argument("--cache", type=pathlib.Path,
                        default=pathlib.Path(os.environ.get("XDG_CACHE_HOME", pathlib.Path.home() / ".cache")) / "ueshaderdecoder-generator")
    parser.add_argument("--branch", action="append", default=[], metavar="RELEASE=REF",
                        help="read a release from another git ref than origin/<release>")
    parser.add_argument("--releases", nargs="*", default=RELEASES)
    args = parser.parse_args()
    refs = {release: f"origin/{release}" for release in RELEASES}
    refs.update(dict(item.split("=", 1) for item in args.branch))

    releases = {}
    for release in args.releases:
        try:
            releases[release] = read_release(args.unreal, args.cache, release, refs[release])
        except GeneratorError as error:
            sys.exit(f"{release}: {error}")
        print(f"{release}: " + ", ".join(f"{name} {len(layout[0])} members {len(layout[1])} resources"
                                         for name, layout in releases[release][0].items()) +
              f", record {len(releases[release][1])} fields", file=sys.stderr)

    if args.output:
        args.output.write_text(emit(releases))

if __name__ == "__main__":
    main()

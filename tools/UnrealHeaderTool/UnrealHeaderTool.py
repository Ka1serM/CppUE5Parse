#!/usr/bin/env python3
"""UnrealHeaderTool for CppUE5Parse: reads the UCLASS/USTRUCT/UENUM/UPROPERTY/UFUNCTION markers of a module's headers
and writes, for each header Foo.h, Foo.generated.h (the GENERATED_BODY expansions) and Foo.gen.cpp (StaticClass,
the generated constructors, exec thunks and the compiled-in registration), as UE's UHT does.

The headers are parsed by libclang, with the markers defined so that they stay in the syntax tree: UPROPERTY and
UFUNCTION become annotations on their member, UCLASS/USTRUCT/UENUM and GENERATED_BODY become static_asserts in front
of (or inside) their type. What the cooked data reflects (property types, flags, the properties C++ does not declare)
comes from the game's mappings at run time; the generated code tells the runtime what only C++ knows: sizes, member
offsets (the target compiler computes them: offsetof), constructors, natives.

    UnrealHeaderTool.py --module Engine --package /Script/Engine --include-root include --output build/Generated/Engine
                        @compile-flags.rsp include/ue5parse/runtime/Engine/Animation/Skeleton.h ...

The response file holds the target's -I and -D flags. libclang is found on the system (--libclang to choose one).
"""

import argparse
import glob
import os
import re
import sys
from dataclasses import dataclass, field

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "external", "clang-python"))
import clang.cindex as ci  # noqa: E402

RUNTIME = "::ue5parse::runtime::"

# Cast flags UE gives fixed classes (EClassCastFlags); every other class has 0.
CAST_FLAG_CLASSES = {
    "ULevel", "AActor", "APlayerController", "APawn", "USceneComponent", "UPrimitiveComponent", "USkinnedMeshComponent",
    "USkeletalMeshComponent", "UBlueprint", "UStaticMeshComponent",
}

# The markers as the parser sees them.
MARKER_DEFINES = [
    'UCLASS(...)=static_assert(true, "UCLASS:" #__VA_ARGS__);',
    'USTRUCT(...)=static_assert(true, "USTRUCT:" #__VA_ARGS__);',
    'UENUM(...)=static_assert(true, "UENUM:" #__VA_ARGS__);',
    'UINTERFACE(...)=static_assert(true, "UINTERFACE:" #__VA_ARGS__);',
    'UDELEGATE(...)=static_assert(true, "UDELEGATE:" #__VA_ARGS__);',
    'GENERATED_BODY(...)=static_assert(true, "GENERATED_BODY");',
    'GENERATED_USTRUCT_BODY(...)=static_assert(true, "GENERATED_BODY");',
    'UPROPERTY(...)=__attribute__((annotate("UPROPERTY:" #__VA_ARGS__)))',
    'UFUNCTION(...)=__attribute__((annotate("UFUNCTION:" #__VA_ARGS__)))',
    'UPARAM(...)=',
    'UMETA(...)=',
]
TYPE_MARKERS = ("UCLASS", "USTRUCT", "UENUM", "UINTERFACE", "UDELEGATE")


class UhtError(Exception):
    pass


# ---------------------------------------------------------------------------------------------------------------------
# The parsed types

@dataclass
class Specifiers:
    """The arguments of a marker macro: UPROPERTY(EditAnywhere, Category = "A", meta = (DisplayName = "B"))."""
    flags: set = field(default_factory=set)
    values: dict = field(default_factory=dict)
    meta: dict = field(default_factory=dict)

    def has(self, name):
        return name.lower() in self.flags


@dataclass
class PropertyDecl:
    name: str
    cpp_type: str
    specifiers: Specifiers
    line: int


@dataclass
class ParamDecl:
    name: str
    cpp_type: str       # as written: "const FVector &"
    base_type: str      # without const and reference: "FVector"
    is_const: bool
    is_ref: bool
    has_default: bool
    category: str       # how the thunk reads it (see classify_type)
    inner: str = ""     # the element type of a TArray, the class of an object pointer
    script_type: str = ""   # the runtime's parameter type string (FNativeParam::Type)


@dataclass
class FunctionDecl:
    name: str
    return_type: str
    params: list
    is_static: bool
    is_const: bool
    is_virtual: bool
    access: str
    specifiers: Specifiers
    line: int
    return_script_type: str = ""

    @property
    def is_event(self):
        return self.specifiers.has("BlueprintImplementableEvent") or self.specifiers.has("BlueprintNativeEvent")

    @property
    def is_rpc(self):
        s = self.specifiers
        return s.has("Server") or s.has("Client") or s.has("NetMulticast")

    @property
    def calls_through_process_event(self):
        """Events and RPCs: the C++ function UnrealHeaderTool writes calls ProcessEvent, so that a Blueprint override
        (or the remote side) runs; the native body is Foo_Implementation."""
        return self.is_event or self.is_rpc

    @property
    def has_native_body(self):
        return not self.specifiers.has("BlueprintImplementableEvent")


@dataclass
class TypeDecl:
    kind: str                   # "class", "struct", "enum"
    name: str
    namespace: str
    specifiers: Specifiers
    line: int
    super_name: str = ""
    body_line: int = 0          # the line of GENERATED_BODY
    properties: list = field(default_factory=list)
    functions: list = field(default_factory=list)
    has_default_constructor: bool = False
    has_object_initializer_constructor: bool = False
    has_destructor: bool = False
    enum_underlying: str = ""
    is_enum_class: bool = False

    @property
    def has_constructor(self):
        return self.has_default_constructor or self.has_object_initializer_constructor

    @property
    def script_name(self):
        """The name of the type in the engine: the C++ name without its prefix (USkeleton -> Skeleton)."""
        name = self.name.split("::")[-1]
        if self.kind == "enum":
            return name
        return name[1:] if len(name) > 1 and name[0] in "UAFI" and name[1].isupper() else name


def parse_specifiers(text):
    """Specifiers from the stringized marker arguments: flags, key = value, meta = (key = value, ...)."""
    def split_top(text):
        parts, depth, quoted, current = [], 0, False, ""
        for i, ch in enumerate(text):
            if ch == '"' and (i == 0 or text[i - 1] != "\\"):
                quoted = not quoted
            elif not quoted and ch == "(":
                depth += 1
            elif not quoted and ch == ")":
                depth -= 1
            if ch == "," and depth == 0 and not quoted:
                parts.append(current.strip())
                current = ""
            else:
                current += ch
        if current.strip():
            parts.append(current.strip())
        return parts

    specifiers = Specifiers()
    for part in split_top(text):
        key, sep, value = part.partition("=")
        key, value = key.strip(), value.strip()
        if not sep:
            specifiers.flags.add(key.lower())
        elif key.lower() == "meta" and value.startswith("("):
            for item in split_top(value[1:-1]):
                meta_key, _, meta_value = item.partition("=")
                specifiers.meta[meta_key.strip()] = meta_value.strip().strip('"')
        else:
            specifiers.values[key.lower()] = value.strip('"')
    return specifiers


# ---------------------------------------------------------------------------------------------------------------------
# libclang

def find_libclang(explicit):
    if explicit:
        return explicit
    if os.environ.get("UE5PARSE_LIBCLANG"):
        return os.environ["UE5PARSE_LIBCLANG"]
    patterns = [
        "/usr/lib64/libclang.so*", "/usr/lib/libclang.so*", "/usr/lib/*-linux-gnu/libclang*.so*",
        "/usr/lib/llvm-*/lib/libclang.so*", "/usr/lib64/llvm*/lib*/libclang.so*",
        "/opt/homebrew/opt/llvm/lib/libclang.dylib", "/usr/local/opt/llvm/lib/libclang.dylib",
        "/Library/Developer/CommandLineTools/usr/lib/libclang.dylib",
        "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/libclang.dylib",
        "C:/Program Files/LLVM/bin/libclang.dll",
    ]
    found = [path for pattern in patterns for path in glob.glob(pattern) if os.path.isfile(path)]
    if not found:
        raise UhtError("libclang not found: install it (Fedora clang-libs, Debian/Ubuntu libclang1, LLVM on Windows) "
                       "or pass its path in UE5PARSE_LIBCLANG")

    def version(path):
        return [int(part) for part in re.findall(r"\d+", os.path.basename(path))] or [0]
    return max(found, key=version)


def clang_version():
    function = ci.conf.lib.clang_getClangVersion
    function.restype = ci._CXString
    return ci._CXString.from_result(function())


def find_resource_dir(libclang):
    """clang's own headers (stddef.h, ...) for the libclang in use."""
    match = re.search(r"version (\d+)", clang_version())
    major = match.group(1) if match else "*"
    libdir = os.path.dirname(os.path.realpath(libclang))
    candidates = []
    for base in (libdir, os.path.join(libdir, ".."), "/usr", "/usr/local"):
        for pattern in (f"clang/{major}*", f"lib*/clang/{major}*"):
            candidates += glob.glob(os.path.join(base, pattern))
    for candidate in sorted(set(candidates), reverse=True):
        if os.path.isfile(os.path.join(candidate, "include", "stddef.h")):
            return os.path.normpath(candidate)
    raise UhtError(f"clang's resource directory (include/stddef.h) for libclang {major} not found")


def read_response_files(arguments):
    out = []
    for argument in arguments:
        if argument.startswith("@"):
            with open(argument[1:], encoding="utf-8") as f:
                out += [line.strip() for line in f if line.strip()]
        else:
            out.append(argument)
    return out


# ---------------------------------------------------------------------------------------------------------------------
# The reader: the module's headers in one translation unit, walked for the markers.

def location_text(cursor):
    return f"{cursor.location.file.name}:{cursor.location.line}" if cursor.location.file else "?"


def annotation(cursor):
    for child in cursor.get_children():
        if child.kind == ci.CursorKind.ANNOTATE_ATTR:
            return child.spelling
    return None


def marker_text(cursor):
    """The message of a marker static_assert ("UCLASS:Abstract"), or None for any other static_assert."""
    for child in cursor.get_children():
        if child.kind == ci.CursorKind.STRING_LITERAL:
            text = child.spelling
            if text.startswith('"') and text.endswith('"'):
                text = text[1:-1].encode().decode("unicode_escape")
            if text == "GENERATED_BODY" or text.split(":", 1)[0] in TYPE_MARKERS:
                return text
    return None


def strip_const(spelling):
    return re.sub(r"^const\s+|\s+const$", "", spelling.strip())


def template_name(clang_type):
    declaration = clang_type.get_canonical().get_declaration()
    return declaration.spelling if declaration.kind != ci.CursorKind.NO_DECL_FOUND else ""


SIGNED_KINDS = {ci.TypeKind.SCHAR, ci.TypeKind.CHAR_S, ci.TypeKind.SHORT, ci.TypeKind.INT, ci.TypeKind.LONG,
                ci.TypeKind.LONGLONG}
UNSIGNED_KINDS = {ci.TypeKind.UCHAR, ci.TypeKind.CHAR_U, ci.TypeKind.USHORT, ci.TypeKind.UINT, ci.TypeKind.ULONG,
                  ci.TypeKind.ULONGLONG}
INTEGER_PROPERTIES = {
    (True, 1): "FInt8Property", (True, 2): "FInt16Property", (True, 4): "FIntProperty", (True, 8): "FInt64Property",
    (False, 1): "FByteProperty", (False, 2): "FUInt16Property", (False, 4): "FUInt32Property",
    (False, 8): "FUInt64Property",
}
NAMED_PROPERTIES = {"FName": "FNameProperty", "FString": "FStrProperty", "FText": "FTextProperty"}


def classify_type(clang_type, spelling):
    """How a thunk reads a value of this type: (category, inner). Categories: bool, property:<FProperty class>, enum,
    byteenum, array, object, class, softobject, weakobject, interface, struct, unknown."""
    canonical = clang_type.get_canonical()
    kind = canonical.kind
    if kind == ci.TypeKind.BOOL:
        return "bool", ""
    if kind in SIGNED_KINDS or kind in UNSIGNED_KINDS:
        return "property:" + INTEGER_PROPERTIES[(kind in SIGNED_KINDS, canonical.get_size())], ""
    if kind == ci.TypeKind.FLOAT:
        return "property:FFloatProperty", ""
    if kind == ci.TypeKind.DOUBLE:
        return "property:FDoubleProperty", ""
    if kind == ci.TypeKind.ENUM:
        return "enum", ""
    if kind == ci.TypeKind.POINTER:
        pointee = canonical.get_pointee()
        if pointee.kind == ci.TypeKind.RECORD:
            return "object", strip_const(re.sub(r"\s*\*$", "", spelling))
        return "unknown", ""
    if kind == ci.TypeKind.RECORD:
        name = template_name(canonical)
        inner = spelling[spelling.find("<") + 1:spelling.rfind(">")].strip() if "<" in spelling else ""
        if name in NAMED_PROPERTIES:
            return "property:" + NAMED_PROPERTIES[name], ""
        return {
            "TArray": ("array", inner), "TEnumAsByte": ("byteenum", inner), "TSubclassOf": ("class", inner),
            "TObjectPtr": ("object", inner), "FSoftObjectPtr": ("softobject", ""), "FSoftObjectPath": ("softobject", ""),
            "FWeakObjectPtr": ("weakobject", ""), "FScriptInterface": ("interface", ""),
            "TMap": ("unknown", ""), "TSet": ("unknown", ""),
        }.get(name, ("struct", ""))
    return "unknown", ""


# The script names of the core math types when clang reports them by their template (the sugar of the alias is lost).
CANONICAL_STRUCT_NAMES = {
    "TVector<double>": "Vector", "TVector<float>": "Vector3f", "TVector2<double>": "Vector2D", "TVector2<float>": "Vector2f",
    "TVector4<double>": "Vector4", "TVector4<float>": "Vector4f", "TRotator<double>": "Rotator", "TRotator<float>": "Rotator3f",
    "TQuat<double>": "Quat", "TQuat<float>": "Quat4f", "TTransform<double>": "Transform", "TTransform<float>": "Transform3f",
    "TMatrix<double>": "Matrix", "TMatrix<float>": "Matrix44f", "TBox<double>": "Box", "TBox<float>": "Box3f",
    "TBox2<double>": "Box2D", "TBox2<float>": "Box2f", "TPlane<double>": "Plane", "TPlane<float>": "Plane4f",
    "TSphere<double>": "Sphere", "TSphere<float>": "Sphere3f", "TBoxSphereBounds<double>": "BoxSphereBounds",
    "TIntPoint<int>": "IntPoint", "TIntVector3<int>": "IntVector", "TIntVector2<int>": "IntVector2",
    "TIntVector4<int>": "IntVector4",
}


def strip_prefix(name, prefixes):
    """UE's type name without its C++ prefix: AActor -> Actor, FVector -> Vector, IFoo -> Foo."""
    if len(name) > 1 and name[0] in prefixes and name[1].isupper():
        return name[1:]
    return name


def plain_spelling(clang_type):
    spelling = strip_const(clang_type.spelling)
    spelling = re.sub(r"^(struct|class|enum)\s+", "", spelling)
    return re.sub(r"^(::)?(\w+::)*", "", spelling)


def template_arguments(clang_type):
    count = clang_type.get_num_template_arguments()
    if count < 0:
        clang_type = clang_type.get_canonical()
        count = clang_type.get_num_template_arguments()
    return [clang_type.get_template_argument_type(i) for i in range(max(count, 0))]


def record_script_name(clang_type, prefixes="UAFI"):
    spelling = plain_spelling(clang_type)
    if "<" in spelling or not spelling:
        canonical = re.sub(r"^(::)?(\w+::)*", "", strip_const(clang_type.get_canonical().spelling))
        canonical = re.sub(r"\b(\w+::)+", "", canonical)
        if canonical in CANONICAL_STRUCT_NAMES:
            return CANONICAL_STRUCT_NAMES[canonical]
        spelling = canonical
    if spelling.startswith("U") or spelling.startswith("A") or spelling.startswith("F") or spelling.startswith("I"):
        return strip_prefix(spelling, prefixes)
    return spelling


def integer_script_type(canonical):
    signed = canonical.kind in SIGNED_KINDS
    return ("int" if signed else "uint") + str(canonical.get_size() * 8)


def enum_script_name(declaration):
    # A namespaced enum (namespace EFoo { enum Type }) is named by its namespace, as UE does.
    if declaration.spelling == "Type" and declaration.semantic_parent is not None:
        return declaration.semantic_parent.spelling
    return declaration.spelling


def script_type(clang_type):
    """The runtime's type string of a parameter type (see FNativeParam in UObjectGlobals.h); None when the runtime has
    no property for it."""
    canonical = clang_type.get_canonical()
    kind = canonical.kind
    if kind in (ci.TypeKind.LVALUEREFERENCE, ci.TypeKind.RVALUEREFERENCE):
        return script_type(clang_type.get_pointee())
    if kind == ci.TypeKind.BOOL:
        return "bool"
    if kind in SIGNED_KINDS or kind in UNSIGNED_KINDS:
        return integer_script_type(canonical)
    if kind == ci.TypeKind.FLOAT:
        return "float"
    if kind == ci.TypeKind.DOUBLE:
        return "double"
    if kind == ci.TypeKind.ENUM:
        declaration = canonical.get_declaration()
        underlying = integer_script_type(declaration.enum_type.get_canonical())
        if declaration.is_scoped_enum():
            return f"Enum:{enum_script_name(declaration)}:{underlying}"
        return f"Byte:{enum_script_name(declaration)}"
    if kind == ci.TypeKind.POINTER:
        pointee = clang_type.get_pointee()
        if pointee.get_canonical().kind != ci.TypeKind.RECORD:
            return None
        name = record_script_name(pointee)
        return "Class:Object" if name == "Class" else f"Object:{name}"
    if kind != ci.TypeKind.RECORD:
        return None
    name = template_name(canonical)
    arguments = template_arguments(clang_type)
    if name in ("TArray", "TSet") and arguments:
        inner = script_type(arguments[0])
        return f"{name[1:]}<{inner}>" if inner else None
    if name == "TMap" and len(arguments) >= 2:
        key, value = script_type(arguments[0]), script_type(arguments[1])
        return f"Map<{key},{value}>" if key and value else None
    if name == "TEnumAsByte" and arguments:
        return f"Byte:{enum_script_name(arguments[0].get_canonical().get_declaration())}"
    if name == "TSubclassOf" and arguments:
        return f"Class:{record_script_name(arguments[0])}"
    if name == "TSoftObjectPtr" and arguments:
        return f"SoftObject:{record_script_name(arguments[0])}"
    if name == "TSoftClassPtr" and arguments:
        return f"SoftClass:{record_script_name(arguments[0])}"
    if name == "TWeakObjectPtr" and arguments:
        return f"WeakObject:{record_script_name(arguments[0])}"
    if name == "TScriptInterface" and arguments:
        return f"Interface:{record_script_name(arguments[0])}"
    if name == "TObjectPtr" and arguments:
        return f"Object:{record_script_name(arguments[0])}"
    simple = {"FName": "Name", "FString": "Str", "FText": "Text", "FFieldPath": "FieldPath", "FSoftObjectPtr": "SoftObject:Object",
              "FWeakObjectPtr": "WeakObject:Object", "FScriptInterface": "Interface:Interface"}
    if name in simple:
        return simple[name]
    # A dynamic delegate parameter (FTimerDynamicDelegate and the like are aliases of these).
    if name == "FScriptDelegate":
        return "Delegate"
    if name == "FMulticastScriptDelegate":
        return "MulticastDelegate"
    return f"Struct:{record_script_name(clang_type, 'F')}"


class ModuleReader:
    def __init__(self, headers, compile_flags, libclang, stub_dir):
        self.headers = {os.path.realpath(header): header for header in headers}
        self.compile_flags = compile_flags
        self.libclang = libclang
        self.stub_dir = stub_dir
        self.types = {header: [] for header in headers}

    def parse(self):
        ci.Config.set_library_file(self.libclang)
        resource_dir = find_resource_dir(self.libclang)
        # The .generated.h files are stubs while parsing: the ones of this module may be stale, and the ones of other
        # modules may not exist yet. The stub directory comes first in the include path.
        os.makedirs(self.stub_dir, exist_ok=True)
        for header in self.headers.values():
            self.write_stub(os.path.splitext(os.path.basename(header))[0] + ".generated.h")
        main = os.path.join(self.stub_dir, "UnrealHeaderToolModule.cpp")
        source = "".join(f'#include "{os.path.abspath(header)}"\n' for header in self.headers.values())
        arguments = ["-x", "c++", "-std=c++23", f"-resource-dir={resource_dir}", f"-I{self.stub_dir}"]
        arguments += [f"-D{define}" for define in MARKER_DEFINES] + self.compile_flags
        index = ci.Index.create()
        for _ in range(64):
            tu = index.parse(main, args=arguments, unsaved_files=[(main, source)],
                             options=ci.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
            missing = [m.group(1) for d in tu.diagnostics
                       if (m := re.search(r"'([\w.]+\.generated\.h)' file not found", d.spelling))]
            if not missing:
                break
            for name in missing:
                self.write_stub(name)
        errors = [d for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
        if errors:
            raise UhtError("the headers do not compile:\n" + "\n".join(f"  {d.format()}" for d in errors[:20]))
        self.check_generated_includes(tu)
        self.walk_scope(tu.cursor, [])
        return self.types

    def write_stub(self, name):
        path = os.path.join(self.stub_dir, name)
        if not os.path.exists(path):
            with open(path, "w", encoding="utf-8") as f:
                f.write("// Stub of UnrealHeaderTool's parse.\n")

    def check_generated_includes(self, tu):
        included = {}
        for inclusion in tu.get_includes():
            source = os.path.realpath(inclusion.source.name)
            if source in self.headers:
                included.setdefault(source, set()).add(os.path.basename(inclusion.include.name))
        for real, header in self.headers.items():
            name = os.path.splitext(os.path.basename(header))[0] + ".generated.h"
            if name not in included.get(real, set()):
                raise UhtError(f"{header}: missing #include \"{name}\" (the last include of the header)")

    def header_of(self, cursor):
        if cursor.location.file is None:
            return None
        return self.headers.get(os.path.realpath(cursor.location.file.name))

    def walk_scope(self, scope, namespace):
        pending = None
        for cursor in scope.get_children():
            header = self.header_of(cursor)
            if cursor.kind == ci.CursorKind.NAMESPACE:
                self.walk_scope(cursor, namespace + [cursor.spelling])
                continue
            if cursor.kind == ci.CursorKind.LINKAGE_SPEC:
                self.walk_scope(cursor, namespace)
                continue
            if header is None:
                continue
            if cursor.kind == ci.CursorKind.STATIC_ASSERT and (text := marker_text(cursor)):
                if text == "GENERATED_BODY":
                    raise UhtError(f"{location_text(cursor)}: GENERATED_BODY outside a UCLASS or USTRUCT")
                if pending is not None:
                    raise UhtError(f"{location_text(pending[0])}: {pending[1].split(':')[0]} is not followed by its type")
                pending = (cursor, text)
                continue
            if pending is None:
                if cursor.kind in (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL) and cursor.is_definition():
                    self.check_unmarked(cursor)
                continue
            marker_cursor, text = pending
            marker, _, arguments = text.partition(":")
            if marker in ("UINTERFACE", "UDELEGATE"):
                raise UhtError(f"{location_text(marker_cursor)}: {marker} is not supported yet")
            expected = {"UCLASS": (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL),
                        "USTRUCT": (ci.CursorKind.STRUCT_DECL, ci.CursorKind.CLASS_DECL),
                        "UENUM": (ci.CursorKind.ENUM_DECL,)}[marker]
            if cursor.kind not in expected or not cursor.is_definition():
                raise UhtError(f"{location_text(marker_cursor)}: {marker} is not followed by its type")
            pending = None
            specifiers = parse_specifiers(arguments)
            if marker == "UENUM":
                self.types[header].append(self.read_enum(cursor, specifiers, namespace))
            else:
                self.types[header].append(self.read_record(cursor, marker, specifiers, namespace))
        if pending is not None:
            raise UhtError(f"{location_text(pending[0])}: {pending[1].split(':')[0]} is not followed by its type")

    def check_unmarked(self, cursor):
        for child in cursor.get_children():
            if annotation(child) or (child.kind == ci.CursorKind.STATIC_ASSERT and marker_text(child)):
                raise UhtError(f"{location_text(child)}: reflection markers in {cursor.spelling}, which has no "
                               "UCLASS or USTRUCT")

    def read_record(self, cursor, marker, specifiers, namespace):
        kind = "class" if marker == "UCLASS" else "struct"
        decl = TypeDecl(kind, cursor.spelling, "::".join(namespace), specifiers, cursor.location.line)
        bases = [child for child in cursor.get_children() if child.kind == ci.CursorKind.CXX_BASE_SPECIFIER]
        if bases:
            decl.super_name = re.sub(r"^(public|protected|private|virtual)\s+", "", bases[0].type.spelling)
        if kind == "class" and not bases:
            raise UhtError(f"{location_text(cursor)}: UCLASS {decl.name} has no base class")
        for child in cursor.get_children():
            text = annotation(child)
            if child.kind == ci.CursorKind.STATIC_ASSERT and marker_text(child) == "GENERATED_BODY":
                decl.body_line = child.location.line
            elif child.kind == ci.CursorKind.FIELD_DECL and text and text.startswith("UPROPERTY:"):
                if child.is_bitfield():
                    raise UhtError(f"{location_text(child)}: a bitfield UPROPERTY is not supported: use bool")
                decl.properties.append(PropertyDecl(child.spelling, child.type.spelling,
                                                    parse_specifiers(text.partition(":")[2]), child.location.line))
            elif child.kind == ci.CursorKind.CXX_METHOD and text and text.startswith("UFUNCTION:"):
                if kind == "struct":
                    raise UhtError(f"{location_text(child)}: UFUNCTION in a USTRUCT")
                decl.functions.append(self.read_function(child, parse_specifiers(text.partition(":")[2])))
            elif text:
                raise UhtError(f"{location_text(child)}: {text.split(':')[0]} on a member it does not apply to")
            elif child.kind == ci.CursorKind.CONSTRUCTOR:
                params = list(child.get_arguments())
                if not params:
                    decl.has_default_constructor = True
                elif "FObjectInitializer" in params[0].type.spelling:
                    decl.has_object_initializer_constructor = True
            elif child.kind == ci.CursorKind.DESTRUCTOR:
                decl.has_destructor = True
        if decl.body_line == 0:
            raise UhtError(f"{location_text(cursor)}: {decl.name} has no GENERATED_BODY()")
        return decl

    def read_function(self, cursor, specifiers):
        params = []
        for param in cursor.get_arguments():
            clang_type = param.type
            is_ref = clang_type.kind == ci.TypeKind.LVALUEREFERENCE
            value_type = clang_type.get_pointee() if is_ref else clang_type
            base = strip_const(value_type.spelling)
            category, inner = classify_type(value_type, base)
            has_default = any(token.spelling == "=" for token in param.get_tokens())
            params.append(ParamDecl(param.spelling, clang_type.spelling, base, value_type.is_const_qualified(), is_ref,
                                    has_default, category, inner, script_type(value_type) or ""))
            if not params[-1].script_type and not specifiers.has("CustomThunk"):
                raise UhtError(f"{location_text(param)}: parameter '{clang_type.spelling} {param.spelling}' has a type "
                               "the runtime has no property for")
        access = cursor.access_specifier.name.lower()
        result = cursor.result_type
        return_script = "" if result.get_canonical().kind == ci.TypeKind.VOID else script_type(result)
        if return_script is None:
            raise UhtError(f"{location_text(cursor)}: return type '{result.spelling}' has no property type")
        return FunctionDecl(cursor.spelling, result.spelling, params, cursor.is_static_method(),
                            cursor.is_const_method(), cursor.is_virtual_method(), access, specifiers,
                            cursor.location.line, return_script)

    def read_enum(self, cursor, specifiers, namespace):
        tokens = [token.spelling for token in cursor.get_tokens()]
        brace = tokens.index("{") if "{" in tokens else len(tokens)
        explicit_underlying = ":" in tokens[:brace]
        return TypeDecl("enum", cursor.spelling, "::".join(namespace), specifiers, cursor.location.line,
                        enum_underlying=cursor.enum_type.spelling if explicit_underlying else "",
                        is_enum_class=cursor.is_scoped_enum())


# ---------------------------------------------------------------------------------------------------------------------
# Exec thunks (UE's UhtProperty.AppendFunctionThunkParameterGet)

def thunk_param(param, function, decl):
    """The P_GET_ line of a parameter and the argument expression of the call (UE's UhtProperty thunk rules). Every
    reference parameter reads by reference: UE marks them out parameters, and ProcessEvent passes their address."""
    by_ref = param.is_ref
    var = f"Z_Param_Out_{param.name}" if by_ref else f"Z_Param_{param.name}"
    if param.category == "bool":
        return f"P_GET_UBOOL{'_REF' if by_ref else ''}({var});", var
    if by_ref:
        return f"PARAM_PASSED_BY_REF({var}, {param.base_type})", var
    return f"PARAM_PASSED_BY_VAL({var}, {param.base_type})", var


def param_flags(param):
    """UE's property flags of a parameter: CPF_Parm; a reference is an out parameter (a const one also a const
    reference parameter, as UHT marks them)."""
    flags = ["CPF_Parm"]
    if param.is_ref:
        flags.append("CPF_OutParm")
        if param.is_const:
            flags += ["CPF_ConstParm", "CPF_ReferenceParm"]
    return " | ".join(f"{RUNTIME}{flag}" for flag in flags)


def function_flags(function):
    s = function.specifiers
    flags = []
    if function.has_native_body:
        flags.append("FUNC_Native")
    if function.is_event:
        flags += ["FUNC_Event", "FUNC_BlueprintEvent"]
    if function.is_rpc:
        flags += ["FUNC_Net", "FUNC_Event"]
        if s.has("Reliable"):
            flags.append("FUNC_NetReliable")
        if s.has("Server"):
            flags.append("FUNC_NetServer")
        if s.has("Client"):
            flags.append("FUNC_NetClient")
        if s.has("NetMulticast"):
            flags.append("FUNC_NetMulticast")
    if s.has("BlueprintAuthorityOnly"):
        flags.append("FUNC_BlueprintAuthorityOnly")
    if s.has("BlueprintCosmetic"):
        flags.append("FUNC_BlueprintCosmetic")
    if function.is_static:
        flags.append("FUNC_Static")
    if not function.is_virtual and not function.calls_through_process_event:
        flags.append("FUNC_Final")
    flags.append({"public": "FUNC_Public", "protected": "FUNC_Protected", "private": "FUNC_Private"}[function.access])
    if s.has("BlueprintCallable") or s.has("BlueprintPure"):
        flags.append("FUNC_BlueprintCallable")
    if s.has("BlueprintPure"):
        flags.append("FUNC_BlueprintPure")
    if function.is_const:
        flags.append("FUNC_Const")
    if s.has("Exec"):
        flags.append("FUNC_Exec")
    if any(p.is_ref for p in function.params) or function.return_type != "void":
        flags.append("FUNC_HasOutParms")
    if any(p.has_default for p in function.params):
        flags.append("FUNC_HasDefaults")
    return " | ".join(flags)


# ---------------------------------------------------------------------------------------------------------------------
# Output

def sanitize(text):
    return re.sub(r"\W", "_", text)


class HeaderGenerator:
    def __init__(self, module, package, include_path, header_path, types):
        self.module = module
        self.package = package
        self.include_path = include_path                 # "ue5parse/runtime/Engine/Animation/Skeleton.h"
        self.header_path = header_path
        self.types = types
        self.stem = os.path.splitext(os.path.basename(include_path))[0]
        self.file_id = "FID_" + sanitize(module + "_" + os.path.splitext(self.relative_path())[0]) + "_h"

    def relative_path(self):
        marker = f"/{self.module}/"
        index = self.include_path.find(marker)
        return self.include_path[index + len(marker):] if index >= 0 else self.include_path

    def body_macro(self, decl):
        return f"{self.file_id}_{decl.body_line}_GENERATED_BODY"

    def check(self, decl):
        for function in decl.functions:
            s = function.specifiers
            if function.calls_through_process_event and function.is_static:
                raise UhtError(f"{self.header_path}:{function.line}: a static function cannot be an event or an RPC")
            if decl.kind == "struct":
                raise UhtError(f"{self.header_path}:{function.line}: UFUNCTION in a USTRUCT")

    def generated_h(self):
        guard = f"{sanitize(self.module).upper()}_{sanitize(self.stem)}_generated_h"
        out = [
            f"// Generated by UnrealHeaderTool.py from {self.relative_path()}. Do not edit.",
            "",
            f"#ifdef {guard}",
            f"#error \"{self.stem}.generated.h already included, missing '#pragma once' in {self.stem}.h\"",
            "#endif",
            f"#define {guard}",
            "",
            "#include \"ue5parse/runtime/CoreUObject/UObject/ObjectMacros.h\"",
            "#include \"ue5parse/runtime/CoreUObject/UObject/ReflectedTypeAccessors.h\"",
            "#include \"ue5parse/runtime/CoreUObject/UObject/ScriptMacros.h\"",
            "#include \"ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h\"",
            "",
        ]
        for decl in self.types:
            if decl.kind == "enum":
                if not decl.is_enum_class or not decl.enum_underlying:
                    continue
                lines = [f"enum class {decl.name} : {decl.enum_underlying};",
                         f"template <> {RUNTIME}UEnum* StaticEnum<{decl.name}>();"]
                out += ([f"namespace {decl.namespace} {{"] + lines + ["}"] if decl.namespace else lines) + [""]
                continue
            self.check(decl)
            lines = []
            if decl.kind == "struct":
                lines.append(f"friend struct Z_Construct_UScriptStruct_{decl.name}_Statics;")
                if decl.super_name:
                    lines.append(f"using Super = {decl.super_name};")
                lines.append(f"static {RUNTIME}UScriptStruct* StaticStruct();")
            else:
                cast_flag = f"{RUNTIME}CASTCLASS_{decl.name}" if decl.name in CAST_FLAG_CLASSES else "0"
                lines += [
                    "public:",
                    f"friend struct Z_Construct_UClass_{decl.name}_Statics;",
                    f"using Super = {decl.super_name};",
                    f"using ThisClass = {decl.name};",
                    f"static constexpr {RUNTIME}uint64 StaticClassCastFlags = {cast_flag};",
                    f"static {RUNTIME}UClass* StaticClass();",
                ]
                # A CustomThunk function declares its exec function itself (DECLARE_FUNCTION), as in UE.
                for function in decl.functions:
                    if not function.specifiers.has("CustomThunk") and function.has_native_body:
                        lines.append(f"static void exec{function.name}({RUNTIME}UObject* Context, {RUNTIME}FFrame& Stack, "
                                     f"void* const RESULT_PARAM);")
                if not decl.has_constructor:
                    lines.append(f"{decl.name}(const {RUNTIME}FObjectInitializer& ObjectInitializer = "
                                 f"{RUNTIME}FObjectInitializer::Get());")
                # DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL / DEFINE_DEFAULT_CONSTRUCTOR_CALL: a member, so
                # that a private constructor works.
                arguments = "(X)" if decl.has_object_initializer_constructor or not decl.has_constructor else ""
                lines.append(f"static void __DefaultConstructor(const {RUNTIME}FObjectInitializer& X) {{ "
                             f"new (static_cast<void*>(X.GetObj())) {decl.name}{arguments}; }}")
                if not decl.has_destructor:
                    lines.append(f"virtual ~{decl.name}();")
                lines.append("private:")
            out.append(f"#define {self.body_macro(decl)} \\")
            out += [f"    {line} \\" for line in lines[:-1]] + [f"    {lines[-1]}", ""]
        out += ["#undef CURRENT_FILE_ID", f"#define CURRENT_FILE_ID {self.file_id}", ""]
        return "\n".join(out) + "\n"

    def gen_cpp(self):
        out = [
            f"// Generated by UnrealHeaderTool.py from {self.relative_path()}. Do not edit.",
            "",
            "#include <cstddef>",
            "",
            f"#include \"{self.include_path}\"",
            "#include \"ue5parse/runtime/CoreUObject/UObject/Class.h\"",
            "#include \"ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h\"",
            "",
        ]
        registrations = {}   # namespace -> (classes, structs)
        open_namespace = None
        for decl in self.types:
            namespace = decl.namespace
            if namespace != open_namespace:
                if open_namespace:
                    out += ["}", ""]
                if namespace:
                    out += [f"namespace {namespace} {{", ""]
                open_namespace = namespace
            classes, structs = registrations.setdefault(namespace, ([], []))
            if decl.kind == "enum":
                if decl.is_enum_class and decl.enum_underlying:
                    out += [f"template <> {RUNTIME}UEnum* StaticEnum<{decl.name}>() {{",
                            f"    static {RUNTIME}UEnum* const Enum = {RUNTIME}FindNativeEnum(::ue5parse::FName(\"{decl.name}\"));",
                            "    return Enum;", "}", ""]
            elif decl.kind == "struct":
                out += self.struct_cpp(decl)
                structs.append(decl)
            else:
                out += self.class_cpp(decl)
                classes.append(decl)
        if open_namespace:
            out += ["}", ""]
        for namespace, (classes, structs) in registrations.items():
            if not classes and not structs:
                continue
            if namespace:
                out.append(f"namespace {namespace} {{")
            out.append("namespace {")
            if classes:
                out.append(f"const {RUNTIME}FNativeClassInfo Z_CompiledInClasses[] = {{")
                out += [f"    Z_Construct_UClass_{decl.name}_Statics::ClassInfo()," for decl in classes]
                out.append("};")
            if structs:
                out.append(f"const {RUNTIME}FNativeStructInfo Z_CompiledInStructs[] = {{")
                out += [f"    Z_Construct_UScriptStruct_{decl.name}_Statics::StructInfo()," for decl in structs]
                out.append("};")
            out.append(f"const {RUNTIME}FRegisterCompiledInInfo Z_CompiledInDeferFile_{self.file_id}(")
            out.append(f"    {'Z_CompiledInClasses' if classes else '{}'}, {'Z_CompiledInStructs' if structs else '{}'});")
            out += ["}", "}", ""] if namespace else ["}", ""]
        return "\n".join(out)

    def members(self, decl):
        """The FNativeMember table of a type's UPROPERTY members."""
        if not decl.properties:
            return []
        out = [f"    static constexpr {RUNTIME}FNativeMember Members[] = {{"]
        for prop in decl.properties:
            out.append(f"        {{\"{prop.name}\", static_cast<{RUNTIME}int32>(offsetof({decl.name}, {prop.name})), "
                       f"static_cast<{RUNTIME}int32>(sizeof({decl.name}::{prop.name}))}},")
        out.append("    };")
        return out

    def struct_cpp(self, decl):
        statics = f"Z_Construct_UScriptStruct_{decl.name}_Statics"
        out = [
            f"{RUNTIME}UScriptStruct* {decl.name}::StaticStruct() {{",
            f"    static {RUNTIME}UScriptStruct* const Struct = {RUNTIME}FindNativeStruct(::ue5parse::FName(\"{decl.script_name}\"));",
            "    return Struct;",
            "}",
            "",
            f"struct {statics} {{",
            f"    static constexpr {RUNTIME}FCppStructOps Ops = {RUNTIME}TCppStructOps<{decl.name}>();",
        ]
        out += self.members(decl)
        members = "Members" if decl.properties else "{}"
        out += [
            f"    static constexpr {RUNTIME}FNativeStructInfo StructInfo() {{",
            f"        return {{\"{self.package}\", \"{decl.script_name}\", &Ops, {members}}};",
            "    }",
            "};",
            "",
        ]
        return out

    def class_cpp(self, decl):
        statics = f"Z_Construct_UClass_{decl.name}_Statics"
        out = [
            f"{RUNTIME}UClass* {decl.name}::StaticClass() {{",
            f"    static {RUNTIME}UClass* const Class = {RUNTIME}GetNativeClass(\"{decl.script_name}\");",
            "    return Class;",
            "}",
        ]
        if not decl.has_constructor:
            out.append(f"{decl.name}::{decl.name}(const {RUNTIME}FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {{}}")
        if not decl.has_destructor:
            out.append(f"{decl.name}::~{decl.name}() {{}}")
        out.append("")
        natives = decl.functions
        for function in natives:
            if function.calls_through_process_event:
                out += self.process_event_caller(decl, function)
            if function.specifiers.has("CustomThunk") or not function.has_native_body:
                continue
            out += self.thunk(decl, function)
        class_flags = []
        if decl.specifiers.has("Abstract"):
            class_flags.append(f"{RUNTIME}CLASS_Abstract")
        if decl.specifiers.has("DefaultToInstanced"):
            class_flags.append(f"{RUNTIME}CLASS_DefaultToInstanced")
        if decl.specifiers.has("EditInlineNew"):
            class_flags.append(f"{RUNTIME}CLASS_EditInlineNew")
        if decl.specifiers.has("Transient"):
            class_flags.append(f"{RUNTIME}CLASS_Transient")
        out.append(f"struct {statics} {{")
        out += self.members(decl)
        for function in natives:
            params = self.param_table(function)
            if params:
                out.append(f"    static constexpr {RUNTIME}FNativeParam Params_{function.name}[] = {{")
                out += [f"        {param}," for param in params]
                out.append("    };")
        if natives:
            out.append(f"    static constexpr {RUNTIME}FNativeFunction Functions[] = {{")
            for function in natives:
                flags = " | ".join(f"{RUNTIME}{flag}" for flag in function_flags(function).split(" | ") if flag) or "0"
                thunk = f"&{decl.name}::exec{function.name}" if function.has_native_body else "nullptr"
                params = f"Params_{function.name}" if self.param_table(function) else "{}"
                out.append(f"        {{\"{function.name}\", {thunk}, {flags}, {params}}},")
            out.append("    };")
        super_script = TypeDecl("class", decl.super_name, "", Specifiers(), 0).script_name
        out += [
            f"    static constexpr {RUNTIME}FNativeClassInfo ClassInfo() {{",
            f"        return {{\"{self.package}\", \"{decl.script_name}\", \"{super_script}\", sizeof({decl.name}), "
            f"alignof({decl.name}), &{decl.name}::__DefaultConstructor, {' | '.join(class_flags) or '0'}, "
            f"{decl.name}::StaticClassCastFlags, {'Members' if decl.properties else '{}'}, "
            f"{'Functions' if natives else '{}'}}};",
            "    }",
            "};",
            "",
        ]
        return out

    def param_table(self, function):
        """The FNativeParam entries of a function: its parameters, then its return value (UE's order). Empty when a
        CustomThunk function has a parameter the runtime has no type for."""
        if any(not param.script_type for param in function.params):
            return []
        entries = [f"{{\"{param.name}\", \"{param.script_type}\", {param_flags(param)}}}" for param in function.params]
        if function.return_script_type:
            entries.append(f"{{\"ReturnValue\", \"{function.return_script_type}\", "
                           f"{RUNTIME}CPF_Parm | {RUNTIME}CPF_OutParm | {RUNTIME}CPF_ReturnParm}}")
        return entries

    def process_event_caller(self, decl, function):
        """The C++ body of an event or RPC: its parameters in a struct laid out as the function's frame, then
        ProcessEvent on the function the object's class has for the name (a Blueprint override, or the native one that
        runs Foo_Implementation); out parameters and the return value are copied back (UE's generated event caller)."""
        parms = f"{decl.script_name}_event{function.name}_Parms"
        signature = ", ".join(f"{param.cpp_type} {param.name}" for param in function.params)
        const = " const" if function.is_const else ""
        lines = [f"{function.return_type} {decl.name}::{function.name}({signature}){const} {{", f"    struct {parms} {{"]
        lines += [f"        {param.base_type} {param.name};" for param in function.params]
        if function.return_type != "void":
            lines.append(f"        {function.return_type} ReturnValue;")
        lines += ["    };", f"    {parms} Parms{{}};"]
        lines += [f"    Parms.{param.name} = {param.name};" for param in function.params]
        lines.append(f"    static const ::ue5parse::FName NAME_{decl.name}_{function.name}(\"{function.name}\");")
        self_expr = f"const_cast<{decl.name}*>(this)" if function.is_const else "this"
        lines.append(f"    {self_expr}->ProcessEvent({self_expr}->FindFunctionChecked(NAME_{decl.name}_{function.name}), &Parms);")
        lines += [f"    {param.name} = Parms.{param.name};" for param in function.params if param.is_ref and not param.is_const]
        if function.return_type != "void":
            lines.append("    return Parms.ReturnValue;")
        lines += ["}", ""]
        return lines

    def thunk(self, decl, function):
        lines = [f"void {decl.name}::exec{function.name}({RUNTIME}UObject* Context, {RUNTIME}FFrame& Stack, void* const RESULT_PARAM) {{"]
        args = []
        for param in function.params:
            get, arg = thunk_param(param, function, decl)
            lines.append(f"    {get}")
            args.append(arg)
        lines.append("    P_FINISH;")
        lines.append("    P_NATIVE_BEGIN;")
        name = f"{function.name}_Implementation" if function.calls_through_process_event else function.name
        target = f"{decl.name}::{name}" if function.is_static else f"P_THIS->{name}"
        call = f"{target}({', '.join(args)})"
        if function.return_type == "void":
            lines.append(f"    {call};")
        else:
            lines.append(f"    *({function.return_type}*)RESULT_PARAM = {call};")
        lines.append("    P_NATIVE_END;")
        lines += ["}", ""]
        return lines


def write_if_changed(path, text):
    """Unchanged outputs keep their time stamps, so that nothing recompiles (ninja restat)."""
    try:
        with open(path, encoding="utf-8") as f:
            if f.read() == text:
                return
    except FileNotFoundError:
        pass
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--module", required=True, help="the module name, e.g. Engine")
    parser.add_argument("--package", required=True, help="the script package, e.g. /Script/Engine")
    parser.add_argument("--include-root", required=True, help="the directory the headers are included relative to")
    parser.add_argument("--output", required=True, help="the directory for the generated files")
    parser.add_argument("--libclang", help="the libclang library (default: found on the system)")
    parser.add_argument("arguments", nargs="+", help="headers, -I/-D flags and @response files")
    args, flags = parser.parse_known_args(read_response_files(argv))
    arguments = args.arguments + flags
    headers = [argument for argument in arguments if not argument.startswith("-")]
    compile_flags = [argument for argument in arguments if argument.startswith(("-I", "-D")) and argument not in ("-I", "-D")]

    os.makedirs(args.output, exist_ok=True)
    try:
        stems = {}
        for header in headers:
            stem = os.path.splitext(os.path.basename(header))[0]
            if stem in stems:
                raise UhtError(f"{header}: a header named {stem}.h is in the module already ({stems[stem]})")
            stems[stem] = header
        reader = ModuleReader(headers, compile_flags, find_libclang(args.libclang), os.path.join(args.output, ".parse"))
        for header, types in reader.parse().items():
            include_path = os.path.relpath(os.path.abspath(header), os.path.abspath(args.include_root)).replace(os.sep, "/")
            generator = HeaderGenerator(args.module, args.package, include_path, header, types)
            write_if_changed(os.path.join(args.output, f"{generator.stem}.generated.h"), generator.generated_h())
            write_if_changed(os.path.join(args.output, f"{generator.stem}.gen.cpp"), generator.gen_cpp())
    except UhtError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

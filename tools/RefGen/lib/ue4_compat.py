#!/usr/bin/env python3
"""ue4_compat.py <project Source dir> <engine Build.version>

UE4's UnrealHeaderTool rejects reflected declarations UE5 accepts, and it cannot skip them behind a version #if.
For UE4 only, rewrite the project's (copied) headers:
  TObjectPtr<T>                          -> T*            (TObjectPtr is UE5)
  BlueprintRead* on double               -> dropped       (Blueprint doubles are UE5)
  BlueprintRead* on int64 (before 4.22)  -> dropped
  TSoftObjectPtr/TSoftClassPtr/FSoftObjectPath/FSoftClassPath (before 4.18) -> TAssetPtr/TAssetSubclassOf/FStringAssetReference/FStringClassReference
  UPrimaryDataAsset base (before 4.16)    -> UDataAsset
  LoadClass<T>(a, b[, ...]) (before 4.11)   -> all five arguments
  T Member[N] = { 0, ... } (before 4.15)  -> T Member[N]       (VS2015 has no in-class array initializers)
  GameplayTags/Abilities members, includes, classes and dependencies (before 4.8) -> removed (see RG_GAMEPLAY)
  TSet (before 4.13) / TMap (before 4.8) UPROPERTYs -> plain members (not reflected or saved)
  GENERATED_BODY() in a USTRUCT (before 4.8) -> GENERATED_USTRUCT_BODY()
  members named like a core struct (Vector, Rotator, ...; before 4.8) -> <Name>Value, also in `D->Name` uses
  CreateUserDefinedPin(Name, Type, Dir) (before 4.8) -> CreateUserDefinedPin(Name, Type)  (direction follows the node)
  UClass default constructors (before 4.8)  -> FObjectInitializer constructors
  CreateDefaultSubobject<T>(Name) (before 4.7) -> ObjectInitializer.CreateDefaultSubobject<T>(this, Name)
  exposed UPROPERTYs without a Category (before 4.6) -> Category = "RefGen" added (editor metadata only)
  GENERATED_BODY() in a UCLASS (before 4.6) -> GENERATED_UCLASS_BODY(), which declares the (PCIP) constructor:
                                          explicit declarations are dropped, missing definitions go to <Module>Ctors.cpp
  several UCLASSes in one header (before 4.8) -> one header per class, named after it (old UHT requires that);
                                          the enums/structs move to <Header>Defs.h, the original header includes all
Before 4.16 the rules files also use the old UBT API:
  ModuleRules(ReadOnlyTargetRules Target) : base(Target) -> ModuleRules(TargetInfo Target)
  TargetRules(TargetInfo Target) : base(Target)          -> TargetRules(TargetInfo Target)
  ExtraModuleNames.Add/AddRange(...)                      -> SetupBinaries(..., ref OutExtraModuleNames) override
The properties themselves stay, so the serialized data of the generated assets is unchanged."""
import json, pathlib, re, sys

src, version = sys.argv[1], json.load(open(sys.argv[2]))
major, minor = version["MajorVersion"], version["MinorVersion"]
if major >= 5:
    sys.exit(0)
no_bp = ["double"] + (["int64"] if minor < 22 else []) + (["TSet", "TMap"] if minor < 15 else [])  # BP sets/maps are 4.15+
changed = []
for h in pathlib.Path(src).rglob("*.h"):
    s = old = h.read_text()
    while (t := re.sub(r"TObjectPtr<\s*([A-Za-z_][\w:]*)\s*>", r"\1*", s)) != s:
        s = t
    if minor < 18:  # the soft reference types were renamed in 4.18; same serialized form
        for a, b in (("TSoftObjectPtr", "TAssetPtr"), ("TSoftClassPtr", "TAssetSubclassOf"),
                     ("FSoftObjectPath", "FStringAssetReference"), ("FSoftClassPath", "FStringClassReference")):
            s = re.sub(r"\b" + a + r"\b", b, s)
    if minor < 16:  # UPrimaryDataAsset (asset manager) is 4.16+
        s = re.sub(r"\bpublic\s+UPrimaryDataAsset\b", "public UDataAsset", s)
    if minor < 15:  # VS2015 can't initialize array members in-class; UObject memory starts zeroed anyway
        s = re.sub(r"(\b\w+\s+\w+\[\w+\])\s*=\s*\{\s*0(?:\s*,\s*0)*\s*\}\s*;", r"\1;", s)
    if minor < 13:  # TSet UPROPERTYs are 4.13+: keep the member but don't reflect (or save) it
        s = re.sub(r"UPROPERTY\([^)]*\)\s*(TSet\s*<)", r"\1", s)
    if minor < 8:  # same for TMap UPROPERTYs (4.8+)
        s = re.sub(r"UPROPERTY\([^)]*\)\s*(TMap\s*<)", r"\1", s)
    if minor < 13:  # attributes were plain floats before 4.13
        s = re.sub(r"\bFGameplayAttributeData\b", "float", s)
    for ty in no_bp:
        s = re.sub(r"UPROPERTY\(([^)]*)\)(\s*)" + ty + r"\b",
                   lambda m: "UPROPERTY(" + re.sub(r",?\s*BlueprintRead(Write|Only)", "", m.group(1)).strip(", ") + ")" + m.group(2) + ty, s)
    if s != old:
        h.write_text(s)
        changed.append(h.name)
if minor < 16:
    for f in list(pathlib.Path(src).rglob("*.Build.cs")) + list(pathlib.Path(src).rglob("*.Target.cs")):
        s = old = f.read_text()
        s = re.sub(r"\(\s*ReadOnlyTargetRules\s+Target\s*\)\s*:\s*base\(\s*Target\s*\)", "(TargetInfo Target)", s)
        s = re.sub(r"\(\s*TargetInfo\s+Target\s*\)\s*:\s*base\(\s*Target\s*\)", "(TargetInfo Target)", s)
        if minor < 15:  # explicit PCHs (UseExplicitOrSharedPCHs) are 4.15+
            s = s.replace("PCHUsageMode.UseExplicitOrSharedPCHs", "PCHUsageMode.NoSharedPCHs")
        extra = re.findall(r"^\s*ExtraModuleNames\.(Add|AddRange)\((.*)\);\s*$", s, re.M)
        if extra:
            s = re.sub(r"^\s*ExtraModuleNames\.(Add|AddRange)\(.*\);\s*\n", "", s, flags=re.M)
            body = "".join(f"\t\tOutExtraModuleNames.{m}({a});\n" for m, a in extra)
            method = ("\n\tpublic override void SetupBinaries(TargetInfo Target, "
                      "ref System.Collections.Generic.List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations, "
                      "ref System.Collections.Generic.List<string> OutExtraModuleNames)\n\t{\n" + body + "\t}\n")
            i = s.rstrip().rfind("}")  # before the class's closing brace
            s = s[:i] + method + s[i:]
        if s != old:
            f.write_text(s)
            changed.append(f.name)
if minor < 13:  # no initializer_list support in TArray/TSet: `X = { a, b };` -> RefGen::AssignList(X, a, b);
    for f in pathlib.Path(src).rglob("*.cpp"):
        t = f.read_text()
        n = re.sub(r"^(\s*)(TArray<[^=;]+>)\s+(\w+)\s*=\s*\{(.*)\};[ \t]*$", r"\1\2 \3;\n\1RefGen::AssignList(\3, \4);", t, flags=re.M)
        n = re.sub(r"^(\s*)([\w.>\-]+)\s*=\s*\{(.*)\};[ \t]*$", r"\1RefGen::AssignList(\2, \3);", n, flags=re.M)
        if n != t:
            f.write_text(n)
            changed.append(f.name)
if minor < 11:  # LoadClass has no default arguments before 4.11: pad to (Outer, Name, Filename, LoadFlags, Sandbox)
    defaults = ["nullptr", "nullptr", "nullptr", "LOAD_None", "nullptr"]

    def pad_load_class(t):
        out, i = [], 0
        for m in re.finditer(r"\bLoadClass<[^<>()]+>\(", t):
            if m.start() < i:
                continue
            j, depth, args, start = m.end(), 1, [], m.end()
            while depth:  # split the call's arguments at top-level commas
                ch = t[j]
                if ch in "([{":
                    depth += 1
                elif ch in ")]}":
                    depth -= 1
                if (ch == "," and depth == 1) or depth == 0:
                    args.append(t[start:j].strip())
                    start = j + 1
                j += 1
            args += defaults[len(args):]
            out.append(t[i:m.end()] + ", ".join(args) + ")")
            i = j
        return "".join(out) + t[i:]

    for f in pathlib.Path(src).rglob("*.cpp"):
        t = f.read_text()
        n = pad_load_class(t)
        if n != t:
            f.write_text(n)
            changed.append(f.name)
if minor < 14:  # many per-class headers (UObject/Package.h, ...) only exist since 4.14; the monolithic PCH covers them
    for f in pathlib.Path(src).rglob("*"):
        if f.suffix not in (".cpp", ".h", ".inl") or "generated.h" in (t := f.read_text()):
            continue
        n = re.sub(r'^#include "((?!RefGen|RefTypes)[^"]+)"[ \t]*$',
                   lambda m: f'#if __has_include("{m.group(1)}")\n#include "{m.group(1)}"\n#endif', t, flags=re.M)
        if n != t:
            f.write_text(n)
            changed.append(f.name)
if minor < 15:  # pre-IWYU UBT: every .cpp in a module must include the same header first
    for d, umbrella in (("RefGen", "Engine.h"), ("RefGenEditor", "UnrealEd.h")):
        mod = pathlib.Path(src) / d
        if not mod.is_dir():
            continue
        (mod / f"{d}PCH.h").write_text(f'#pragma once\n#include "{umbrella}"\n')
        for c in mod.glob("*.cpp"):
            t = c.read_text()
            if not t.startswith(f'#include "{d}PCH.h"'):
                c.write_text(f'#include "{d}PCH.h"\n' + t)
                changed.append(c.name)
if minor < 8:  # 4.7 and older: no GameplayTags/Abilities (installed-build UHT orders AIModule before GameplayTags)
    for h in pathlib.Path(src).rglob("*.h"):
        s = old = h.read_text()
        s = re.sub(r'^#include "(GameplayTagContainer|AttributeSet|AbilitySystemComponent)\.h"[ \t]*\n', "", s, flags=re.M)
        s = re.sub(r"^[ \t]*(UPROPERTY\([^)]*\)\s*)?(FGameplayTag|FGameplayTagContainer|UAbilitySystemComponent\s*\*|TObjectPtr<UAbilitySystemComponent>)\s+\w+;[ \t]*\n",
                   "", s, flags=re.M)
        s = re.sub(r"^UCLASS\([^)]*\)\s*\nclass\s+\w+\s+\w+\s*:\s*public\s+UAttributeSet\b.*?^\};[ \t]*\n\n?", "", s, flags=re.M | re.S)
        if s != old:
            h.write_text(s)
            changed.append(h.name)
    for f in pathlib.Path(src).rglob("*.Build.cs"):
        s = old = f.read_text()
        s = re.sub(r'"(GameplayTags|GameplayAbilities|GameplayTasks)",?\s*', "", s)  # C# allows a trailing comma
        if s != old:
            f.write_text(s)
            changed.append(f.name)
if minor < 8:  # 4.7 UHT: GENERATED_USTRUCT_BODY only, and members may not share a name with Object.h's structs
    core = {"Guid", "Vector", "Vector4", "Vector2D", "Plane", "Rotator", "Quat", "IntPoint", "IntVector", "Color",
            "LinearColor", "Box", "Matrix", "Transform", "RandomStream", "DateTime", "Timespan", "FloatRange", "Int32Range"}
    renamed = set()
    for h in pathlib.Path(src).rglob("*.h"):
        s = old = h.read_text()
        s = re.sub(r"(USTRUCT\([^)]*\)\s*\nstruct\s[^{]*\{\s*)GENERATED_BODY\(\)", r"\1GENERATED_USTRUCT_BODY()", s)

        def member(m):
            if m.group(2) not in core:
                return m.group(0)
            renamed.add(m.group(2))
            return m.group(1) + m.group(2) + "Value"
        s = re.sub(r"(UPROPERTY\([^)]*\)\s*[\w:<>, *]+?\s)(\w+)(?=\s*(=|;|\[))", member, s)
        if s != old:
            h.write_text(s)
            changed.append(h.name)
    for f in pathlib.Path(src).rglob("*.cpp"):
        t = f.read_text()
        n = re.sub(r"\bD->(" + "|".join(sorted(renamed)) + r")\b", r"D->\1Value", t) if renamed else t
        if n != t:
            f.write_text(n)
            changed.append(f.name)
if minor < 8:  # 4.7: no pin direction argument, and UObject constructors take an FObjectInitializer
    ctors = set()
    for h in pathlib.Path(src).rglob("*.h"):
        t = h.read_text()
        for name in re.findall(r"^UCLASS\(.*?\)\s*\nclass\s+(?:\w+_API\s+)?(\w+)", t, flags=re.M | re.S):
            n = re.sub(r"^(\s*)" + name + r"\(\);", r"\1" + name + "(const FObjectInitializer& ObjectInitializer);", t, flags=re.M)
            if n != t:
                ctors.add(name)
                t = n
                h.write_text(t)
                changed.append(h.name)
    for f in pathlib.Path(src).rglob("*.cpp"):
        t = f.read_text()
        n = re.sub(r"(CreateUserDefinedPin\(.*),\s*EGPD_\w+\s*\)", r"\1)", t)
        for name in ctors:
            n = re.sub(r"\b" + name + "::" + name + r"\(\)", f"{name}::{name}(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)", n)
        if minor < 7:  # UObject::CreateDefaultSubobject is 4.7+
            n = re.sub(r"(?<![.>])\bCreateDefaultSubobject<(\w+)>\(", r"ObjectInitializer.CreateDefaultSubobject<\1>(this, ", n)
        if minor < 5:  # 4.4: pin categories are FString members of the K2 schema, lights have SetBrightness,
            # and the FBX import options predate the import-mesh toggles, auto collision and lightmap UV generation
            n = re.sub(r"\bUEdGraphSchema_K2::(P[CN]_\w+)", r"GetDefault<UEdGraphSchema_K2>()->\1", n)
            n = n.replace("->SetIntensity(", "->SetBrightness(")
            n = re.sub(r"^[ \t]*\w+->(?:\w+->)?(?:bImportMesh|bImportAsSkeletal|OriginalImportType|bAutoGenerateCollision|bGenerateLightmapUVs|bImportCustomAttribute) = [^;]+;[ \t]*\n", "", n, flags=re.M)
        if minor < 6:  # 4.5 actors have no component getters, only public TSubobjectPtr members of the same name
            n = re.sub(r"->Get(AudioComponent|CameraComponent|Component|Decal|LightComponent|SkeletalMeshComponent|StaticMeshComponent|TextRender|CaptureComponent|ParticleSystemComponent|BrushComponent)\(\)", r"->\1.Get()", n)
        if minor < 6:  # 4.5 subobjects come back as TSubobjectPtrConstructor, only convertible through a TSubobjectPtr
            n = re.sub(r"^(\s*)([\w.>-]+) = ObjectInitializer\.CreateDefaultSubobject<(\w+)>\((.*)\);",
                       r"\1{ TSubobjectPtr<\3> Sub = ObjectInitializer.CreateDefaultSubobject<\3>(\4); \2 = static_cast<\3*>(Sub.Get()); }", n, flags=re.M)
        if n != t:
            f.write_text(n)
            changed.append(f.name)
if minor < 6:  # 4.5 UHT: every editor/Blueprint-exposed property needs a Category
    def categorize(m):
        spec = m.group(1)
        if "Category" in spec or not re.search(r"\b(Edit\w*|Visible\w*|Blueprint\w*)\b", spec):
            return m.group(0)
        return "UPROPERTY(" + spec + ', Category = "RefGen")'
    for h in pathlib.Path(src).rglob("*.h"):
        t = h.read_text()
        n = re.sub(r"UPROPERTY\(([^()]*(?:\([^()]*\)[^()]*)*)\)", categorize, t)
        if n != t:
            h.write_text(n)
            changed.append(h.name)
if minor < 8:  # UHT before 4.8: one UCLASS per header, and the header must be named after it (URefFoo -> RefFoo.h)
    for h in list(pathlib.Path(src).rglob("*.h")):
        t = h.read_text()
        gen = f'#include "{h.stem}.generated.h"'
        classes = re.findall(r"^UCLASS\(.*?\)\s*\nclass\s+(?:\w+_API\s+)?[UA](\w+)", t, flags=re.M | re.S)
        if gen not in t or not classes or classes == [h.stem]:
            continue
        head, body = t.split(gen, 1)
        blocks = re.split(r"^(?=(?:UENUM|USTRUCT|UCLASS)\()", body, flags=re.M)
        defs, extra = [blocks[0]], []
        for b in blocks[1:]:
            m = re.match(r"UCLASS\(.*?\)\s*\nclass\s+(?:\w+_API\s+)?[UA](\w+)", b, flags=re.S)
            if not m:
                defs.append(b)
                continue
            name = m.group(1)
            (h.parent / f"{name}.h").write_text(f'#pragma once\n#include "{h.stem}Defs.h"\n#include "{name}.generated.h"\n\n{b}')
            extra.append(name)
        (h.parent / f"{h.stem}Defs.h").write_text(head + f'#include "{h.stem}Defs.generated.h"' + "".join(defs))
        h.write_text("#pragma once\n" + "".join(f'#include "{n}.h"\n' for n in [h.stem + "Defs"] + extra))
        changed.append(f"{h.name} (split: {', '.join(extra)})")
if minor < 6:  # 4.5: GENERATED_UCLASS_BODY() only; it declares Class(const FPostConstructInitializeProperties&)
    pcip = "const class FPostConstructInitializeProperties& ObjectInitializer"
    classes = {}  # name -> header
    for h in pathlib.Path(src).rglob("*.h"):
        t = h.read_text()
        names = re.findall(r"^UCLASS\(.*?\)\s*\nclass\s+(?:\w+_API\s+)?(\w+)", t, flags=re.M | re.S)
        if not names:
            continue
        n = re.sub(r"(UCLASS\(.*?\)\s*\nclass\s[^{]*\{\s*)GENERATED_BODY\(\)", r"\1GENERATED_UCLASS_BODY()", t, flags=re.S)
        for name in names:
            classes[name] = h
            n = re.sub(r"^[ \t]*" + name + r"\(const FObjectInitializer& ObjectInitializer\);[ \t]*\n", "", n, flags=re.M)
        if n != t:
            h.write_text(n)
            changed.append(h.name)
    defined = set()
    for f in pathlib.Path(src).rglob("*.cpp"):
        t = f.read_text()
        n = t.replace("const FObjectInitializer& ObjectInitializer", pcip)
        defined |= {c for c in classes if re.search(r"\b" + c + "::" + c + r"\(", n)}
        if n != t:
            f.write_text(n)
            changed.append(f.name)
    for module in {h.parent for h in classes.values()}:
        missing = sorted(c for c, h in classes.items() if h.parent == module and c not in defined)
        if missing:
            body = f'#include "{module.name}PCH.h"\n' + "".join(f'#include "{classes[c].name}"\n' for c in missing)
            body += "".join(f"\n{c}::{c}({pcip})\n\t: Super(ObjectInitializer)\n{{\n}}\n" for c in missing)
            (module / f"{module.name}Ctors.cpp").write_text(body)
            changed.append(f"{module.name}Ctors.cpp")
if major == 4 and minor < 6:  # 4.5 and older only start a project whose [URL] GameName matches the .uproject name
    ini = pathlib.Path(src).parent / "Config" / "DefaultEngine.ini"
    if ini.exists() and not re.search(r"^GameName=", ini.read_text(), re.M):
        ini.write_text(ini.read_text().rstrip("\n") + "\n\n[URL]\nGameName=RefGen\n")
        changed.append(ini.name)
if major == 4 and minor < 5:  # same 4.4 pin category access in the editor headers (RefGenGraph.h)
    for h in pathlib.Path(src).rglob("*.h"):
        t = h.read_text()
        n = re.sub(r"\bUEdGraphSchema_K2::(P[CN]_\w+)", r"GetDefault<UEdGraphSchema_K2>()->\1", t)
        if n != t:
            h.write_text(n)
            changed.append(h.name)
if major == 4 and minor < 5:  # 4.4 UHT: no enum class and no default member initializers on reflected types
    enums = set()
    for h in pathlib.Path(src).rglob("*.h"):
        t = h.read_text()
        # UENUM() enum class E : uint8 { ... };  ->  namespace E { enum Type { ... }; }  (used as TEnumAsByte<E::Type>)
        n = re.sub(r"(UENUM\(.*?\)\s*\n)enum class (\w+)\s*(?::\s*\w+)?\s*\{(.*?)\};",
                   lambda m: enums.add(m.group(2)) or f"{m.group(1)}namespace {m.group(2)}\n{{\nenum Type\n{{{m.group(3)}}};\n}}", t, flags=re.S)
        if n != t:
            h.write_text(n)
    ctor_inits = {}  # class -> [assignment]
    member = re.compile(r"^(\s*(?:UPROPERTY\(.*\)\s*)?(?!return\b|static\b)[\w:<>,\* ]+?[\s\*&](\w+))(\[\w+\])?\s*=\s*(.+?);(\s*//.*)?$")
    for h in pathlib.Path(src).rglob("*.h"):
        lines, out, typ, kind, depth = h.read_text().split("\n"), [], None, None, 0
        inits = {}
        for line in lines:
            m = re.match(r"^(class|struct)\s+(?:\w+_API\s+)?(\w+)\b[^;]*$", line)
            if m and depth == 0:
                kind, typ = m.group(1), m.group(2)
            mm = member.match(line) if typ and depth == 1 else None
            if mm:
                name, arr, expr = mm.group(2), mm.group(3), mm.group(4).strip()
                line = mm.group(1) + (arr or "") + ";" + (mm.group(5) or "")
                inits.setdefault((kind, typ), []).append(
                    f"FMemory::Memzero({name});" if arr else f"{name} = {expr};")
            for e in enums:  # reflected members of an enum type hold a TEnumAsByte
                line = re.sub(r"(?<![\w:<])" + e + r"(?=\s+\w+\s*[;\[])", f"TEnumAsByte<{e}::Type>", line)
            depth += line.count("{") - line.count("}")
            if depth == 0 and line.startswith("};"):
                typ = None
            out.append(line)
        # structs get a default constructor right after GENERATED_USTRUCT_BODY()
        text = "\n".join(out)
        for (kind, typ), assigns in inits.items():
            if kind == "struct":
                text = re.sub(r"(struct\s+(?:\w+_API\s+)?" + typ + r"\b[^{;]*\{\s*\n\s*GENERATED_USTRUCT_BODY\(\)[ \t]*\n)",
                              lambda m: m.group(1) + f"\n\t{typ}()\n\t{{\n" + "".join(f"\t\t{a}\n" for a in assigns) + "\t}\n", text, count=1)
            else:
                ctor_inits[typ] = assigns
        h.write_text(text)
        changed.append(h.name)
    for f in pathlib.Path(src).rglob("*.cpp"):
        t = f.read_text()
        n = t
        for e in enums:
            n = re.sub(r"<" + e + r">", f"<{e}::Type>", n)
        for c, assigns in ctor_inits.items():
            n = re.sub(r"(\b" + c + "::" + c + r"\([^)]*\)[^{]*\{)", lambda m: m.group(1) + "".join(f"\n\t{a}" for a in assigns), n, count=1)
        if n != t:
            f.write_text(n)
            changed.append(f.name)
print(f"ue4_compat: {major}.{minor} -> rewrote {', '.join(changed) or 'nothing'}")

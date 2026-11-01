// ue5script <Paks folder> <file.usmap> <class or function path>...
//
// Loads each class (or one function) through the runtime and prints the bytecode of its functions, linked into the
// memory format the VM runs (UE5_AES_KEY holds the key). One statement per line, with its offset, so that jumps can be
// followed; calls print as Function(arguments), variables by name.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#include "ue5parse/crypto/Decryptor.h"
#include "ue5parse/io/FileSource.h"
#include "ue5parse/provider/FileProvider.h"
#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/Script.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

using namespace ue5parse;
namespace rt = ue5parse::runtime;

namespace {

void OnProblem(const char* Code, const rt::FString& Message) {
    if (std::getenv("UE5SCRIPT_PROBLEMS"))
        std::fprintf(stderr, "problem %s: %s\n", Code, Message.ToUtf8().c_str());
}

// Reads one function's linked bytecode.
class FScriptPrinter {
public:
    explicit FScriptPrinter(const rt::UStruct& InStruct) : Struct(InStruct), Code(InStruct.Script.GetData()), Size(InStruct.Script.Num()) {}

    void Print() {
        while (Offset < Size) {
            const rt::int32 At = Offset;
            const std::string Statement = Expr();
            std::printf("  %5d: %s\n", At, Statement.c_str());
            if (bFailed) {
                std::printf("  (stopped: unknown token)\n");
                return;
            }
        }
    }

private:
    template <class T>
    T Read() {
        T Value{};
        std::memcpy(&Value, Code + Offset, sizeof(T));
        Offset += sizeof(T);
        return Value;
    }
    std::string Object() {
        const auto* Pointer = reinterpret_cast<const rt::UObject*>(static_cast<std::uintptr_t>(Read<rt::ScriptPointerType>()));
        return Pointer != nullptr ? Pointer->GetPathName().ToUtf8() : std::string("null");
    }
    std::string ShortObject() {
        const auto* Pointer = reinterpret_cast<const rt::UObject*>(static_cast<std::uintptr_t>(Read<rt::ScriptPointerType>()));
        if (Pointer == nullptr)
            return "null";
        const rt::UObject* Outer = Pointer->GetOuter();
        return Outer != nullptr && rt::Cast<rt::UClass>(Outer) != nullptr ? Outer->GetName().ToUtf8() + "::" + Pointer->GetName().ToUtf8()
                                                                            : Pointer->GetName().ToUtf8();
    }
    std::string Property() {
        const auto* Field = reinterpret_cast<const rt::FField*>(static_cast<std::uintptr_t>(Read<rt::ScriptPointerType>()));
        return Field != nullptr ? Field->GetName().ToUtf8() : std::string("null");
    }
    std::string Name() {
        rt::uint32 Index = 0;
        std::memcpy(&Index, Code + Offset, sizeof(Index));
        Offset += rt::ScriptNameSize;
        return Index < static_cast<rt::uint32>(Struct.ScriptNames.Num()) ? rt::NameToString(Struct.ScriptNames[Index]).ToUtf8() : "?";
    }
    std::string String(bool bWide) {
        std::string Text;
        if (bWide) {
            for (rt::uint16 Char = Read<rt::uint16>(); Char != 0; Char = Read<rt::uint16>())
                Text += Char < 0x80 ? static_cast<char>(Char) : '?';
        } else {
            for (char Char = Read<char>(); Char != 0; Char = Read<char>())
                Text += Char;
        }
        return "\"" + Text + "\"";
    }
    std::string List(rt::EExprToken End) {
        std::string Text;
        while (!bFailed && Offset < Size && static_cast<rt::EExprToken>(Code[Offset]) != End) {
            if (!Text.empty())
                Text += ", ";
            Text += Expr();
        }
        Offset++;
        return Text;
    }

    std::string Expr();

    const rt::UStruct& Struct;
    const rt::uint8* Code;
    rt::int32 Size;
    rt::int32 Offset = 0;
    bool bFailed = false;
};

std::string FScriptPrinter::Expr() {
    using namespace rt;
    const auto Token = static_cast<EExprToken>(Read<uint8>());
    switch (Token) {
    case EX_LocalVariable: return Property();
    case EX_InstanceVariable: return "this." + Property();
    case EX_DefaultVariable: return "default." + Property();
    case EX_LocalOutVariable: return "out " + Property();
    case EX_ClassSparseDataVariable: return "sparse." + Property();
    case EX_PropertyConst: return "property " + Property();
    case EX_Return: return "return " + Expr();
    case EX_Jump: return std::format("jump {}", Read<CodeSkipSizeType>());
    case EX_JumpIfNot: {
        const CodeSkipSizeType Target = Read<CodeSkipSizeType>();
        return std::format("if not ({}) jump {}", Expr(), Target);
    }
    case EX_Assert: {
        const uint16 Line = Read<uint16>();
        Read<uint8>();
        return std::format("assert({}) line {}", Expr(), Line);
    }
    case EX_Nothing: return "nothing";
    case EX_NothingInt32: return std::format("nothing({})", Read<int32>());
    case EX_Let: {
        Property();
        std::string Target = Expr();
        return Target + " = " + Expr();
    }
    case EX_LetObj:
    case EX_LetWeakObjPtr:
    case EX_LetBool:
    case EX_LetDelegate:
    case EX_LetMulticastDelegate: {
        std::string Target = Expr();
        return Target + " = " + Expr();
    }
    case EX_LetValueOnPersistentFrame: {
        std::string Target = "persistent." + Property();
        return Target + " = " + Expr();
    }
    case EX_BitFieldConst: {
        std::string Field = Property();
        return std::format("{}:{}", Field, Read<uint8>());
    }
    case EX_ClassContext:
    case EX_Context:
    case EX_Context_FailSilent: {
        std::string Context = Expr();
        Read<CodeSkipSizeType>();
        Property();
        return Context + "->" + Expr();
    }
    case EX_MetaCast: {
        std::string Class = ShortObject();
        return std::format("MetaCast<{}>({})", Class, Expr());
    }
    case EX_DynamicCast: {
        std::string Class = ShortObject();
        return std::format("Cast<{}>({})", Class, Expr());
    }
    case EX_ObjToInterfaceCast:
    case EX_CrossInterfaceCast:
    case EX_InterfaceToObjCast: {
        std::string Class = ShortObject();
        return std::format("InterfaceCast<{}>({})", Class, Expr());
    }
    case EX_Cast: {
        const uint8 Conversion = Read<uint8>();
        return std::format("Cast#{}({})", Conversion, Expr());
    }
    case EX_EndParmValue: return "end_parm_value";
    case EX_EndFunctionParms: return ")";
    case EX_Self: return "self";
    case EX_Skip: {
        Read<CodeSkipSizeType>();
        return "skip " + Expr();
    }
    case EX_VirtualFunction:
    case EX_LocalVirtualFunction: {
        std::string Function = Name();
        return Function + "(" + List(EX_EndFunctionParms) + ")";
    }
    case EX_FinalFunction:
    case EX_LocalFinalFunction:
    case EX_CallMath: {
        std::string Function = ShortObject();
        return Function + "(" + List(EX_EndFunctionParms) + ")";
    }
    case EX_CallMulticastDelegate: {
        std::string Signature = ShortObject();
        std::string Arguments = List(EX_EndFunctionParms);
        return "broadcast<" + Signature + ">(" + Arguments + ")";
    }
    case EX_IntConst: return std::to_string(Read<int32>());
    case EX_FloatConst: return std::format("{}f", Read<float>());
    case EX_DoubleConst: return std::format("{}", Read<double>());
    case EX_Int64Const: return std::to_string(Read<int64>());
    case EX_UInt64Const: return std::to_string(Read<uint64>());
    case EX_StringConst: return String(false);
    case EX_UnicodeStringConst: return String(true);
    case EX_ObjectConst: return Object();
    case EX_NameConst: return "'" + Name() + "'";
    case EX_RotationConst:
    case EX_VectorConst: {
        const double X = Read<double>(), Y = Read<double>(), Z = Read<double>();
        return std::format("{}({}, {}, {})", Token == EX_VectorConst ? "Vector" : "Rotator", X, Y, Z);
    }
    case EX_Vector3fConst: {
        const float X = Read<float>(), Y = Read<float>(), Z = Read<float>();
        return std::format("Vector3f({}, {}, {})", X, Y, Z);
    }
    case EX_TransformConst: {
        std::string Text = "Transform(";
        for (int i = 0; i < 10; i++)
            Text += std::format("{}{}", i ? ", " : "", Read<double>());
        return Text + ")";
    }
    case EX_ByteConst:
    case EX_IntConstByte: return std::format("{}b", Read<uint8>());
    case EX_IntZero: return "0";
    case EX_IntOne: return "1";
    case EX_True: return "true";
    case EX_False: return "false";
    case EX_NoObject: return "none";
    case EX_NoInterface: return "no_interface";
    case EX_TextConst: {
        const uint8 Type = Read<uint8>();
        switch (Type) {
        case 0: return "text()";
        case 1: {
            std::string Source = Expr();
            std::string Key = Expr();
            std::string Namespace = Expr();
            return "text(" + Source + ", " + Key + ", " + Namespace + ")";
        }
        case 2:
        case 3: return "text(" + Expr() + ")";
        case 4: {
            std::string Table = Object();
            std::string Id = Expr();
            return "text(" + Table + ", " + Id + ", " + Expr() + ")";
        }
        default: bFailed = true; return "text?";
        }
    }
    case EX_StructConst: {
        std::string Struct = ShortObject();
        Read<int32>();
        return Struct + "{" + List(EX_EndStructConst) + "}";
    }
    case EX_SetArray: {
        std::string Target = Expr();
        return Target + " = [" + List(EX_EndArray) + "]";
    }
    case EX_SetSet: {
        std::string Target = Expr();
        Read<int32>();
        return Target + " = set{" + List(EX_EndSet) + "}";
    }
    case EX_SetMap: {
        std::string Target = Expr();
        Read<int32>();
        return Target + " = map{" + List(EX_EndMap) + "}";
    }
    case EX_ArrayConst: {
        Property();
        Read<int32>();
        return "[" + List(EX_EndArrayConst) + "]";
    }
    case EX_SetConst: {
        Property();
        Read<int32>();
        return "set{" + List(EX_EndSetConst) + "}";
    }
    case EX_MapConst: {
        Property();
        Property();
        Read<int32>();
        return "map{" + List(EX_EndMapConst) + "}";
    }
    case EX_StructMemberContext: {
        std::string Member = Property();
        return Expr() + "." + Member;
    }
    case EX_InstanceDelegate: return "delegate " + Name();
    case EX_BindDelegate: {
        std::string Function = Name();
        std::string Delegate = Expr();
        return Delegate + ".Bind(" + Expr() + ", " + Function + ")";
    }
    case EX_AddMulticastDelegate: {
        std::string Delegate = Expr();
        return Delegate + " += " + Expr();
    }
    case EX_RemoveMulticastDelegate: {
        std::string Delegate = Expr();
        return Delegate + " -= " + Expr();
    }
    case EX_ClearMulticastDelegate: return Expr() + ".Clear()";
    case EX_ArrayGetByRef: {
        std::string Array = Expr();
        return Array + "[" + Expr() + "]";
    }
    case EX_PushExecutionFlow: return std::format("push_flow {}", Read<CodeSkipSizeType>());
    case EX_PopExecutionFlow: return "pop_flow";
    case EX_PopExecutionFlowIfNot: return "pop_flow_if_not " + Expr();
    case EX_ComputedJump: return "computed_jump " + Expr();
    case EX_SkipOffsetConst: return std::format("offset {}", Read<CodeSkipSizeType>());
    case EX_InterfaceContext: return "interface " + Expr();
    case EX_SoftObjectConst: return "soft " + Expr();
    case EX_FieldPathConst: return "field_path " + Expr();
    case EX_AutoRtfmAbortIfNot: return "rtfm_abort_if_not " + Expr();
    case EX_SwitchValue: {
        const uint16 Cases = Read<uint16>();
        Read<CodeSkipSizeType>();
        std::string Text = "switch(" + Expr() + ") {";
        for (uint16 i = 0; i < Cases && !bFailed; i++) {
            std::string Label = Expr();
            Read<CodeSkipSizeType>();
            Text += " " + Label + ": " + Expr() + ";";
        }
        return Text + " default: " + Expr() + " }";
    }
    case EX_InstrumentationEvent: Read<uint8>(); return "instrumentation";
    case EX_DeprecatedOp4A:
    case EX_WireTracepoint:
    case EX_Tracepoint:
    case EX_Breakpoint: return "tracepoint";
    case EX_EndOfScript: return "end";
    case EX_AutoRtfmTransact: {
        Read<int32>();
        Read<CodeSkipSizeType>();
        return "rtfm{" + List(EX_AutoRtfmStopTransact) + "}";
    }
    case EX_AutoRtfmStopTransact: {
        Read<int32>();
        Read<int8>();
        return "rtfm_stop";
    }
    default: bFailed = true; return std::format("token 0x{:02x}", static_cast<int>(Token));
    }
}

void PrintFunction(rt::UFunction& Function) {
    std::printf("function %s flags 0x%08x\n", Function.GetName().ToUtf8().c_str(), static_cast<unsigned>(Function.FunctionFlags));
    for (const rt::FField* Field = Function.ChildProperties; Field != nullptr; Field = Field->Next)
        std::printf("  local %s %s\n", Field->GetClass()->GetName(), Field->GetName().ToUtf8().c_str());
    if (!Function.EnsureScriptLinked()) {
        std::printf("  (bytecode not linked)\n");
        return;
    }
    FScriptPrinter(Function).Print();
}

}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "ue5script <Paks folder> <file.usmap> <class or function path>...\n");
        return 2;
    }
    MountOptions Options;
    Result<std::vector<std::unique_ptr<FileSource>>> Archives = openLocalFiles(argv[1]);
    if (!Archives) {
        std::fprintf(stderr, "cannot open %s: %s\n", argv[1], getErrorName(Archives.error().code));
        return 1;
    }
    Options.archives = std::move(*Archives);
    if (const char* Key = std::getenv("UE5_AES_KEY")) {
        Result<AesKey> Parsed = parseAesKey(Key);
        if (!Parsed) {
            std::fprintf(stderr, "UE5_AES_KEY is not a key\n");
            return 1;
        }
        KeyRing Keys;
        Keys.add(FGuid{}, *Parsed);
        Options.decryptor = std::make_unique<AesDecryptor>(Keys);
    }
    Result<std::unique_ptr<FileSource>> Mappings = openLocalFile(argv[2]);
    if (!Mappings)
        return 1;
    Options.mappings = std::move(*Mappings);
    Options.game = EGame::GAME_Valorant;
    Result<MountedGame> Game = mount(std::move(Options));
    if (!Game) {
        std::fprintf(stderr, "mount failed: %s\n", getErrorName(Game.error().code));
        return 1;
    }
    FileProvider Provider(std::move(*Game));
    rt::FRuntime Runtime(Provider);
    rt::SetLoadProblemHandler(&OnProblem);

    for (int i = 3; i < argc; i++) {
        rt::UObject* Object = rt::StaticLoadObject(nullptr, nullptr, rt::FString(argv[i]));
        if (Object == nullptr) {
            std::printf("%s: not loaded\n", argv[i]);
            continue;
        }
        if (auto* Function = rt::Cast<rt::UFunction>(Object)) {
            PrintFunction(*Function);
            continue;
        }
        auto* Class = rt::Cast<rt::UClass>(Object);
        if (Class == nullptr) {
            std::printf("%s: %s is neither a class nor a function\n", argv[i], Object->GetClass()->GetName().ToUtf8().c_str());
            continue;
        }
        // The class and each cooked super class: a native class has no bytecode.
        for (rt::UStruct* Struct = Class; Struct != nullptr && Struct->GetPathName().ToUtf8().starts_with("/Game/"); Struct = Struct->GetSuperStruct()) {
            std::printf("class %s : %s\n", Struct->GetPathName().ToUtf8().c_str(),
                        Struct->GetSuperStruct() != nullptr ? Struct->GetSuperStruct()->GetPathName().ToUtf8().c_str() : "none");
            for (rt::UField* Field = Struct->Children; Field != nullptr; Field = Field->Next)
                if (auto* Function = rt::Cast<rt::UFunction>(Field))
                    PrintFunction(*Function);
        }
    }
    return 0;
}

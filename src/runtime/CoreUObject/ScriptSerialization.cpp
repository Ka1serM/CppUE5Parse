#include <cstring>

#include "CoreUObjectPrivate.h"

#include "ue5parse/package/Package.h"
#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/Script.h"

// UStruct::SerializeExpr on load: turns cooked bytecode into the memory format the VM runs. Object and property
// operands become 8-byte pointers to runtime objects, names 12-byte slots, everything else is copied, so that the
// offsets the cooker computed (jumps, skips) stay valid.

namespace ue5parse::runtime {

class FScriptLinker {
public:
    FScriptLinker(UStruct& InStruct, FLinkerLoad& InLinker)
        : Struct(InStruct), Linker(InLinker),
          Reader(InLinker, std::span<const std::byte>(reinterpret_cast<const std::byte*>(InStruct.SerializedScript.GetData()),
                                                      static_cast<std::size_t>(InStruct.SerializedScript.Num())),
                 0) {}

    bool Link() {
        TArray<uint8>& Script = Struct.Script;
        Script.Reset();
        Script.Reserve(Struct.BytecodeBufferSize);
        while (Reader.GetRemaining() > 0 && !Reader.IsError() && !bFailed) SerializeExpr(0);
        if (Reader.IsError() || bFailed) return false;
        if (Script.Num() != Struct.BytecodeBufferSize) {
            ReportLoadProblem("script.size", Struct.GetPathName());
            return false;
        }
        return true;
    }

private:
    template <class T>
    T Xfer() {
        T Value = Reader.Read<T>();
        Append(&Value, sizeof(T));
        return Value;
    }
    void Append(const void* Data, std::size_t Size) {
        TArray<uint8>& Script = Struct.Script;
        int32 At = Script.AddUninitialized(static_cast<int32>(Size));
        std::memcpy(Script.GetData() + At, Data, Size);
    }
    void AppendPointer(const void* Pointer) {
        ScriptPointerType Value = ToScriptPointer(Pointer);
        Append(&Value, sizeof(Value));
    }
    void XferName() {
        FName Name;
        Reader << Name;
        int32 Index = Struct.ScriptNames.Find(Name);
        if (Index == INDEX_NONE) Index = Struct.ScriptNames.Add(Name);
        uint8 Slot[ScriptNameSize] = {};
        uint32 Stored = static_cast<uint32>(Index);
        std::memcpy(Slot, &Stored, sizeof(Stored));
        Append(Slot, sizeof(Slot));
    }
    UObject* XferObject() {
        FPackageIndex Index = Reader.ReadPackageIndex();
        UObject* Object = Linker.IndexToObject(Index);
        if (Object == nullptr && !Index.IsNull())
            ReportLoadProblem("script.object_missing", Struct.GetPathName() + FString(": ") + FString(Linker.GetPackage().resolve(Index.Index).getPathName()));
        AppendPointer(Object);
        return Object;
    }
    // A property pointer: its field path (names, then the owning struct).
    void XferProperty() {
        TArray<FName> Path;
        int32 Count = Reader.ReadCount(8);
        for (int32 i = 0; i < Count && !Reader.IsError(); i++) {
            FName Name;
            Reader << Name;
            Path.Add(Name);
        }
        FPackageIndex OwnerIndex = Reader.ReadPackageIndex();
        FField* Field = nullptr;
        if (Path.Num() > 0 && !(Path.Num() == 1 && Path[0].isNone())) {
            UStruct* Owner = Cast<UStruct>(Linker.IndexToObject(OwnerIndex));
            if (Owner != nullptr) {
                Field = Owner->FindPropertyByName(Path.Last());
                for (int32 i = Path.Num() - 2; Field != nullptr && i >= 0; i--) {
                    FField* Inner = nullptr;
                    if (auto* Array = CastField<FArrayProperty>(Field)) Inner = Array->Inner;
                    else if (auto* Set = CastField<FSetProperty>(Field)) Inner = Set->ElementProp;
                    else if (auto* Map = CastField<FMapProperty>(Field)) Inner = Map->KeyProp->GetFName() == Path[i] ? Map->KeyProp : Map->ValueProp;
                    else if (auto* Enum = CastField<FEnumProperty>(Field)) Inner = Enum->UnderlyingProp;
                    Field = Inner;
                }
            }
            if (Field == nullptr) {
                ReportLoadProblem("script.property_missing", Struct.GetPathName() + FString(": ") + FString(Linker.GetPackage().resolve(OwnerIndex.Index).getPathName()) +
                                                                 FString(":") + NameToString(Path.Last()));
            }
        }
        AppendPointer(Field);
    }
    void XferString(bool bWide) {
        if (bWide) {
            uint16 Char;
            do Char = Xfer<uint16>();
            while (Char != 0 && !Reader.IsError());
        } else {
            uint8 Char;
            do Char = Xfer<uint8>();
            while (Char != 0 && !Reader.IsError());
        }
    }
    // Parameters up to and including the end token.
    void XferList(EExprToken EndToken, int32 Depth) {
        while (!Reader.IsError() && !bFailed && SerializeExpr(Depth + 1) != EndToken) {}
    }

    EExprToken SerializeExpr(int32 Depth);

    UStruct& Struct;
    FLinkerLoad& Linker;
    FExportArchive Reader;
    bool bFailed = false;
};

EExprToken FScriptLinker::SerializeExpr(int32 Depth) {
    if (Depth > 512) {
        bFailed = true;
        return EX_Max;
    }
    EExprToken Expr = static_cast<EExprToken>(Xfer<uint8>());
    switch (Expr) {
    case EX_Cast:
        Xfer<uint8>();
        SerializeExpr(Depth + 1);
        break;
    case EX_ObjToInterfaceCast:
    case EX_CrossInterfaceCast:
    case EX_InterfaceToObjCast:
        XferObject();
        SerializeExpr(Depth + 1);
        break;
    case EX_Let:
        XferProperty();
        SerializeExpr(Depth + 1);
        SerializeExpr(Depth + 1);
        break;
    case EX_LetObj:
    case EX_LetWeakObjPtr:
    case EX_LetBool:
    case EX_LetDelegate:
    case EX_LetMulticastDelegate:
        SerializeExpr(Depth + 1);
        SerializeExpr(Depth + 1);
        break;
    case EX_LetValueOnPersistentFrame:
        XferProperty();
        SerializeExpr(Depth + 1);
        break;
    case EX_StructMemberContext:
        XferProperty();
        SerializeExpr(Depth + 1);
        break;
    case EX_Jump:
    case EX_PushExecutionFlow:
    case EX_SkipOffsetConst:
        Xfer<CodeSkipSizeType>();
        break;
    case EX_ComputedJump:
    case EX_InterfaceContext:
    case EX_Return:
    case EX_ClearMulticastDelegate:
    case EX_SoftObjectConst:
    case EX_FieldPathConst:
    case EX_PopExecutionFlowIfNot:
    case EX_AutoRtfmAbortIfNot:
        SerializeExpr(Depth + 1);
        break;
    case EX_LocalVariable:
    case EX_InstanceVariable:
    case EX_DefaultVariable:
    case EX_LocalOutVariable:
    case EX_ClassSparseDataVariable:
    case EX_PropertyConst:
        XferProperty();
        break;
    case EX_NothingInt32:
    case EX_IntConst:
        Xfer<int32>();
        break;
    case EX_Nothing:
    case EX_EndOfScript:
    case EX_EndFunctionParms:
    case EX_EndStructConst:
    case EX_EndArray:
    case EX_EndArrayConst:
    case EX_EndSet:
    case EX_EndMap:
    case EX_EndSetConst:
    case EX_EndMapConst:
    case EX_IntZero:
    case EX_IntOne:
    case EX_True:
    case EX_False:
    case EX_NoObject:
    case EX_NoInterface:
    case EX_Self:
    case EX_EndParmValue:
    case EX_PopExecutionFlow:
    case EX_DeprecatedOp4A:
    case EX_WireTracepoint:
    case EX_Tracepoint:
        break;
    case EX_Breakpoint:
        // A loaded breakpoint runs as a tracepoint.
        Struct.Script[Struct.Script.Num() - 1] = EX_Tracepoint;
        break;
    case EX_InstrumentationEvent:
        Xfer<uint8>();
        break;
    case EX_CallMath:
    case EX_LocalFinalFunction:
    case EX_FinalFunction:
    case EX_CallMulticastDelegate:
        XferObject();
        XferList(EX_EndFunctionParms, Depth);
        break;
    case EX_LocalVirtualFunction:
    case EX_VirtualFunction:
        XferName();
        XferList(EX_EndFunctionParms, Depth);
        break;
    case EX_ClassContext:
    case EX_Context:
    case EX_Context_FailSilent:
        SerializeExpr(Depth + 1);
        Xfer<CodeSkipSizeType>();
        XferProperty();
        SerializeExpr(Depth + 1);
        break;
    case EX_AddMulticastDelegate:
    case EX_RemoveMulticastDelegate:
    case EX_ArrayGetByRef:
        SerializeExpr(Depth + 1);
        SerializeExpr(Depth + 1);
        break;
    case EX_Int64Const:
        Xfer<int64>();
        break;
    case EX_UInt64Const:
        Xfer<uint64>();
        break;
    case EX_FloatConst:
        Xfer<float>();
        break;
    case EX_DoubleConst:
        Xfer<double>();
        break;
    case EX_StringConst:
        XferString(false);
        break;
    case EX_UnicodeStringConst:
        XferString(true);
        break;
    case EX_TextConst: {
        // UE 5.8 renumbered EBlueprintTextLiteralType; the runtime runs the 5.0-5.7 values.
        uint8 Stored = Reader.Read<uint8>();
        uint8 Type = Stored <= 4 ? Stored : 0;
        Append(&Type, 1);
        switch (static_cast<EBlueprintTextLiteralType>(Type)) {
        case EBlueprintTextLiteralType::Empty:
            break;
        case EBlueprintTextLiteralType::LocalizedText:
            SerializeExpr(Depth + 1);
            SerializeExpr(Depth + 1);
            SerializeExpr(Depth + 1);
            break;
        case EBlueprintTextLiteralType::InvariantText:
        case EBlueprintTextLiteralType::LiteralString:
            SerializeExpr(Depth + 1);
            break;
        case EBlueprintTextLiteralType::StringTableEntry:
            XferObject();
            SerializeExpr(Depth + 1);
            SerializeExpr(Depth + 1);
            break;
        }
        break;
    }
    case EX_ObjectConst:
        XferObject();
        break;
    case EX_NameConst:
        XferName();
        break;
    case EX_RotationConst:
    case EX_VectorConst:
        Xfer<double>();
        Xfer<double>();
        Xfer<double>();
        break;
    case EX_Vector3fConst:
        Xfer<float>();
        Xfer<float>();
        Xfer<float>();
        break;
    case EX_TransformConst:
        for (int i = 0; i < 10; i++) Xfer<double>();
        break;
    case EX_StructConst:
        XferObject();
        Xfer<int32>();
        XferList(EX_EndStructConst, Depth);
        break;
    case EX_SetArray:
        SerializeExpr(Depth + 1);
        XferList(EX_EndArray, Depth);
        break;
    case EX_SetSet:
        SerializeExpr(Depth + 1);
        Xfer<int32>();
        XferList(EX_EndSet, Depth);
        break;
    case EX_SetMap:
        SerializeExpr(Depth + 1);
        Xfer<int32>();
        XferList(EX_EndMap, Depth);
        break;
    case EX_ArrayConst:
        XferProperty();
        Xfer<int32>();
        XferList(EX_EndArrayConst, Depth);
        break;
    case EX_SetConst:
        XferProperty();
        Xfer<int32>();
        XferList(EX_EndSetConst, Depth);
        break;
    case EX_MapConst:
        XferProperty();
        XferProperty();
        Xfer<int32>();
        XferList(EX_EndMapConst, Depth);
        break;
    case EX_BitFieldConst:
        XferProperty();
        Xfer<uint8>();
        break;
    case EX_ByteConst:
    case EX_IntConstByte:
        Xfer<uint8>();
        break;
    case EX_MetaCast:
    case EX_DynamicCast:
        XferObject();
        SerializeExpr(Depth + 1);
        break;
    case EX_JumpIfNot:
    case EX_Skip:
        Xfer<CodeSkipSizeType>();
        SerializeExpr(Depth + 1);
        break;
    case EX_Assert:
        Xfer<uint16>();
        Xfer<uint8>();
        SerializeExpr(Depth + 1);
        break;
    case EX_InstanceDelegate:
        XferName();
        break;
    case EX_BindDelegate:
        XferName();
        SerializeExpr(Depth + 1);
        SerializeExpr(Depth + 1);
        break;
    case EX_SwitchValue: {
        uint16 NumCases = Xfer<uint16>();
        Xfer<CodeSkipSizeType>();
        SerializeExpr(Depth + 1);
        for (uint16 CaseIndex = 0; CaseIndex < NumCases && !Reader.IsError(); CaseIndex++) {
            SerializeExpr(Depth + 1);
            Xfer<CodeSkipSizeType>();
            SerializeExpr(Depth + 1);
        }
        SerializeExpr(Depth + 1);
        break;
    }
    case EX_AutoRtfmTransact:
        Xfer<int32>();
        Xfer<CodeSkipSizeType>();
        XferList(EX_AutoRtfmStopTransact, Depth);
        break;
    case EX_AutoRtfmStopTransact:
        Xfer<int32>();
        Xfer<int8>();
        break;
    default:
        ReportLoadProblem("script.token", Struct.GetPathName());
        bFailed = true;
        break;
    }
    return Expr;
}

bool UFunction::EnsureScriptLinked() {
    if (bScriptLinked) return Script.Num() > 0 || SerializedScript.Num() == 0;
    std::lock_guard Lock(GetLoaderMutex());
    if (bScriptLinked) return Script.Num() > 0 || SerializedScript.Num() == 0;
    bool bOk = true;
    if (SerializedScript.Num() > 0 && GetLinker() != nullptr) {
        FScriptLinker ScriptLinker(*this, *GetLinker());
        bOk = ScriptLinker.Link();
        if (!bOk) Script.Empty();
    }
    bScriptLinked = true;
    return bOk;
}

}

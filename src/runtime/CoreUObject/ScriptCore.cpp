#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "CoreUObjectPrivate.h"

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/ScriptMacros.h"
#include "ue5parse/runtime/CoreUObject/UObject/Stack.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Core/Math/UnrealMath.h"

// The Kismet VM: frames, function calls, ProcessEvent and the exec function of every opcode (UE's ScriptCore.cpp).

namespace ue5parse::runtime {

FNativeFuncPtr GNatives[EX_Max];
FNativeFuncPtr GCasts[CST_Max];
int32 GMaximumScriptLoopIterations = 1000000;
int32 GScriptRecurseLimit = 120;

namespace {

thread_local FFrame* GTopTrackingStackFrame = nullptr;

// UE's FBlueprintContextTracker: the runaway counters of this thread.
struct FBlueprintContextTracker {
    int32 Runaway = 0;
    int32 Recurse = 0;
    bool bRanaway = false;
    int32 ScriptEntryTag = 0;
};

thread_local FBlueprintContextTracker GContextTracker;

#define CHECK_RUNAWAY ++GContextTracker.Runaway;

FScriptExceptionHandler GScriptExceptionHandler = nullptr;

// The frames' parameter and local memory: a per-thread stack (UE's alloca or virtual stack allocator).
class FFrameAllocator {
public:
    struct FMark {
        int32 Chunk;
        std::size_t Used;
    };

    uint8* Allocate(std::size_t Size, std::size_t Alignment) {
        if (Size == 0) Size = 1;
        Alignment = std::max<std::size_t>(Alignment, 16);
        for (;;) {
            if (Current < Chunks.size()) {
                FChunk& Chunk = Chunks[Current];
                std::size_t Start = (Chunk.Used + Alignment - 1) / Alignment * Alignment;
                if (Start + Size <= Chunk.Size) {
                    Chunk.Used = Start + Size;
                    return Chunk.Memory.get() + Start;
                }
                if (Current + 1 < Chunks.size()) {
                    Current++;
                    Chunks[Current].Used = 0;
                    continue;
                }
            }
            std::size_t ChunkSize = std::max<std::size_t>(Size + Alignment, 1 << 20);
            Chunks.push_back(FChunk{std::unique_ptr<uint8[]>(new uint8[ChunkSize]), ChunkSize, 0});
            Current = Chunks.size() - 1;
        }
    }
    FMark GetMark() const { return FMark{static_cast<int32>(Current), Current < Chunks.size() ? Chunks[Current].Used : 0}; }
    void Reset(FMark Mark) {
        Current = static_cast<std::size_t>(Mark.Chunk);
        if (Current < Chunks.size()) Chunks[Current].Used = Mark.Used;
    }

private:
    struct FChunk {
        std::unique_ptr<uint8[]> Memory;
        std::size_t Size;
        std::size_t Used;
    };
    std::vector<FChunk> Chunks;
    std::size_t Current = 0;
};

thread_local FFrameAllocator GFrameAllocator;

class FFrameAllocatorScope {
public:
    FFrameAllocatorScope() : Mark(GFrameAllocator.GetMark()) {}
    ~FFrameAllocatorScope() { GFrameAllocator.Reset(Mark); }

private:
    FFrameAllocator::FMark Mark;
};

uint8* FrameAlloc(std::size_t Size, std::size_t Alignment = 16) { return GFrameAllocator.Allocate(Size, Alignment); }

void ClearReturnValue(FProperty* ReturnProp, RESULT_DECL) {
    if (ReturnProp == nullptr || RESULT_PARAM == nullptr) return;
    uint8* Data = static_cast<uint8*>(RESULT_PARAM);
    for (int32 i = 0; i < ReturnProp->ArrayDim; i++, Data += ReturnProp->ElementSize) {
        ReturnProp->DestroyValue(Data);
        ReturnProp->ClearValue(Data);
    }
}

void ThrowAccessViolation(const UObject* Object, FFrame& Stack, const char* Code, const FString& Message) {
    std::string Text = Message.ToUtf8();
    FBlueprintCoreDelegates::ThrowScriptException(Object, Stack, FBlueprintExceptionInfo{EBlueprintExceptionType::AccessViolation, Code, Text});
}

void ThrowNonFatal(const UObject* Object, FFrame& Stack, const char* Code, const FString& Message) {
    std::string Text = Message.ToUtf8();
    FBlueprintCoreDelegates::ThrowScriptException(Object, Stack, FBlueprintExceptionInfo{EBlueprintExceptionType::NonFatalError, Code, Text});
}

FString PropertyNameSafe(const FProperty* Property) { return Property ? Property->GetName() : FString("None"); }

}

void FBlueprintCoreDelegates::ThrowScriptException(const UObject* ActiveObject, const FFrame& StackFrame, const FBlueprintExceptionInfo& Info) {
    if (GScriptExceptionHandler != nullptr) GScriptExceptionHandler(ActiveObject, StackFrame, Info);
    if (Info.Type == EBlueprintExceptionType::AbortExecution) const_cast<FFrame&>(StackFrame).bAbortingExecution = true;
}

void FBlueprintCoreDelegates::SetScriptExceptionHandler(FScriptExceptionHandler Handler) { GScriptExceptionHandler = Handler; }

// Resets the runaway-loop counters; the world calls it each frame (UE's GInitRunaway).
void GInitRunaway() {
    GContextTracker.Runaway = 0;
    GContextTracker.Recurse = 0;
    GContextTracker.bRanaway = false;
}

FFrame::FFrame(UObject* InObject, UFunction* InNode, void* InLocals, FFrame* InPreviousFrame, FField* InPropertyChainForCompiledIn)
    : Node(InNode), Object(InObject), Code(InNode->Script.GetData()), Locals(static_cast<uint8*>(InLocals)),
      PreviousFrame(InPreviousFrame), PropertyChainForCompiledIn(InPropertyChainForCompiledIn) {
    PreviousTrackingFrame = GTopTrackingStackFrame;
    GTopTrackingStackFrame = this;
}

FFrame::~FFrame() { GTopTrackingStackFrame = PreviousTrackingFrame; }

FFrame* FFrame::GetThreadLocalTopStackFrame() { return GTopTrackingStackFrame; }

FName FFrame::ReadName() {
    uint32 Index;
    std::memcpy(&Index, Code, sizeof(Index));
    Code += ScriptNameSize;
    return Node->ScriptNames.IsValidIndex(static_cast<int32>(Index)) ? Node->ScriptNames[static_cast<int32>(Index)] : FName();
}

VariableSizeType FFrame::ReadVariableSize(FProperty** ExpressionField) {
    FProperty* Property = CastField<FProperty>(ReadField());
    if (ExpressionField != nullptr) *ExpressionField = Property;
    return Property ? static_cast<VariableSizeType>(Property->GetSize()) : 0;
}

void FFrame::StepExplicitProperty(void* const Result, FProperty* Property) {
    if (Property == nullptr) {
        KismetExecutionMessage("MissingParameter", "a native function was called without its parameter properties",
                               EBlueprintExceptionType::NonFatalError);
        return;
    }
    if (Property->PropertyFlags & CPF_OutParm) {
        FOutParmRec* Out = OutParms;
        while (Out != nullptr && Out->Property != Property) Out = Out->NextOutParm;
        MostRecentPropertyAddress = Out ? Out->PropAddr : nullptr;
        MostRecentPropertyContainer = nullptr;
    } else {
        MostRecentPropertyAddress = Property->ContainerPtrToValuePtr<uint8>(Locals);
        MostRecentPropertyContainer = Locals;
        Property->CopyCompleteValueToScriptVM_InContainer(Result, MostRecentPropertyContainer);
    }
}

FString FFrame::GetStackTrace() const {
    FString Result;
    for (const FFrame* Frame = this; Frame != nullptr; Frame = Frame->PreviousFrame) {
        if (!Result.IsEmpty()) Result += u" <- ";
        Result += Frame->Node ? Frame->Node->GetPathName() : FString("?");
    }
    return Result;
}

void FFrame::KismetExecutionMessage(const char* InCode, std::string_view Message, EBlueprintExceptionType Type) const {
    FBlueprintCoreDelegates::ThrowScriptException(Object, *this, FBlueprintExceptionInfo{Type, InCode, Message});
}

void UObject::SkipFunction(FFrame& Stack, RESULT_DECL, UFunction* Function) {
    FFrameAllocatorScope Scope;
    uint8* Frame = FrameAlloc(static_cast<std::size_t>(Function->PropertiesSize), static_cast<std::size_t>(Function->GetMinAlignment()));
    std::memset(Frame, 0, static_cast<std::size_t>(Function->PropertiesSize));
    for (FProperty* Property = CastField<FProperty>(Function->ChildProperties); *Stack.Code != EX_EndFunctionParms;
         Property = Property ? Property->GetNextProperty() : nullptr) {
        Stack.MostRecentPropertyAddress = nullptr;
        Stack.MostRecentPropertyContainer = nullptr;
        bool bOut = Property && (Property->PropertyFlags & CPF_OutParm);
        Stack.Step(Stack.Object, bOut || Property == nullptr ? nullptr : Property->ContainerPtrToValuePtr<uint8>(Frame));
    }
    Stack.Code++;
    for (FProperty* Destruct = Function->DestructorLink; Destruct != nullptr; Destruct = Destruct->DestructorLinkNext)
        if (!Destruct->HasAnyPropertyFlags(CPF_OutParm)) Destruct->DestroyValue_InContainer(Frame);
    FProperty* ReturnProp = Function->GetReturnProperty();
    if (ReturnProp != nullptr && RESULT_PARAM != nullptr) {
        ReturnProp->DestroyValue(RESULT_PARAM);
        std::memset(RESULT_PARAM, 0, static_cast<std::size_t>(ReturnProp->ArrayDim) * ReturnProp->ElementSize);
    }
}

namespace {

// Sets up a bytecode function's frame from the caller's parameter expressions and runs it with ExecFtor (UE's
// ProcessScriptFunction).
template <class Exec>
void ProcessScriptFunction(UObject* Context, UFunction* Function, FFrame& Stack, RESULT_DECL, Exec ExecFtor) {
    Function->EnsureScriptLinked();
    FFrameAllocatorScope Scope;
    FFrame NewStack(Context, Function, nullptr, &Stack, Function->ChildProperties);
    uint8* FrameMemory = Function->GetOuterUClass() ? Function->GetOuterUClass()->GetPersistentUberGraphFrame(Context, Function) : nullptr;
    bool bUsePersistentFrame = FrameMemory != nullptr;
    if (!bUsePersistentFrame) {
        FrameMemory = FrameAlloc(static_cast<std::size_t>(Function->PropertiesSize), static_cast<std::size_t>(Function->GetMinAlignment()));
        if (Function->PropertiesSize) std::memset(FrameMemory, 0, static_cast<std::size_t>(Function->PropertiesSize));
    }
    if (Function->ReturnValueOffset != 0xffff) {
        FProperty* ReturnProperty = Function->GetReturnProperty();
        if (ReturnProperty != nullptr) {
            auto* RetVal = reinterpret_cast<FOutParmRec*>(FrameAlloc(sizeof(FOutParmRec)));
            RetVal->PropAddr = static_cast<uint8*>(RESULT_PARAM);
            RetVal->Property = ReturnProperty;
            RetVal->NextOutParm = nullptr;
            NewStack.OutParms = RetVal;
        }
    }
    NewStack.Locals = FrameMemory;
    FOutParmRec** LastOut = &NewStack.OutParms;
    if (*LastOut != nullptr) LastOut = &(*LastOut)->NextOutParm;
    for (FProperty* Property = CastField<FProperty>(Function->ChildProperties); *Stack.Code != EX_EndFunctionParms;
         Property = Property ? Property->GetNextProperty() : nullptr) {
        Stack.MostRecentPropertyAddress = nullptr;
        Stack.MostRecentPropertyContainer = nullptr;
        if (Property == nullptr) {
            // More arguments than parameters: corrupt data; evaluate and drop them.
            alignas(16) uint8 Scratch[1024] = {};
            Stack.Step(Stack.Object, Scratch);
            continue;
        }
        if (Property->PropertyFlags & CPF_ReturnParm) continue;
        if (Property->PropertyFlags & CPF_OutParm) {
            Stack.Step(Stack.Object, nullptr);
            auto* Out = reinterpret_cast<FOutParmRec*>(FrameAlloc(sizeof(FOutParmRec)));
            // An optional out parameter without a variable uses the function's own memory.
            Out->PropAddr = Stack.MostRecentPropertyAddress != nullptr ? Stack.MostRecentPropertyAddress
                                                                        : Property->ContainerPtrToValuePtr<uint8>(NewStack.Locals);
            Out->Property = Property;
            Out->NextOutParm = nullptr;
            *LastOut = Out;
            LastOut = &Out->NextOutParm;
        } else {
            uint8* Param = Property->ContainerPtrToValuePtr<uint8>(NewStack.Locals);
            Property->InitializeValue_InContainer(NewStack.Locals);
            Stack.Step(Stack.Object, Param);
        }
    }
    Stack.Code++;
    if (!bUsePersistentFrame) {
        for (FProperty* LocalProp = Function->FirstPropertyToInit; LocalProp != nullptr; LocalProp = LocalProp->PostConstructLinkNext)
            LocalProp->InitializeValue_InContainer(NewStack.Locals);
    }
    if (Function->Script.Num() > 0) ExecFtor(Context, NewStack, RESULT_PARAM);
    if (!bUsePersistentFrame) {
        for (FProperty* Destruct = Function->DestructorLink; Destruct != nullptr; Destruct = Destruct->DestructorLinkNext)
            if (!Destruct->HasAnyPropertyFlags(CPF_OutParm)) Destruct->DestroyValue_InContainer(NewStack.Locals);
    }
    Stack.bAbortingExecution |= NewStack.bAbortingExecution;
}

void ProcessLocalFunction(UObject* Context, UFunction* Fn, FFrame& Stack, RESULT_DECL) {
    if (Fn == nullptr) {
        UObject::execUndefined(Context, Stack, RESULT_PARAM);
        return;
    }
    if (Fn->HasAnyFunctionFlags(FUNC_Native)) {
        Stack.CurrentNativeFunction = Fn;
        Fn->Invoke(Context, Stack, RESULT_PARAM);
    } else {
        ProcessScriptFunction(Context, Fn, Stack, RESULT_PARAM, ProcessLocalScriptFunction);
    }
}

}

void ProcessLocalScriptFunction(UObject* Context, FFrame& Stack, RESULT_DECL) {
    UFunction* Function = Stack.Node;
    alignas(16) uint8 Buffer[MAX_SIMPLE_RETURN_VALUE_SIZE];
    FBlueprintContextTracker& Tracker = GContextTracker;
    if (Tracker.bRanaway) {
        ClearReturnValue(Function->GetReturnProperty(), RESULT_PARAM);
        return;
    }
    if (++Tracker.Recurse == GScriptRecurseLimit) {
        ClearReturnValue(Function->GetReturnProperty(), RESULT_PARAM);
        ThrowNonFatal(Context, Stack, "InfiniteLoop", FString("Infinite script recursion detected"));
        Tracker.bRanaway = true;
        return;
    }
    while (*Stack.Code != EX_Return && !Stack.bAbortingExecution) {
        if (Tracker.Runaway > GMaximumScriptLoopIterations) {
            ClearReturnValue(Function->GetReturnProperty(), RESULT_PARAM);
            Tracker.Runaway = 0;
            FBlueprintCoreDelegates::ThrowScriptException(Context, Stack,
                                                          FBlueprintExceptionInfo{EBlueprintExceptionType::InfiniteLoop, "RunawayLoop", "Runaway loop detected"});
            return;
        }
        Stack.Step(Stack.Object, Buffer);
    }
    if (!Stack.bAbortingExecution) {
        Stack.Code++;
        if (*Stack.Code != EX_Nothing) {
            Stack.Step(Stack.Object, RESULT_PARAM);
        } else {
            Stack.Code++;
        }
    } else {
        ClearReturnValue(Function->GetReturnProperty(), RESULT_PARAM);
    }
    --Tracker.Recurse;
}

DEFINE_FUNCTION(UObject::ProcessInternal) {
    UFunction* Function = Stack.Node;
    int32 FunctionCallspace = P_THIS->GetFunctionCallspace(Function, nullptr);
    if (FunctionCallspace & FunctionCallspace::Remote) P_THIS->CallRemoteFunction(Function, Stack.Locals, Stack.OutParms, nullptr);
    if (FunctionCallspace & FunctionCallspace::Local) {
        ProcessLocalScriptFunction(Context, Stack, RESULT_PARAM);
    } else {
        ClearReturnValue(Function->GetReturnProperty(), RESULT_PARAM);
    }
}

void UObject::CallFunction(FFrame& Stack, RESULT_DECL, UFunction* Function) {
    if (Function == nullptr) {
        execUndefined(this, Stack, RESULT_PARAM);
        return;
    }
    if (Function->FunctionFlags & FUNC_Native) {
        bool bNetFunction = Function->HasAnyFunctionFlags(FUNC_NetFuncFlags | FUNC_BlueprintAuthorityOnly | FUNC_BlueprintCosmetic |
                                                          FUNC_NetRequest | FUNC_NetResponse);
        int32 FunctionCallspace = bNetFunction ? GetFunctionCallspace(Function, &Stack) : FunctionCallspace::Local;
        uint8* SavedCode = nullptr;
        if (FunctionCallspace & FunctionCallspace::Remote) {
            FFrameAllocatorScope Scope;
            uint8* Buffer = FrameAlloc(Function->ParmsSize, static_cast<std::size_t>(Function->GetMinAlignment()));
            SavedCode = Stack.Code;
            std::memset(Buffer, 0, Function->ParmsSize);
            for (FProperty* Property = CastField<FProperty>(Function->ChildProperties);
                 Property && (Property->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm; Property = Property->GetNextProperty()) {
                uint8* CurrentPropAddr = Property->ContainerPtrToValuePtr<uint8>(Buffer);
                if (FBoolProperty* Bool = CastField<FBoolProperty>(Property); Bool && Bool->ArrayDim == 1) {
                    bool bValue = false;
                    Stack.Step(Stack.Object, &bValue);
                    if (bValue) Bool->SetPropertyValue(CurrentPropAddr, true);
                } else {
                    Stack.Step(Stack.Object, CurrentPropAddr);
                }
            }
            CallRemoteFunction(Function, Buffer, Stack.OutParms, &Stack);
        }
        if (FunctionCallspace & FunctionCallspace::Local) {
            if (SavedCode) Stack.Code = SavedCode;
            Stack.CurrentNativeFunction = Function;
            Function->Invoke(this, Stack, RESULT_PARAM);
        } else {
            SkipFunction(Stack, RESULT_PARAM, Function);
        }
    } else {
        ProcessScriptFunction(this, Function, Stack, RESULT_PARAM, ProcessInternal);
    }
}

void UObject::ProcessEvent(UFunction* Function, void* Parms) {
    if (!IsValid(this) || Function == nullptr) return;
    if (Function->FunctionFlags & FUNC_Native) {
        int32 FunctionCallspace = GetFunctionCallspace(Function, nullptr);
        if (FunctionCallspace & FunctionCallspace::Remote) CallRemoteFunction(Function, Parms, nullptr, nullptr);
        if ((FunctionCallspace & FunctionCallspace::Local) == 0) return;
    } else {
        Function->EnsureScriptLinked();
        if (Function->Script.Num() == 0) return;
    }
    GContextTracker.ScriptEntryTag++;
    int32 EventGraphParams;
    if (Function->EventGraphFunction != nullptr) {
        // Straight into the ubergraph, skipping the event's stub.
        EventGraphParams = Function->EventGraphCallOffset;
        Parms = &EventGraphParams;
        Function = Function->EventGraphFunction;
        Function->EnsureScriptLinked();
    }
    {
        uint8* Frame = nullptr;
        if (Function->HasAnyFunctionFlags(FUNC_UbergraphFunction) && Function->GetOuterUClass())
            Frame = Function->GetOuterUClass()->GetPersistentUberGraphFrame(this, Function);
        FFrameAllocatorScope Scope;
        bool bUsePersistentFrame = Frame != nullptr;
        if (!bUsePersistentFrame) {
            Frame = FrameAlloc(static_cast<std::size_t>(Function->PropertiesSize), static_cast<std::size_t>(Function->GetMinAlignment()));
            int32 NonParmsPropertiesSize = Function->PropertiesSize - Function->ParmsSize;
            if (NonParmsPropertiesSize) std::memset(Frame + Function->ParmsSize, 0, static_cast<std::size_t>(NonParmsPropertiesSize));
        }
        if (Function->ParmsSize) std::memcpy(Frame, Parms, Function->ParmsSize);
        FFrame NewStack(this, Function, Frame, nullptr, Function->ChildProperties);
        if (Function->HasAnyFunctionFlags(FUNC_HasOutParms)) {
            FOutParmRec** LastOut = &NewStack.OutParms;
            for (FProperty* Property = CastField<FProperty>(Function->ChildProperties);
                 Property && (Property->PropertyFlags & CPF_Parm) == CPF_Parm; Property = Property->GetNextProperty()) {
                if (!Property->HasAnyPropertyFlags(CPF_OutParm)) continue;
                auto* Out = reinterpret_cast<FOutParmRec*>(FrameAlloc(sizeof(FOutParmRec)));
                Out->PropAddr = Property->ContainerPtrToValuePtr<uint8>(Parms);
                Out->Property = Property;
                Out->NextOutParm = nullptr;
                *LastOut = Out;
                LastOut = &Out->NextOutParm;
            }
        }
        if (!bUsePersistentFrame) {
            for (FProperty* LocalProp = Function->FirstPropertyToInit; LocalProp != nullptr; LocalProp = LocalProp->PostConstructLinkNext)
                LocalProp->InitializeValue_InContainer(NewStack.Locals);
        }
        bool bHasReturnParam = Function->ReturnValueOffset != 0xffff;
        uint8* ReturnValueAddress = bHasReturnParam ? static_cast<uint8*>(Parms) + Function->ReturnValueOffset : nullptr;
        NewStack.CurrentNativeFunction = Function;
        Function->Invoke(this, NewStack, ReturnValueAddress);
        if (!bUsePersistentFrame) {
            // Locals are destroyed; value parameters are copied back, so that the caller destroys its copy once.
            for (FProperty* P = Function->DestructorLink; P != nullptr; P = P->DestructorLinkNext) {
                if (!P->IsInContainer(Function->ParmsSize)) {
                    P->DestroyValue_InContainer(NewStack.Locals);
                } else if (!(P->PropertyFlags & CPF_OutParm)) {
                    std::memcpy(P->ContainerPtrToValuePtr<uint8>(Parms), P->ContainerPtrToValuePtr<uint8>(NewStack.Locals),
                                static_cast<std::size_t>(P->ArrayDim) * P->ElementSize);
                }
            }
        }
    }
    GContextTracker.ScriptEntryTag--;
}

DEFINE_FUNCTION(UObject::execUndefined) {
    std::string Message = "Encountered an undefined opcode (0x";
    static const char Hex[] = "0123456789ABCDEF";
    uint8 Op = Stack.Code ? Stack.Code[-1] : 0;
    Message += Hex[Op >> 4];
    Message += Hex[Op & 15];
    Message += ")";
    Stack.KismetExecutionMessage("UndefinedOpcode", Message, EBlueprintExceptionType::FatalError);
    // The rest of this function cannot be decoded: stop it.
    Stack.bAbortingExecution = true;
}

DEFINE_FUNCTION(UObject::execLocalVariable) {
    FProperty* VarProperty = Stack.ReadProperty();
    if (VarProperty == nullptr) {
        ThrowAccessViolation(P_THIS, Stack, "MissingLocalVariable", FString("Attempted to access missing local variable."));
        Stack.MostRecentPropertyAddress = nullptr;
        Stack.MostRecentPropertyContainer = nullptr;
    } else {
        Stack.MostRecentPropertyAddress = VarProperty->ContainerPtrToValuePtr<uint8>(Stack.Locals);
        Stack.MostRecentPropertyContainer = Stack.Locals;
        if (RESULT_PARAM) VarProperty->CopyCompleteValueToScriptVM_InContainer(RESULT_PARAM, Stack.MostRecentPropertyContainer);
    }
}

DEFINE_FUNCTION(UObject::execInstanceVariable) {
    FProperty* VarProperty = reinterpret_cast<FProperty*>(Stack.ReadField());
    Stack.MostRecentProperty = VarProperty;
    UClass* OwnerClass = VarProperty ? VarProperty->GetOwnerClass() : nullptr;
    if (VarProperty == nullptr || (OwnerClass != nullptr && !P_THIS->IsA(OwnerClass))) {
        ThrowAccessViolation(P_THIS, Stack, "MissingProperty",
                             FString("Attempted to access missing property '") + PropertyNameSafe(VarProperty) + FString("'."));
        Stack.MostRecentPropertyAddress = nullptr;
        Stack.MostRecentPropertyContainer = nullptr;
    } else {
        Stack.MostRecentPropertyAddress = VarProperty->ContainerPtrToValuePtr<uint8>(P_THIS);
        Stack.MostRecentPropertyContainer = reinterpret_cast<uint8*>(P_THIS);
        if (RESULT_PARAM) VarProperty->CopyCompleteValueToScriptVM_InContainer(RESULT_PARAM, Stack.MostRecentPropertyContainer);
    }
}

DEFINE_FUNCTION(UObject::execClassSparseDataVariable) {
    // Sparse class data is not used by the cooked Blueprints the runtime reads; the access reads nothing.
    FProperty* VarProperty = reinterpret_cast<FProperty*>(Stack.ReadField());
    Stack.MostRecentProperty = VarProperty;
    ThrowAccessViolation(P_THIS, Stack, "MissingSparseProperty",
                         FString("Attempted to access missing sparse property '") + PropertyNameSafe(VarProperty) + FString("'."));
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
}

DEFINE_FUNCTION(UObject::execDefaultVariable) {
    FProperty* VarProperty = reinterpret_cast<FProperty*>(Stack.ReadField());
    Stack.MostRecentProperty = VarProperty;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    UObject* DefaultObject = P_THIS->HasAnyFlags(RF_ClassDefaultObject) ? P_THIS : nullptr;
    UClass* OwnerClass = VarProperty ? VarProperty->GetOwnerClass() : nullptr;
    if (VarProperty == nullptr || (DefaultObject && OwnerClass && !DefaultObject->IsA(OwnerClass))) {
        ThrowAccessViolation(P_THIS, Stack, "MissingPropertyDefaultObject", FString("Attempted to access a missing property on a CDO."));
    } else if (DefaultObject != nullptr) {
        Stack.MostRecentPropertyAddress = VarProperty->ContainerPtrToValuePtr<uint8>(DefaultObject);
        Stack.MostRecentPropertyContainer = reinterpret_cast<uint8*>(DefaultObject);
        if (RESULT_PARAM) VarProperty->CopyCompleteValueToScriptVM_InContainer(RESULT_PARAM, Stack.MostRecentPropertyContainer);
    } else {
        ThrowAccessViolation(P_THIS, Stack, "AccessNoneDefaultObject", FString("Accessed None attempting to read a default property"));
    }
}

DEFINE_FUNCTION(UObject::execLocalOutVariable) {
    FProperty* VarProperty = Stack.ReadProperty();
    FOutParmRec* Out = Stack.OutParms;
    while (Out != nullptr && Out->Property != VarProperty) Out = Out->NextOutParm;
    if (Out == nullptr) {
        ThrowAccessViolation(P_THIS, Stack, "MissingOutParameter", FString("Out parameter ") + PropertyNameSafe(VarProperty) + FString(" not found"));
        Stack.MostRecentPropertyAddress = nullptr;
        return;
    }
    Stack.MostRecentPropertyAddress = Out->PropAddr;
    if (RESULT_PARAM && RESULT_PARAM != Stack.MostRecentPropertyAddress)
        VarProperty->CopyCompleteValueToScriptVM(RESULT_PARAM, Stack.MostRecentPropertyAddress);
}

DEFINE_FUNCTION(UObject::execInterfaceContext) {
    FScriptInterface InterfaceValue;
    Stack.Step(P_THIS, &InterfaceValue);
    if (RESULT_PARAM != nullptr) *static_cast<UObject**>(RESULT_PARAM) = InterfaceValue.GetObject();
}

DEFINE_FUNCTION(UObject::execClassContext) {
    UClass* ClassContext = nullptr;
    Stack.Step(P_THIS, &ClassContext);
    if (IsValid(ClassContext)) {
        UObject* DefaultObject = ClassContext->GetDefaultObject();
        Stack.Code += sizeof(CodeSkipSizeType) + sizeof(ScriptPointerType);
        Stack.Step(DefaultObject, RESULT_PARAM);
    } else {
        if (Stack.MostRecentProperty != nullptr) {
            ThrowAccessViolation(P_THIS, Stack, "AccessedNoneClass",
                                 FString("Accessed None trying to read Class from property ") + Stack.MostRecentProperty->GetName());
        } else {
            ThrowAccessViolation(P_THIS, Stack, "AccessedNoneClassUnknownProperty", FString("Accessed None reading a Class"));
        }
        CodeSkipSizeType wSkip = Stack.ReadCodeSkipCount();
        FProperty* RValueProperty = nullptr;
        Stack.ReadVariableSize(&RValueProperty);
        Stack.Code += wSkip;
        Stack.MostRecentPropertyAddress = nullptr;
        Stack.MostRecentPropertyContainer = nullptr;
        Stack.MostRecentProperty = nullptr;
        if (RESULT_PARAM && RValueProperty) RValueProperty->ClearValue(RESULT_PARAM);
    }
}

DEFINE_FUNCTION(UObject::execEndOfScript) {
    Stack.KismetExecutionMessage("EndOfScript", "Execution beyond end of script", EBlueprintExceptionType::FatalError);
    Stack.bAbortingExecution = true;
}

DEFINE_FUNCTION(UObject::execNothing) {}
DEFINE_FUNCTION(UObject::execNothingInt32) { Stack.ReadInt<int32>(); }
DEFINE_FUNCTION(UObject::execNothingOp4a) {}
DEFINE_FUNCTION(UObject::execBreakpoint) {}
DEFINE_FUNCTION(UObject::execTracepoint) {}
DEFINE_FUNCTION(UObject::execWireTracepoint) {}
DEFINE_FUNCTION(UObject::execInstrumentation) { Stack.SkipCode(1); }
DEFINE_FUNCTION(UObject::execEndFunctionParms) { Stack.Code--; }

DEFINE_FUNCTION(UObject::execJump) {
    CHECK_RUNAWAY;
    CodeSkipSizeType Offset = Stack.ReadCodeSkipCount();
    Stack.Code = &Stack.Node->Script[static_cast<int32>(Offset)];
}

DEFINE_FUNCTION(UObject::execComputedJump) {
    CHECK_RUNAWAY;
    int32 ComputedOffset = 0;
    Stack.Step(Stack.Object, &ComputedOffset);
    if (ComputedOffset < 0 || ComputedOffset >= Stack.Node->Script.Num()) {
        Stack.KismetExecutionMessage("ComputedJump", "Computed jump out of the script", EBlueprintExceptionType::FatalError);
        Stack.bAbortingExecution = true;
        return;
    }
    Stack.Code = &Stack.Node->Script[ComputedOffset];
}

DEFINE_FUNCTION(UObject::execJumpIfNot) {
    CHECK_RUNAWAY;
    CodeSkipSizeType Offset = Stack.ReadCodeSkipCount();
    bool Value = false;
    Stack.Step(Stack.Object, &Value);
    if (!Value) Stack.Code = &Stack.Node->Script[static_cast<int32>(Offset)];
}

DEFINE_FUNCTION(UObject::execAssert) {
    Stack.ReadWord();
    Stack.Code++;
    uint32 Value = 0;
    Stack.Step(Stack.Object, &Value);
}

DEFINE_FUNCTION(UObject::execPushExecutionFlow) {
    CodeSkipSizeType Offset = Stack.ReadCodeSkipCount();
    Stack.FlowStack.Push(Offset);
}

DEFINE_FUNCTION(UObject::execPopExecutionFlow) {
    CHECK_RUNAWAY;
    if (Stack.FlowStack.Num()) {
        CodeSkipSizeType Offset = Stack.FlowStack.Pop();
        Stack.Code = &Stack.Node->Script[static_cast<int32>(Offset)];
    } else {
        Stack.KismetExecutionMessage("FlowStack", "Tried to pop from an empty flow stack", EBlueprintExceptionType::NonFatalError);
    }
}

DEFINE_FUNCTION(UObject::execPopExecutionFlowIfNot) {
    CHECK_RUNAWAY;
    bool Value = false;
    Stack.Step(Stack.Object, &Value);
    if (!Value) {
        if (Stack.FlowStack.Num()) {
            CodeSkipSizeType Offset = Stack.FlowStack.Pop();
            Stack.Code = &Stack.Node->Script[static_cast<int32>(Offset)];
        } else {
            Stack.KismetExecutionMessage("FlowStack", "Tried to pop from an empty flow stack", EBlueprintExceptionType::NonFatalError);
        }
    }
}

DEFINE_FUNCTION(UObject::execLetValueOnPersistentFrame) {
    Stack.MostRecentProperty = nullptr;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    FProperty* DestProperty = Stack.ReadProperty();
    UFunction* UberGraphFunction = DestProperty ? Cast<UFunction>(DestProperty->GetOwnerUObject()) : nullptr;
    UClass* OuterClass = UberGraphFunction ? UberGraphFunction->GetOuterUClass() : nullptr;
    uint8* FrameBase = OuterClass ? OuterClass->GetPersistentUberGraphFrame(Stack.Object, UberGraphFunction) : nullptr;
    if (FrameBase == nullptr) {
        alignas(16) uint8 Scratch[1024] = {};
        Stack.Step(Stack.Object, Scratch);
        ThrowAccessViolation(P_THIS, Stack, "PersistentFrame", FString("No persistent ubergraph frame for ") + PropertyNameSafe(DestProperty));
        return;
    }
    Stack.Step(Stack.Object, DestProperty->ContainerPtrToValuePtr<uint8>(FrameBase));
}

DEFINE_FUNCTION(UObject::execSwitchValue) {
    const int32 NumCases = Stack.ReadWord();
    const CodeSkipSizeType OffsetToEnd = Stack.ReadCodeSkipCount();
    Stack.MostRecentProperty = nullptr;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.Step(Stack.Object, nullptr);
    FProperty* IndexProperty = Stack.MostRecentProperty;
    uint8* IndexAddress = Stack.MostRecentPropertyAddress;
    if (IndexProperty == nullptr) {
        Stack.KismetExecutionMessage("SwitchValueIndex", "Switch statement without an index property", EBlueprintExceptionType::FatalError);
        Stack.bAbortingExecution = true;
        return;
    }
    if (IndexAddress == nullptr)
        ThrowNonFatal(P_THIS, Stack, "SwitchValueIndex",
                      FString("Switch statement failed to read property for index value for index property ") + IndexProperty->GetName());
    bool bProperCaseUsed = false;
    {
        FFrameAllocatorScope Scope;
        uint8* LocalTempIndexMem = FrameAlloc(static_cast<std::size_t>(IndexProperty->GetSize()), static_cast<std::size_t>(IndexProperty->GetMinAlignment()));
        std::memset(LocalTempIndexMem, 0, static_cast<std::size_t>(IndexProperty->GetSize()));
        IndexProperty->InitializeValue(LocalTempIndexMem);
        for (int32 CaseIndex = 0; CaseIndex < NumCases; ++CaseIndex) {
            Stack.Step(Stack.Object, LocalTempIndexMem);
            const CodeSkipSizeType OffsetToNextCase = Stack.ReadCodeSkipCount();
            if (IndexAddress && IndexProperty->Identical(IndexAddress, LocalTempIndexMem)) {
                Stack.Step(Stack.Object, RESULT_PARAM);
                bProperCaseUsed = true;
                break;
            }
            Stack.Code = &Stack.Node->Script[static_cast<int32>(OffsetToNextCase)];
        }
        IndexProperty->DestroyValue(LocalTempIndexMem);
    }
    if (bProperCaseUsed) {
        Stack.Code = &Stack.Node->Script[static_cast<int32>(OffsetToEnd)];
    } else {
        ThrowNonFatal(P_THIS, Stack, "SwitchValueOutOfBounds", FString("Switch statement failed to match case for index property ") + IndexProperty->GetName());
        Stack.Step(Stack.Object, RESULT_PARAM);
    }
}

DEFINE_FUNCTION(UObject::execArrayGetByRef) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.Step(Stack.Object, nullptr);
    if (Stack.MostRecentPropertyAddress == nullptr)
        ThrowAccessViolation(P_THIS, Stack, "ArrayGetRefException", FString("Attempt to assign variable through None"));
    void* ArrayAddr = Stack.MostRecentPropertyAddress;
    FArrayProperty* ArrayProperty = ExactCastField<FArrayProperty>(Stack.MostRecentProperty);
    int32 ArrayIndex = 0;
    Stack.Step(Stack.Object, &ArrayIndex);
    if (ArrayProperty == nullptr || ArrayAddr == nullptr) {
        Stack.bArrayContextFailed = true;
        return;
    }
    FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayAddr);
    Stack.MostRecentProperty = ArrayProperty->Inner;
    if (ArrayHelper.IsValidIndex(ArrayIndex)) {
        Stack.MostRecentPropertyAddress = ArrayHelper.GetRawPtr(ArrayIndex);
        Stack.MostRecentPropertyContainer = nullptr;
        if (RESULT_PARAM) ArrayProperty->Inner->CopyCompleteValueToScriptVM(RESULT_PARAM, ArrayHelper.GetRawPtr(ArrayIndex));
    } else {
        Stack.MostRecentPropertyAddress = nullptr;
        Stack.MostRecentPropertyContainer = nullptr;
        Stack.MostRecentProperty = nullptr;
        ThrowAccessViolation(P_THIS, Stack, "ArrayGetOutofBounds",
                             FString("Attempted to access index ") + FString(std::to_string(ArrayIndex)) + FString(" from array ") +
                                 ArrayProperty->GetName() + FString(" of length ") + FString(std::to_string(ArrayHelper.Num())) + FString("!"));
    }
}

DEFINE_FUNCTION(UObject::execLet) {
    Stack.MostRecentProperty = nullptr;
    FProperty* LocallyKnownProperty = Stack.ReadPropertyUnchecked();
    Stack.MostRecentProperty = nullptr;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.Step(Stack.Object, nullptr);
    FFrameAllocatorScope Scope;
    uint8* LocalTempResult = nullptr;
    if (Stack.MostRecentPropertyAddress == nullptr) {
        ThrowAccessViolation(P_THIS, Stack, "LetAccessNone", FString("Attempted to assign to None"));
        if (LocallyKnownProperty) {
            LocalTempResult = FrameAlloc(static_cast<std::size_t>(LocallyKnownProperty->GetSize()), static_cast<std::size_t>(LocallyKnownProperty->GetMinAlignment()));
            std::memset(LocalTempResult, 0, static_cast<std::size_t>(LocallyKnownProperty->GetSize()));
            LocallyKnownProperty->InitializeValue(LocalTempResult);
            Stack.MostRecentPropertyAddress = LocalTempResult;
        } else {
            Stack.MostRecentPropertyAddress = FrameAlloc(1024);
            std::memset(Stack.MostRecentPropertyAddress, 0, 1024);
        }
    }
    Stack.Step(Stack.Object, Stack.MostRecentPropertyAddress);
    if (LocallyKnownProperty && LocalTempResult) LocallyKnownProperty->DestroyValue(LocalTempResult);
}

namespace {

FObjectPropertyBase* ObjectPropertyOf(FProperty* Property) {
    FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property);
    if (ObjectProperty == nullptr)
        if (FArrayProperty* ArrayProp = ExactCastField<FArrayProperty>(Property)) ObjectProperty = CastField<FObjectPropertyBase>(ArrayProp->Inner);
    return ObjectProperty;
}

}

DEFINE_FUNCTION(UObject::execLetObj) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.Step(Stack.Object, nullptr);
    if (Stack.MostRecentPropertyAddress == nullptr)
        ThrowAccessViolation(P_THIS, Stack, "LetObjAccessNone", FString("Accessed None attempting to assign variable on an object"));
    void* ObjAddr = Stack.MostRecentPropertyAddress;
    FObjectPropertyBase* ObjectProperty = ObjectPropertyOf(Stack.MostRecentProperty);
    UObject* NewValue = nullptr;
    Stack.Step(Stack.Object, &NewValue);
    if (ObjAddr) {
        if (ObjectProperty != nullptr) {
            ObjectProperty->SetObjectPropertyValue(ObjAddr, NewValue);
        } else {
            *static_cast<UObject**>(ObjAddr) = NewValue;
        }
    }
}

DEFINE_FUNCTION(UObject::execLetWeakObjPtr) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.Step(Stack.Object, nullptr);
    if (Stack.MostRecentPropertyAddress == nullptr)
        ThrowAccessViolation(P_THIS, Stack, "LetWeakObjAccessNone", FString("Accessed None attempting to assign variable on a weakly referenced object"));
    void* ObjAddr = Stack.MostRecentPropertyAddress;
    FObjectPropertyBase* ObjectProperty = ObjectPropertyOf(Stack.MostRecentProperty);
    UObject* NewValue = nullptr;
    Stack.Step(Stack.Object, &NewValue);
    if (ObjAddr) {
        if (ObjectProperty != nullptr) {
            ObjectProperty->SetObjectPropertyValue(ObjAddr, NewValue);
        } else {
            *static_cast<FWeakObjectPtr*>(ObjAddr) = NewValue;
        }
    }
}

DEFINE_FUNCTION(UObject::execLetBool) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.Step(Stack.Object, nullptr);
    uint8* BoolAddr = Stack.MostRecentPropertyAddress;
    FBoolProperty* BoolProperty = ExactCastField<FBoolProperty>(Stack.MostRecentProperty);
    if (BoolProperty == nullptr)
        if (FArrayProperty* ArrayProp = ExactCastField<FArrayProperty>(Stack.MostRecentProperty))
            BoolProperty = ExactCastField<FBoolProperty>(ArrayProp->Inner);
    bool NewValue = false;
    Stack.Step(Stack.Object, &NewValue);
    if (BoolAddr) {
        if (BoolProperty != nullptr) {
            BoolProperty->SetPropertyValue(BoolAddr, NewValue);
        } else {
            *reinterpret_cast<bool*>(BoolAddr) = NewValue;
        }
    }
}

DEFINE_FUNCTION(UObject::execLetDelegate) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.Step(Stack.Object, nullptr);
    FScriptDelegate* DelegateAddr = reinterpret_cast<FScriptDelegate*>(Stack.MostRecentPropertyAddress);
    FScriptDelegate Delegate;
    Stack.Step(Stack.Object, &Delegate);
    if (DelegateAddr != nullptr) DelegateAddr->BindUFunction(Delegate.GetUObject(), Delegate.GetFunctionName());
}

DEFINE_FUNCTION(UObject::execLetMulticastDelegate) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.Step(Stack.Object, nullptr);
    FMulticastDelegateProperty* DelegateProp = CastField<FMulticastDelegateProperty>(Stack.MostRecentProperty);
    void* DelegateAddr = Stack.MostRecentPropertyAddress;
    FMulticastScriptDelegate Delegate;
    Stack.Step(Stack.Object, &Delegate);
    if (DelegateProp && DelegateAddr) DelegateProp->SetMulticastDelegate(DelegateAddr, std::move(Delegate));
}

DEFINE_FUNCTION(UObject::execSelf) {
    if (RESULT_PARAM != nullptr) {
        *static_cast<UObject**>(RESULT_PARAM) = P_THIS;
    } else {
        ThrowAccessViolation(P_THIS, Stack, "AccessSelfAddress", FString("Attempted to reference 'self' as an addressable property."));
    }
}

DEFINE_FUNCTION(UObject::execContext) { P_THIS->ProcessContextOpcode(Stack, RESULT_PARAM, false); }
DEFINE_FUNCTION(UObject::execContext_FailSilent) { P_THIS->ProcessContextOpcode(Stack, RESULT_PARAM, true); }

void UObject::ProcessContextOpcode(FFrame& Stack, RESULT_DECL, bool bCanFailSilently) {
    Stack.MostRecentProperty = nullptr;
    UObject* NewContext = nullptr;
    Stack.Step(this, &NewContext);
    uint8* const OriginalCode = Stack.Code;
    const bool bValidContext = IsValid(NewContext);
    if (bValidContext) {
        Stack.Code += sizeof(CodeSkipSizeType) + sizeof(ScriptPointerType);
        Stack.Step(NewContext, RESULT_PARAM);
    }
    if (!bValidContext || Stack.bArrayContextFailed) {
        if (Stack.bArrayContextFailed) {
            Stack.bArrayContextFailed = false;
            Stack.Code = OriginalCode;
        }
        if (!bCanFailSilently) {
            if (NewContext && !IsValid(NewContext)) {
                ThrowAccessViolation(this, Stack, "AccessPendingKill",
                                     FString("Attempted to access ") + GetNameSafe(NewContext) + FString(" via property ") +
                                         PropertyNameSafe(Stack.MostRecentProperty) + FString(", but it is not valid (pending kill or garbage)"));
            } else if (Stack.MostRecentProperty != nullptr) {
                ThrowAccessViolation(this, Stack, "AccessNoneContext", FString("Accessed None trying to read property ") + Stack.MostRecentProperty->GetName());
            } else {
                ThrowAccessViolation(this, Stack, "AccessNoneNoContext", FString("Accessed None"));
            }
        }
        const CodeSkipSizeType wSkip = Stack.ReadCodeSkipCount();
        FProperty* RValueProperty = nullptr;
        Stack.ReadVariableSize(&RValueProperty);
        Stack.Code += wSkip;
        Stack.MostRecentPropertyAddress = nullptr;
        Stack.MostRecentPropertyContainer = nullptr;
        Stack.MostRecentProperty = nullptr;
        if (RESULT_PARAM && RValueProperty) RValueProperty->ClearValue(RESULT_PARAM);
    }
}

DEFINE_FUNCTION(UObject::execStructMemberContext) {
    FProperty* StructProperty = Stack.ReadProperty();
    Stack.MostRecentProperty = nullptr;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.Step(Stack.Object, nullptr);
    if (Stack.MostRecentProperty != nullptr && Stack.MostRecentPropertyAddress != nullptr && StructProperty != nullptr) {
        Stack.MostRecentPropertyContainer = Stack.MostRecentPropertyAddress;
        Stack.MostRecentPropertyAddress = StructProperty->ContainerPtrToValuePtr<uint8>(Stack.MostRecentPropertyAddress);
        Stack.MostRecentProperty = StructProperty;
        if (RESULT_PARAM) StructProperty->CopyCompleteValueToScriptVM_InContainer(RESULT_PARAM, Stack.MostRecentPropertyContainer);
    } else {
        ThrowAccessViolation(P_THIS, Stack, "AccessNoneStructure", FString("Accessed None reading structure ") + PropertyNameSafe(StructProperty));
        Stack.MostRecentPropertyAddress = nullptr;
        Stack.MostRecentPropertyContainer = nullptr;
        Stack.MostRecentProperty = nullptr;
    }
}

DEFINE_FUNCTION(UObject::execVirtualFunction) { P_THIS->CallFunction(Stack, RESULT_PARAM, P_THIS->FindFunctionChecked(Stack.ReadName())); }
DEFINE_FUNCTION(UObject::execFinalFunction) { P_THIS->CallFunction(Stack, RESULT_PARAM, reinterpret_cast<UFunction*>(Stack.ReadObject())); }
DEFINE_FUNCTION(UObject::execLocalVirtualFunction) {
    ProcessLocalFunction(Context, P_THIS->FindFunctionChecked(Stack.ReadName()), Stack, RESULT_PARAM);
}
DEFINE_FUNCTION(UObject::execLocalFinalFunction) {
    ProcessLocalFunction(Context, reinterpret_cast<UFunction*>(Stack.ReadObject()), Stack, RESULT_PARAM);
}

DEFINE_FUNCTION(UObject::execCallMathFunction) {
    UFunction* Function = reinterpret_cast<UFunction*>(Stack.ReadObject());
    if (Function == nullptr || Function->GetNativeFunc() == nullptr) {
        execUndefined(Context, Stack, RESULT_PARAM);
        return;
    }
    UObject* NewContext = Function->GetOuterUClass() ? Function->GetOuterUClass()->GetDefaultObject() : Context;
    Stack.CurrentNativeFunction = Function;
    Function->GetNativeFunc()(NewContext, Stack, RESULT_PARAM);
}

DEFINE_FUNCTION(UObject::execCallMulticastDelegate) {
    UFunction* SignatureFunction = Cast<UFunction>(Stack.ReadObject());
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.Step(Stack.Object, nullptr);
    FMulticastDelegateProperty* DelegateProp = CastField<FMulticastDelegateProperty>(Stack.MostRecentProperty);
    const FMulticastScriptDelegate* DelegateAddr =
        DelegateProp && Stack.MostRecentPropertyAddress ? DelegateProp->GetMulticastDelegate(Stack.MostRecentPropertyAddress) : nullptr;
    if (SignatureFunction == nullptr) {
        while (*Stack.Code != EX_EndFunctionParms) {
            alignas(16) uint8 Scratch[1024] = {};
            Stack.Step(Stack.Object, Scratch);
        }
        Stack.Code++;
        return;
    }
    FFrameAllocatorScope Scope;
    uint8* Parameters = FrameAlloc(SignatureFunction->ParmsSize, static_cast<std::size_t>(SignatureFunction->GetMinAlignment()));
    std::memset(Parameters, 0, SignatureFunction->ParmsSize);
    for (FProperty* Property = CastField<FProperty>(SignatureFunction->ChildProperties); *Stack.Code != EX_EndFunctionParms;
         Property = Property ? Property->GetNextProperty() : nullptr) {
        Stack.MostRecentPropertyAddress = nullptr;
        Stack.MostRecentPropertyContainer = nullptr;
        if (Property == nullptr) {
            alignas(16) uint8 Scratch[1024] = {};
            Stack.Step(Stack.Object, Scratch);
            continue;
        }
        if (Property->PropertyFlags & CPF_OutParm) {
            Stack.Step(Stack.Object, nullptr);
            if (Stack.MostRecentPropertyAddress != nullptr)
                Property->CopyCompleteValueToScriptVM(Property->ContainerPtrToValuePtr<uint8>(Parameters), Stack.MostRecentPropertyAddress);
        } else {
            uint8* Param = Property->ContainerPtrToValuePtr<uint8>(Parameters);
            Property->InitializeValue_InContainer(Parameters);
            Stack.Step(Stack.Object, Param);
        }
    }
    Stack.Code++;
    if (DelegateAddr) DelegateAddr->ProcessMulticastDelegate(Parameters);
    for (FProperty* Destruct = SignatureFunction->DestructorLink; Destruct; Destruct = Destruct->DestructorLinkNext)
        Destruct->DestroyValue_InContainer(Parameters);
}

DEFINE_FUNCTION(UObject::execAddMulticastDelegate) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.Step(Stack.Object, nullptr);
    FMulticastDelegateProperty* DelegateProp = CastField<FMulticastDelegateProperty>(Stack.MostRecentProperty);
    void* DelegateAddr = Stack.MostRecentPropertyAddress;
    FScriptDelegate Delegate;
    Stack.Step(Stack.Object, &Delegate);
    if (DelegateProp && DelegateAddr) DelegateProp->AddDelegate(std::move(Delegate), nullptr, DelegateAddr);
}

DEFINE_FUNCTION(UObject::execRemoveMulticastDelegate) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.Step(Stack.Object, nullptr);
    FMulticastDelegateProperty* DelegateProp = CastField<FMulticastDelegateProperty>(Stack.MostRecentProperty);
    void* DelegateAddr = Stack.MostRecentPropertyAddress;
    FScriptDelegate Delegate;
    Stack.Step(Stack.Object, &Delegate);
    if (DelegateProp && DelegateAddr) DelegateProp->RemoveDelegate(Delegate, nullptr, DelegateAddr);
}

DEFINE_FUNCTION(UObject::execClearMulticastDelegate) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.Step(Stack.Object, nullptr);
    FMulticastDelegateProperty* DelegateProp = CastField<FMulticastDelegateProperty>(Stack.MostRecentProperty);
    void* DelegateAddr = Stack.MostRecentPropertyAddress;
    if (DelegateProp && DelegateAddr) DelegateProp->ClearDelegate(nullptr, DelegateAddr);
}

DEFINE_FUNCTION(UObject::execIntConst) { *static_cast<int32*>(RESULT_PARAM) = Stack.ReadInt<int32>(); }
DEFINE_FUNCTION(UObject::execInt64Const) { *static_cast<int64*>(RESULT_PARAM) = Stack.ReadInt<int64>(); }
DEFINE_FUNCTION(UObject::execUInt64Const) { *static_cast<uint64*>(RESULT_PARAM) = Stack.ReadInt<uint64>(); }
DEFINE_FUNCTION(UObject::execSkipOffsetConst) { *static_cast<int32*>(RESULT_PARAM) = static_cast<int32>(Stack.ReadCodeSkipCount()); }
DEFINE_FUNCTION(UObject::execFloatConst) { *static_cast<float*>(RESULT_PARAM) = Stack.ReadFloat(); }
DEFINE_FUNCTION(UObject::execDoubleConst) { *static_cast<double*>(RESULT_PARAM) = Stack.ReadDouble(); }

DEFINE_FUNCTION(UObject::execStringConst) {
    // An ANSI (Latin-1) string.
    const char* Text = reinterpret_cast<const char*>(Stack.Code);
    std::size_t Length = std::strlen(Text);
    FString Result;
    TArray<TCHAR>& Chars = Result.GetCharArray();
    if (Length > 0) {
        Chars.Reserve(static_cast<int32>(Length) + 1);
        for (std::size_t i = 0; i < Length; i++) Chars.Add(static_cast<TCHAR>(static_cast<uint8>(Text[i])));
        Chars.Add(0);
    }
    *static_cast<FString*>(RESULT_PARAM) = std::move(Result);
    Stack.Code += Length + 1;
}

DEFINE_FUNCTION(UObject::execUnicodeStringConst) {
    std::u16string Text;
    for (;;) {
        uint16 Char;
        std::memcpy(&Char, Stack.Code, sizeof(Char));
        Stack.Code += sizeof(uint16);
        if (Char == 0) break;
        Text.push_back(static_cast<char16_t>(Char));
    }
    *static_cast<FString*>(RESULT_PARAM) = FString(std::u16string_view(Text));
}

DEFINE_FUNCTION(UObject::execTextConst) {
    const EBlueprintTextLiteralType TextLiteralType = static_cast<EBlueprintTextLiteralType>(*Stack.Code++);
    switch (TextLiteralType) {
    case EBlueprintTextLiteralType::Empty:
        *static_cast<FText*>(RESULT_PARAM) = FText::GetEmpty();
        break;
    case EBlueprintTextLiteralType::LocalizedText: {
        FString SourceString, KeyString, Namespace;
        Stack.Step(Stack.Object, &SourceString);
        Stack.Step(Stack.Object, &KeyString);
        Stack.Step(Stack.Object, &Namespace);
        *static_cast<FText*>(RESULT_PARAM) = FText::MakeLocalized(std::move(SourceString), std::move(Namespace), std::move(KeyString));
        break;
    }
    case EBlueprintTextLiteralType::InvariantText: {
        FString SourceString;
        Stack.Step(Stack.Object, &SourceString);
        *static_cast<FText*>(RESULT_PARAM) = FText::AsCultureInvariant(std::move(SourceString));
        break;
    }
    case EBlueprintTextLiteralType::LiteralString: {
        FString SourceString;
        Stack.Step(Stack.Object, &SourceString);
        *static_cast<FText*>(RESULT_PARAM) = FText::FromString(std::move(SourceString));
        break;
    }
    case EBlueprintTextLiteralType::StringTableEntry: {
        Stack.ReadObject();
        FString TableIdString, KeyString;
        Stack.Step(Stack.Object, &TableIdString);
        Stack.Step(Stack.Object, &KeyString);
        // The display string is looked up on first use (SetStringTableResolver).
        *static_cast<FText*>(RESULT_PARAM) = FText::FromStringTable(MakeName(TableIdString), KeyString);
        break;
    }
    }
}

DEFINE_FUNCTION(UObject::execPropertyConst) { *static_cast<FField**>(RESULT_PARAM) = Stack.ReadField(); }
DEFINE_FUNCTION(UObject::execObjectConst) { *static_cast<UObject**>(RESULT_PARAM) = Stack.ReadObject(); }

DEFINE_FUNCTION(UObject::execSoftObjectConst) {
    FString LongPath;
    Stack.Step(Stack.Object, &LongPath);
    *static_cast<FSoftObjectPtr*>(RESULT_PARAM) = FSoftObjectPtr(FSoftObjectPath(LongPath));
}

DEFINE_FUNCTION(UObject::execFieldPathConst) {
    FString StringPath;
    Stack.Step(Stack.Object, &StringPath);
    FFieldPath FieldPath;
    FieldPath.Generate(StringPath);
    *static_cast<FFieldPath*>(RESULT_PARAM) = FieldPath;
}

DEFINE_FUNCTION(UObject::execInstanceDelegate) {
    FName FunctionName = Stack.ReadName();
    static_cast<FScriptDelegate*>(RESULT_PARAM)->BindUFunction(FunctionName.isNone() ? nullptr : P_THIS, FunctionName);
}

DEFINE_FUNCTION(UObject::execBindDelegate) {
    FName FunctionName = Stack.ReadName();
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.Step(Stack.Object, nullptr);
    FScriptDelegate* DelegateAddr = reinterpret_cast<FScriptDelegate*>(Stack.MostRecentPropertyAddress);
    UObject* ObjectForDelegate = nullptr;
    Stack.Step(Stack.Object, &ObjectForDelegate);
    if (DelegateAddr) DelegateAddr->BindUFunction(ObjectForDelegate, FunctionName);
}

DEFINE_FUNCTION(UObject::execNameConst) { *static_cast<FName*>(RESULT_PARAM) = Stack.ReadName(); }
DEFINE_FUNCTION(UObject::execByteConst) { *static_cast<uint8*>(RESULT_PARAM) = *Stack.Code++; }

DEFINE_FUNCTION(UObject::execRotationConst) {
    FRotator* Rotator = static_cast<FRotator*>(RESULT_PARAM);
    Rotator->Pitch = Stack.ReadDouble();
    Rotator->Yaw = Stack.ReadDouble();
    Rotator->Roll = Stack.ReadDouble();
}

DEFINE_FUNCTION(UObject::execVectorConst) {
    FVector* Vector = static_cast<FVector*>(RESULT_PARAM);
    Vector->X = Stack.ReadDouble();
    Vector->Y = Stack.ReadDouble();
    Vector->Z = Stack.ReadDouble();
}

DEFINE_FUNCTION(UObject::execVector3fConst) {
    FVector3f* Vector = static_cast<FVector3f*>(RESULT_PARAM);
    Vector->X = Stack.ReadFloat();
    Vector->Y = Stack.ReadFloat();
    Vector->Z = Stack.ReadFloat();
}

DEFINE_FUNCTION(UObject::execTransformConst) {
    FTransform* Transform = static_cast<FTransform*>(RESULT_PARAM);
    Transform->Rotation.X = Stack.ReadDouble();
    Transform->Rotation.Y = Stack.ReadDouble();
    Transform->Rotation.Z = Stack.ReadDouble();
    Transform->Rotation.W = Stack.ReadDouble();
    Transform->Translation.X = Stack.ReadDouble();
    Transform->Translation.Y = Stack.ReadDouble();
    Transform->Translation.Z = Stack.ReadDouble();
    Transform->Scale3D.X = Stack.ReadDouble();
    Transform->Scale3D.Y = Stack.ReadDouble();
    Transform->Scale3D.Z = Stack.ReadDouble();
}

DEFINE_FUNCTION(UObject::execStructConst) {
    UScriptStruct* ScriptStruct = Cast<UScriptStruct>(Stack.ReadObject());
    Stack.ReadInt<int32>();
    if (ScriptStruct == nullptr) {
        while (*Stack.Code != EX_EndStructConst) {
            alignas(16) uint8 Scratch[1024] = {};
            Stack.Step(Stack.Object, Scratch);
        }
        P_FINISH;
        return;
    }
    for (FProperty* StructProp = ScriptStruct->PropertyLink; StructProp; StructProp = StructProp->PropertyLinkNext) {
        if (StructProp->PropertyFlags & (CPF_Transient | CPF_EditorOnly)) continue;
        for (int32 ArrayIter = 0; ArrayIter < StructProp->ArrayDim; ++ArrayIter)
            Stack.Step(Stack.Object, StructProp->ContainerPtrToValuePtr<uint8>(RESULT_PARAM, ArrayIter));
    }
    P_FINISH;
}

DEFINE_FUNCTION(UObject::execSetArray) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.Step(Stack.Object, nullptr);
    FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
    if (ArrayProperty == nullptr || Stack.MostRecentPropertyAddress == nullptr) {
        while (*Stack.Code != EX_EndArray) {
            alignas(16) uint8 Scratch[1024] = {};
            Stack.Step(Stack.Object, Scratch);
        }
        P_FINISH;
        return;
    }
    FScriptArrayHelper ArrayHelper(ArrayProperty, Stack.MostRecentPropertyAddress);
    ArrayHelper.EmptyValues();
    int32 i = 0;
    while (*Stack.Code != EX_EndArray) {
        ArrayHelper.AddValues(1);
        Stack.Step(Stack.Object, ArrayHelper.GetRawPtr(i++));
    }
    P_FINISH;
}

DEFINE_FUNCTION(UObject::execSetSet) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.Step(Stack.Object, nullptr);
    const int32 Num = Stack.ReadInt<int32>();
    FSetProperty* SetProperty = CastField<FSetProperty>(Stack.MostRecentProperty);
    if (SetProperty == nullptr || Stack.MostRecentPropertyAddress == nullptr) {
        while (*Stack.Code != EX_EndSet) {
            alignas(16) uint8 Scratch[1024] = {};
            Stack.Step(Stack.Object, Scratch);
        }
        P_FINISH;
        return;
    }
    FScriptSetHelper SetHelper(SetProperty, Stack.MostRecentPropertyAddress);
    SetHelper.EmptyElements(Num);
    if (Num > 0) {
        FDefaultConstructedPropertyElement TempElement(SetProperty->ElementProp);
        while (*Stack.Code != EX_EndSet) {
            Stack.Step(Stack.Object, TempElement.GetObjAddress());
            SetHelper.AddElement(TempElement.GetObjAddress());
        }
    }
    P_FINISH;
}

DEFINE_FUNCTION(UObject::execSetMap) {
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.Step(Stack.Object, nullptr);
    const int32 Num = Stack.ReadInt<int32>();
    FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
    if (MapProperty == nullptr || Stack.MostRecentPropertyAddress == nullptr) {
        while (*Stack.Code != EX_EndMap) {
            alignas(16) uint8 Scratch[1024] = {};
            Stack.Step(Stack.Object, Scratch);
        }
        P_FINISH;
        return;
    }
    FScriptMapHelper MapHelper(MapProperty, Stack.MostRecentPropertyAddress);
    MapHelper.EmptyValues(Num);
    if (Num > 0) {
        FDefaultConstructedPropertyElement TempKey(MapProperty->KeyProp);
        FDefaultConstructedPropertyElement TempValue(MapProperty->ValueProp);
        while (*Stack.Code != EX_EndMap) {
            Stack.Step(Stack.Object, TempKey.GetObjAddress());
            Stack.Step(Stack.Object, TempValue.GetObjAddress());
            MapHelper.AddPair(TempKey.GetObjAddress(), TempValue.GetObjAddress());
        }
    }
    P_FINISH;
}

DEFINE_FUNCTION(UObject::execArrayConst) {
    FProperty* InnerProperty = CastField<FProperty>(Stack.ReadField());
    Stack.ReadInt<int32>();
    if (InnerProperty == nullptr) {
        while (*Stack.Code != EX_EndArrayConst) {
            alignas(16) uint8 Scratch[1024] = {};
            Stack.Step(Stack.Object, Scratch);
        }
        P_FINISH;
        return;
    }
    FScriptArrayHelper ArrayHelper = FScriptArrayHelper::CreateHelperFormInnerProperty(InnerProperty, RESULT_PARAM);
    ArrayHelper.EmptyValues();
    int32 i = 0;
    while (*Stack.Code != EX_EndArrayConst) {
        ArrayHelper.AddValues(1);
        Stack.Step(Stack.Object, ArrayHelper.GetRawPtr(i++));
    }
    P_FINISH;
}

DEFINE_FUNCTION(UObject::execSetConst) {
    FProperty* InnerProperty = CastField<FProperty>(Stack.ReadField());
    int32 Num = Stack.ReadInt<int32>();
    if (InnerProperty == nullptr) {
        while (*Stack.Code != EX_EndSetConst) {
            alignas(16) uint8 Scratch[1024] = {};
            Stack.Step(Stack.Object, Scratch);
        }
        P_FINISH;
        return;
    }
    FScriptSetHelper SetHelper = FScriptSetHelper::CreateHelperFormElementProperty(InnerProperty, RESULT_PARAM);
    SetHelper.EmptyElements(Num);
    while (*Stack.Code != EX_EndSetConst) {
        int32 Index = SetHelper.AddDefaultValue_Invalid_NeedsRehash();
        Stack.Step(Stack.Object, SetHelper.GetElementPtr(Index));
    }
    SetHelper.Rehash();
    P_FINISH;
}

DEFINE_FUNCTION(UObject::execMapConst) {
    FProperty* KeyProperty = CastField<FProperty>(Stack.ReadField());
    FProperty* ValProperty = CastField<FProperty>(Stack.ReadField());
    int32 Num = Stack.ReadInt<int32>();
    if (KeyProperty == nullptr || ValProperty == nullptr) {
        while (*Stack.Code != EX_EndMapConst) {
            alignas(16) uint8 Scratch[1024] = {};
            Stack.Step(Stack.Object, Scratch);
        }
        P_FINISH;
        return;
    }
    FScriptMapHelper MapHelper = FScriptMapHelper::CreateHelperFormInnerProperties(KeyProperty, ValProperty, RESULT_PARAM);
    MapHelper.EmptyValues(Num);
    while (*Stack.Code != EX_EndMapConst) {
        int32 Index = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
        Stack.Step(Stack.Object, MapHelper.GetKeyPtr(Index));
        Stack.Step(Stack.Object, MapHelper.GetValuePtr(Index));
    }
    MapHelper.Rehash();
    P_FINISH;
}

DEFINE_FUNCTION(UObject::execBitFieldConst) {
    FBoolProperty* BitProperty = CastField<FBoolProperty>(Stack.ReadField());
    uint8 ByteValue = Stack.Read<uint8>();
    if (BitProperty != nullptr) {
        BitProperty->SetPropertyValue(RESULT_PARAM, ByteValue != 0);
    } else {
        *static_cast<bool*>(RESULT_PARAM) = ByteValue != 0;
    }
}

DEFINE_FUNCTION(UObject::execIntZero) { *static_cast<int32*>(RESULT_PARAM) = 0; }
DEFINE_FUNCTION(UObject::execIntOne) { *static_cast<int32*>(RESULT_PARAM) = 1; }
DEFINE_FUNCTION(UObject::execTrue) { *static_cast<bool*>(RESULT_PARAM) = true; }
DEFINE_FUNCTION(UObject::execFalse) { *static_cast<bool*>(RESULT_PARAM) = false; }
DEFINE_FUNCTION(UObject::execNoObject) { *static_cast<UObject**>(RESULT_PARAM) = nullptr; }
DEFINE_FUNCTION(UObject::execNullInterface) { static_cast<FScriptInterface*>(RESULT_PARAM)->SetObject(nullptr); }
DEFINE_FUNCTION(UObject::execIntConstByte) { *static_cast<int32*>(RESULT_PARAM) = *Stack.Code++; }

DEFINE_FUNCTION(UObject::execDynamicCast) {
    UClass* ClassPtr = reinterpret_cast<UClass*>(Stack.ReadObject());
    UObject* Castee = nullptr;
    Stack.Step(Stack.Object, &Castee);
    *static_cast<UObject**>(RESULT_PARAM) = nullptr;
    if (ClassPtr == nullptr) return;
    if (Castee == nullptr) {
        if (ClassPtr->HasAnyClassFlags(CLASS_Interface)) {
            static_cast<FScriptInterface*>(RESULT_PARAM)->SetObject(nullptr);
        } else {
            *static_cast<UObject**>(RESULT_PARAM) = nullptr;
        }
        return;
    }
    if (ClassPtr->HasAnyClassFlags(CLASS_Interface)) {
        if (Castee->GetClass()->ImplementsInterface(ClassPtr)) {
            static_cast<FScriptInterface*>(RESULT_PARAM)->SetObject(Castee);
            static_cast<FScriptInterface*>(RESULT_PARAM)->SetInterface(Castee->GetClass()->GetInterfaceAddress(Castee, ClassPtr));
        }
    } else if (Castee->IsA(ClassPtr)) {
        *static_cast<UObject**>(RESULT_PARAM) = Castee;
    }
}

DEFINE_FUNCTION(UObject::execMetaCast) {
    UClass* MetaClass = reinterpret_cast<UClass*>(Stack.ReadObject());
    UObject* Castee = nullptr;
    Stack.Step(Stack.Object, &Castee);
    UClass* CasteeClass = Cast<UClass>(Castee);
    *static_cast<UObject**>(RESULT_PARAM) = CasteeClass && CasteeClass->IsChildOf(MetaClass) ? Castee : nullptr;
}

DEFINE_FUNCTION(UObject::execCast) {
    int32 B = *(Stack.Code)++;
    (*GCasts[B])(Stack.Object, Stack, RESULT_PARAM);
}

DEFINE_FUNCTION(UObject::execInterfaceCast) { (*GCasts[CST_ObjectToInterface])(Stack.Object, Stack, RESULT_PARAM); }

DEFINE_FUNCTION(UObject::execDoubleToFloatCast) {
    if (Stack.StepAndCheckMostRecentProperty(Stack.Object, nullptr))
        *static_cast<float*>(RESULT_PARAM) = static_cast<float>(*reinterpret_cast<const double*>(Stack.MostRecentPropertyAddress));
}

DEFINE_FUNCTION(UObject::execFloatToDoubleCast) {
    if (Stack.StepAndCheckMostRecentProperty(Stack.Object, nullptr))
        *static_cast<double*>(RESULT_PARAM) = *reinterpret_cast<const float*>(Stack.MostRecentPropertyAddress);
}

DEFINE_FUNCTION(UObject::execObjectToBool) {
    UObject* Obj = nullptr;
    Stack.Step(Stack.Object, &Obj);
    *static_cast<bool*>(RESULT_PARAM) = Obj != nullptr;
}

DEFINE_FUNCTION(UObject::execInterfaceToBool) {
    FScriptInterface Interface;
    Stack.Step(Stack.Object, &Interface);
    *static_cast<bool*>(RESULT_PARAM) = Interface.GetObject() != nullptr;
}

DEFINE_FUNCTION(UObject::execObjectToInterface) {
    FScriptInterface& InterfaceValue = *static_cast<FScriptInterface*>(RESULT_PARAM);
    UClass* InterfaceClass = Cast<UClass>(Stack.ReadObject());
    UObject* ObjectValue = nullptr;
    Stack.Step(Stack.Object, &ObjectValue);
    if (ObjectValue && InterfaceClass && ObjectValue->GetClass()->ImplementsInterface(InterfaceClass)) {
        InterfaceValue.SetObject(ObjectValue);
        InterfaceValue.SetInterface(ObjectValue->GetClass()->GetInterfaceAddress(ObjectValue, InterfaceClass));
    } else {
        InterfaceValue.SetObject(nullptr);
    }
}

DEFINE_FUNCTION(UObject::execInterfaceToInterface) {
    FScriptInterface& CastResult = *static_cast<FScriptInterface*>(RESULT_PARAM);
    UClass* ClassToCastTo = Cast<UClass>(Stack.ReadObject());
    FScriptInterface InterfaceInput;
    Stack.Step(Stack.Object, &InterfaceInput);
    UObject* ObjectWithInterface = InterfaceInput.GetObject();
    if (ObjectWithInterface && ClassToCastTo && ObjectWithInterface->GetClass()->ImplementsInterface(ClassToCastTo)) {
        CastResult.SetObject(ObjectWithInterface);
        CastResult.SetInterface(ObjectWithInterface->GetClass()->GetInterfaceAddress(ObjectWithInterface, ClassToCastTo));
    } else {
        CastResult.SetObject(nullptr);
    }
}

DEFINE_FUNCTION(UObject::execInterfaceToObject) {
    UClass* ObjClassToCastTo = Cast<UClass>(Stack.ReadObject());
    FScriptInterface InterfaceInput;
    Stack.Step(Stack.Object, &InterfaceInput);
    UObject* InputObjWithInterface = InterfaceInput.GetObject();
    *static_cast<UObject**>(RESULT_PARAM) = InputObjWithInterface && ObjClassToCastTo && InputObjWithInterface->IsA(ObjClassToCastTo)
                                                ? InputObjWithInterface
                                                : nullptr;
}

DEFINE_FUNCTION(UObject::execAutoRtfmTransact) {
    // Without transactional memory the transaction always commits: run to its stop token.
    int32 TransactionId = Stack.ReadInt<int32>();
    Stack.ReadCodeSkipCount();
    for (;;) {
        if (*Stack.Code == EX_AutoRtfmStopTransact) {
            Stack.Code++;
            int32 Value = Stack.ReadInt<int32>();
            Stack.Read<uint8>();
            if (Value == TransactionId) break;
        } else {
            Stack.Step(Stack.Object, RESULT_PARAM);
        }
    }
}

DEFINE_FUNCTION(UObject::execAutoRtfmStopTransact) {
    Stack.ReadInt<int32>();
    Stack.Read<uint8>();
}

DEFINE_FUNCTION(UObject::execAutoRtfmAbortIfNot) {
    bool Result = false;
    Stack.Step(Stack.Object, &Result);
}

namespace {

struct FNativeEntry {
    EExprToken Token;
    FNativeFuncPtr Func;
};

const FNativeEntry VMFunctions[] = {
    {EX_LocalVariable, &UObject::execLocalVariable},
    {EX_InstanceVariable, &UObject::execInstanceVariable},
    {EX_DefaultVariable, &UObject::execDefaultVariable},
    {EX_Jump, &UObject::execJump},
    {EX_JumpIfNot, &UObject::execJumpIfNot},
    {EX_Assert, &UObject::execAssert},
    {EX_Nothing, &UObject::execNothing},
    {EX_NothingInt32, &UObject::execNothingInt32},
    {EX_Let, &UObject::execLet},
    {EX_BitFieldConst, &UObject::execBitFieldConst},
    {EX_ClassContext, &UObject::execClassContext},
    {EX_MetaCast, &UObject::execMetaCast},
    {EX_LetBool, &UObject::execLetBool},
    {EX_EndFunctionParms, &UObject::execEndFunctionParms},
    {EX_Self, &UObject::execSelf},
    {EX_Context, &UObject::execContext},
    {EX_Context_FailSilent, &UObject::execContext_FailSilent},
    {EX_VirtualFunction, &UObject::execVirtualFunction},
    {EX_FinalFunction, &UObject::execFinalFunction},
    {EX_IntConst, &UObject::execIntConst},
    {EX_FloatConst, &UObject::execFloatConst},
    {EX_StringConst, &UObject::execStringConst},
    {EX_ObjectConst, &UObject::execObjectConst},
    {EX_NameConst, &UObject::execNameConst},
    {EX_RotationConst, &UObject::execRotationConst},
    {EX_VectorConst, &UObject::execVectorConst},
    {EX_ByteConst, &UObject::execByteConst},
    {EX_IntZero, &UObject::execIntZero},
    {EX_IntOne, &UObject::execIntOne},
    {EX_True, &UObject::execTrue},
    {EX_False, &UObject::execFalse},
    {EX_TextConst, &UObject::execTextConst},
    {EX_NoObject, &UObject::execNoObject},
    {EX_TransformConst, &UObject::execTransformConst},
    {EX_IntConstByte, &UObject::execIntConstByte},
    {EX_NoInterface, &UObject::execNullInterface},
    {EX_DynamicCast, &UObject::execDynamicCast},
    {EX_StructConst, &UObject::execStructConst},
    {EX_SetArray, &UObject::execSetArray},
    {EX_PropertyConst, &UObject::execPropertyConst},
    {EX_UnicodeStringConst, &UObject::execUnicodeStringConst},
    {EX_Int64Const, &UObject::execInt64Const},
    {EX_UInt64Const, &UObject::execUInt64Const},
    {EX_DoubleConst, &UObject::execDoubleConst},
    {EX_Cast, &UObject::execCast},
    {EX_SetSet, &UObject::execSetSet},
    {EX_SetMap, &UObject::execSetMap},
    {EX_SetConst, &UObject::execSetConst},
    {EX_MapConst, &UObject::execMapConst},
    {EX_Vector3fConst, &UObject::execVector3fConst},
    {EX_StructMemberContext, &UObject::execStructMemberContext},
    {EX_LetMulticastDelegate, &UObject::execLetMulticastDelegate},
    {EX_LetDelegate, &UObject::execLetDelegate},
    {EX_LocalVirtualFunction, &UObject::execLocalVirtualFunction},
    {EX_LocalFinalFunction, &UObject::execLocalFinalFunction},
    {EX_LocalOutVariable, &UObject::execLocalOutVariable},
    {EX_DeprecatedOp4A, &UObject::execNothingOp4a},
    {EX_InstanceDelegate, &UObject::execInstanceDelegate},
    {EX_PushExecutionFlow, &UObject::execPushExecutionFlow},
    {EX_PopExecutionFlow, &UObject::execPopExecutionFlow},
    {EX_ComputedJump, &UObject::execComputedJump},
    {EX_PopExecutionFlowIfNot, &UObject::execPopExecutionFlowIfNot},
    {EX_Breakpoint, &UObject::execBreakpoint},
    {EX_InterfaceContext, &UObject::execInterfaceContext},
    {EX_ObjToInterfaceCast, &UObject::execInterfaceCast},
    {EX_EndOfScript, &UObject::execEndOfScript},
    {EX_CrossInterfaceCast, &UObject::execInterfaceToInterface},
    {EX_InterfaceToObjCast, &UObject::execInterfaceToObject},
    {EX_WireTracepoint, &UObject::execWireTracepoint},
    {EX_SkipOffsetConst, &UObject::execSkipOffsetConst},
    {EX_AddMulticastDelegate, &UObject::execAddMulticastDelegate},
    {EX_ClearMulticastDelegate, &UObject::execClearMulticastDelegate},
    {EX_Tracepoint, &UObject::execTracepoint},
    {EX_LetObj, &UObject::execLetObj},
    {EX_LetWeakObjPtr, &UObject::execLetWeakObjPtr},
    {EX_BindDelegate, &UObject::execBindDelegate},
    {EX_RemoveMulticastDelegate, &UObject::execRemoveMulticastDelegate},
    {EX_CallMulticastDelegate, &UObject::execCallMulticastDelegate},
    {EX_LetValueOnPersistentFrame, &UObject::execLetValueOnPersistentFrame},
    {EX_ArrayConst, &UObject::execArrayConst},
    {EX_SoftObjectConst, &UObject::execSoftObjectConst},
    {EX_CallMath, &UObject::execCallMathFunction},
    {EX_SwitchValue, &UObject::execSwitchValue},
    {EX_InstrumentationEvent, &UObject::execInstrumentation},
    {EX_ArrayGetByRef, &UObject::execArrayGetByRef},
    {EX_ClassSparseDataVariable, &UObject::execClassSparseDataVariable},
    {EX_FieldPathConst, &UObject::execFieldPathConst},
    {EX_AutoRtfmTransact, &UObject::execAutoRtfmTransact},
    {EX_AutoRtfmStopTransact, &UObject::execAutoRtfmStopTransact},
    {EX_AutoRtfmAbortIfNot, &UObject::execAutoRtfmAbortIfNot},
};

}

void InitializeScriptCore() {
    for (FNativeFuncPtr& Native : GNatives) Native = &UObject::execUndefined;
    for (FNativeFuncPtr& Cast : GCasts) Cast = &UObject::execUndefined;
    for (const FNativeEntry& Entry : VMFunctions) GNatives[Entry.Token] = Entry.Func;
    GCasts[CST_ObjectToInterface] = &UObject::execObjectToInterface;
    GCasts[CST_ObjectToBool] = &UObject::execObjectToBool;
    GCasts[CST_InterfaceToBool] = &UObject::execInterfaceToBool;
    GCasts[CST_DoubleToFloat] = &UObject::execDoubleToFloatCast;
    GCasts[CST_FloatToDouble] = &UObject::execFloatToDoubleCast;
}

}

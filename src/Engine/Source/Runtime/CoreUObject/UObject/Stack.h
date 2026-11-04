#pragma once

#include <cstring>

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

namespace ue5parse::runtime {

struct FOutParmRec {
    FProperty* Property;
    uint8* PropAddr;
    FOutParmRec* NextOutParm;
};

struct FFrame {
    UFunction* Node;
    UObject* Object;
    uint8* Code;
    uint8* Locals;
    FProperty* MostRecentProperty = nullptr;
    uint8* MostRecentPropertyAddress = nullptr;
    uint8* MostRecentPropertyContainer = nullptr;
    TArray<CodeSkipSizeType> FlowStack;
    FFrame* PreviousFrame;
    FOutParmRec* OutParms = nullptr;
    FField* PropertyChainForCompiledIn;
    UFunction* CurrentNativeFunction = nullptr;
    FFrame* PreviousTrackingFrame;
    bool bArrayContextFailed = false;
    bool bAbortingExecution = false;

    FFrame(UObject* InObject, UFunction* InNode, void* InLocals, FFrame* InPreviousFrame = nullptr,
           FField* InPropertyChainForCompiledIn = nullptr);
    ~FFrame();
    FFrame(const FFrame&) = delete;
    FFrame& operator=(const FFrame&) = delete;

    void Step(UObject* Context, RESULT_DECL) {
        int32 B = *Code++;
        GNatives[B](Context, *this, RESULT_PARAM);
    }
    bool StepAndCheckMostRecentProperty(UObject* Context, RESULT_DECL) {
        Step(Context, RESULT_PARAM);
        return MostRecentProperty && MostRecentPropertyAddress;
    }
    void StepExplicitProperty(void* const Result, FProperty* Property);
    void StepCompiledIn(void* Result) {
        if (Code) {
            Step(Object, Result);
        } else {
            FProperty* Property = static_cast<FProperty*>(PropertyChainForCompiledIn);
            PropertyChainForCompiledIn = Property->Next;
            StepExplicitProperty(Result, Property);
        }
    }
    template <class TNativeType>
    TNativeType& StepCompiledInRef(void* const TemporaryBuffer) {
        MostRecentPropertyAddress = nullptr;
        MostRecentPropertyContainer = nullptr;
        FProperty* Property = nullptr;
        if (Code) {
            MostRecentProperty = nullptr;
            Step(Object, TemporaryBuffer);
            Property = MostRecentProperty;
        } else {
            Property = static_cast<FProperty*>(PropertyChainForCompiledIn);
            PropertyChainForCompiledIn = Property->Next;
            StepExplicitProperty(TemporaryBuffer, Property);
        }
        if (MostRecentPropertyAddress == nullptr) return *static_cast<TNativeType*>(TemporaryBuffer);
        if (Property != nullptr) Property->ResolveObjectReferences(MostRecentPropertyAddress);
        return *reinterpret_cast<TNativeType*>(MostRecentPropertyAddress);
    }

    void SkipCode(int32 NumOps) { Code += NumOps; }
    template <class T>
    T Read() {
        T Result;
        std::memcpy(&Result, Code, sizeof(T));
        Code += sizeof(T);
        return Result;
    }
    template <class TNumericType>
    TNumericType ReadInt() {
        return Read<TNumericType>();
    }
    float ReadFloat() { return Read<float>(); }
    double ReadDouble() { return Read<double>(); }
    int32 ReadWord() { return Read<uint16>(); }
    UObject* ReadObject() { return reinterpret_cast<UObject*>(static_cast<uintptr_t>(Read<ScriptPointerType>())); }
    FProperty* ReadProperty() {
        FProperty* Result = reinterpret_cast<FProperty*>(static_cast<uintptr_t>(Read<ScriptPointerType>()));
        MostRecentProperty = Result;
        return Result;
    }
    FProperty* ReadPropertyUnchecked() { return ReadProperty(); }
    FField* ReadField() { return reinterpret_cast<FField*>(static_cast<uintptr_t>(Read<ScriptPointerType>())); }
    FName ReadName();
    CodeSkipSizeType ReadCodeSkipCount() { return Read<CodeSkipSizeType>(); }
    VariableSizeType ReadVariableSize(FProperty** ExpressionField);

    static FFrame* GetThreadLocalTopStackFrame();
    void FinishNativeParms();
    void SkipParms();
    void KismetExecutionMessage(const char* Code, std::string_view Message, EBlueprintExceptionType Type = EBlueprintExceptionType::NonFatalError) const;
};

extern int32 GMaximumScriptLoopIterations;
extern int32 GScriptRecurseLimit;

void ProcessLocalScriptFunction(UObject* Context, FFrame& Stack, RESULT_DECL);

}

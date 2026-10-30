#pragma once

#include <cstring>

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"

namespace ue5parse::runtime {

struct FOutParmRec {
    FProperty* Property;
    uint8* PropAddr;
    FOutParmRec* NextOutParm;
};

// FFrame: one activation of a script function: the code pointer into its bytecode, its locals, and the address of the
// last property expression evaluated (so that out parameters and assignments can take the address).
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
    // For a native function called from C++ (ProcessEvent), its parameters, read in order by the P_GET macros.
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

    // Runs the next expression: its opcode's exec function.
    void Step(UObject* Context, RESULT_DECL) {
        int32 B = *Code++;
        GNatives[B](Context, *this, RESULT_PARAM);
    }
    bool StepAndCheckMostRecentProperty(UObject* Context, RESULT_DECL) {
        Step(Context, RESULT_PARAM);
        return MostRecentProperty && MostRecentPropertyAddress;
    }
    void StepExplicitProperty(void* const Result, FProperty* Property);
    // A native function's next parameter: from the bytecode, or from the compiled-in property chain when it was called
    // from C++.
    void StepCompiledIn(void* Result) {
        if (Code) {
            Step(Object, Result);
        } else {
            FProperty* Property = static_cast<FProperty*>(PropertyChainForCompiledIn);
            PropertyChainForCompiledIn = Property->Next;
            StepExplicitProperty(Result, Property);
        }
    }
    // A reference parameter: the address of the caller's variable, or TemporaryBuffer when there is none.
    template <class TNativeType>
    TNativeType& StepCompiledInRef(void* const TemporaryBuffer) {
        MostRecentPropertyAddress = nullptr;
        MostRecentPropertyContainer = nullptr;
        if (Code) {
            Step(Object, TemporaryBuffer);
        } else {
            FProperty* Property = static_cast<FProperty*>(PropertyChainForCompiledIn);
            PropertyChainForCompiledIn = Property->Next;
            StepExplicitProperty(TemporaryBuffer, Property);
        }
        return MostRecentPropertyAddress != nullptr ? *reinterpret_cast<TNativeType*>(MostRecentPropertyAddress)
                                                    : *static_cast<TNativeType*>(TemporaryBuffer);
    }

    uint8 PeekCode() const { return *Code; }
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

    // "Class.Function" of each script frame, innermost first.
    FString GetStackTrace() const;
    static FFrame* GetThreadLocalTopStackFrame();
    // Reports a script problem of this frame through FBlueprintCoreDelegates::ThrowScriptException.
    void KismetExecutionMessage(const char* Code, std::string_view Message, EBlueprintExceptionType Type = EBlueprintExceptionType::NonFatalError) const;
};

// The runaway-loop and recursion limits of the VM (UE's GMaximumScriptLoopIterations and GScriptRecurseLimit).
extern int32 GMaximumScriptLoopIterations;
extern int32 GScriptRecurseLimit;

// Runs a bytecode function called from bytecode: sets up its frame from the caller's parameter expressions.
void ProcessLocalScriptFunction(UObject* Context, FFrame& Stack, RESULT_DECL);

}

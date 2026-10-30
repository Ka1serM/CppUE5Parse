#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/ObjectMacros.h"

namespace ue5parse::runtime {

class UObject;
struct FFrame;

// The bytecode of a function in memory, as UE executes it: object and property operands are 8-byte pointers, names are
// 12-byte slots, so that the jump offsets of the cooked bytecode are valid as they are.
using ScriptPointerType = uint64;
using CodeSkipSizeType = uint32;
using VariableSizeType = uint16;
constexpr int32 ScriptNameSize = 12;

// The size of the buffer the VM gives statements whose result nobody reads.
constexpr int32 MAX_SIMPLE_RETURN_VALUE_SIZE = 64;

// A native function or an exec function of the VM: (context object, frame, result address).
#define RESULT_DECL void* const RESULT_PARAM
using FNativeFuncPtr = void (*)(UObject* Context, FFrame& Stack, RESULT_DECL);

enum EFunctionFlags : uint32 {
    FUNC_None = 0x00000000,
    FUNC_Final = 0x00000001,
    FUNC_RequiredAPI = 0x00000002,
    FUNC_BlueprintAuthorityOnly = 0x00000004,
    FUNC_BlueprintCosmetic = 0x00000008,
    FUNC_Net = 0x00000040,
    FUNC_NetReliable = 0x00000080,
    FUNC_NetRequest = 0x00000100,
    FUNC_Exec = 0x00000200,
    FUNC_Native = 0x00000400,
    FUNC_Event = 0x00000800,
    FUNC_NetResponse = 0x00001000,
    FUNC_Static = 0x00002000,
    FUNC_NetMulticast = 0x00004000,
    FUNC_UbergraphFunction = 0x00008000,
    FUNC_MulticastDelegate = 0x00010000,
    FUNC_Public = 0x00020000,
    FUNC_Private = 0x00040000,
    FUNC_Protected = 0x00080000,
    FUNC_Delegate = 0x00100000,
    FUNC_NetServer = 0x00200000,
    FUNC_HasOutParms = 0x00400000,
    FUNC_HasDefaults = 0x00800000,
    FUNC_NetClient = 0x01000000,
    FUNC_DLLImport = 0x02000000,
    FUNC_BlueprintCallable = 0x04000000,
    FUNC_BlueprintEvent = 0x08000000,
    FUNC_BlueprintPure = 0x10000000,
    FUNC_EditorOnly = 0x20000000,
    FUNC_Const = 0x40000000,
    FUNC_NetValidate = 0x80000000,
};
constexpr uint32 FUNC_NetFuncFlags = FUNC_Net | FUNC_NetReliable | FUNC_NetServer | FUNC_NetClient | FUNC_NetMulticast;

enum EExprToken : uint8 {
    EX_LocalVariable = 0x00,
    EX_InstanceVariable = 0x01,
    EX_DefaultVariable = 0x02,
    EX_Return = 0x04,
    EX_Jump = 0x06,
    EX_JumpIfNot = 0x07,
    EX_Assert = 0x09,
    EX_Nothing = 0x0B,
    EX_NothingInt32 = 0x0C,
    EX_Let = 0x0F,
    EX_BitFieldConst = 0x11,
    EX_ClassContext = 0x12,
    EX_MetaCast = 0x13,
    EX_LetBool = 0x14,
    EX_EndParmValue = 0x15,
    EX_EndFunctionParms = 0x16,
    EX_Self = 0x17,
    EX_Skip = 0x18,
    EX_Context = 0x19,
    EX_Context_FailSilent = 0x1A,
    EX_VirtualFunction = 0x1B,
    EX_FinalFunction = 0x1C,
    EX_IntConst = 0x1D,
    EX_FloatConst = 0x1E,
    EX_StringConst = 0x1F,
    EX_ObjectConst = 0x20,
    EX_NameConst = 0x21,
    EX_RotationConst = 0x22,
    EX_VectorConst = 0x23,
    EX_ByteConst = 0x24,
    EX_IntZero = 0x25,
    EX_IntOne = 0x26,
    EX_True = 0x27,
    EX_False = 0x28,
    EX_TextConst = 0x29,
    EX_NoObject = 0x2A,
    EX_TransformConst = 0x2B,
    EX_IntConstByte = 0x2C,
    EX_NoInterface = 0x2D,
    EX_DynamicCast = 0x2E,
    EX_StructConst = 0x2F,
    EX_EndStructConst = 0x30,
    EX_SetArray = 0x31,
    EX_EndArray = 0x32,
    EX_PropertyConst = 0x33,
    EX_UnicodeStringConst = 0x34,
    EX_Int64Const = 0x35,
    EX_UInt64Const = 0x36,
    EX_DoubleConst = 0x37,
    EX_Cast = 0x38,
    EX_SetSet = 0x39,
    EX_EndSet = 0x3A,
    EX_SetMap = 0x3B,
    EX_EndMap = 0x3C,
    EX_SetConst = 0x3D,
    EX_EndSetConst = 0x3E,
    EX_MapConst = 0x3F,
    EX_EndMapConst = 0x40,
    EX_Vector3fConst = 0x41,
    EX_StructMemberContext = 0x42,
    EX_LetMulticastDelegate = 0x43,
    EX_LetDelegate = 0x44,
    EX_LocalVirtualFunction = 0x45,
    EX_LocalFinalFunction = 0x46,
    EX_LocalOutVariable = 0x48,
    EX_DeprecatedOp4A = 0x4A,
    EX_InstanceDelegate = 0x4B,
    EX_PushExecutionFlow = 0x4C,
    EX_PopExecutionFlow = 0x4D,
    EX_ComputedJump = 0x4E,
    EX_PopExecutionFlowIfNot = 0x4F,
    EX_Breakpoint = 0x50,
    EX_InterfaceContext = 0x51,
    EX_ObjToInterfaceCast = 0x52,
    EX_EndOfScript = 0x53,
    EX_CrossInterfaceCast = 0x54,
    EX_InterfaceToObjCast = 0x55,
    EX_WireTracepoint = 0x5A,
    EX_SkipOffsetConst = 0x5B,
    EX_AddMulticastDelegate = 0x5C,
    EX_ClearMulticastDelegate = 0x5D,
    EX_Tracepoint = 0x5E,
    EX_LetObj = 0x5F,
    EX_LetWeakObjPtr = 0x60,
    EX_BindDelegate = 0x61,
    EX_RemoveMulticastDelegate = 0x62,
    EX_CallMulticastDelegate = 0x63,
    EX_LetValueOnPersistentFrame = 0x64,
    EX_ArrayConst = 0x65,
    EX_EndArrayConst = 0x66,
    EX_SoftObjectConst = 0x67,
    EX_CallMath = 0x68,
    EX_SwitchValue = 0x69,
    EX_InstrumentationEvent = 0x6A,
    EX_ArrayGetByRef = 0x6B,
    EX_ClassSparseDataVariable = 0x6C,
    EX_FieldPathConst = 0x6D,
    EX_AutoRtfmTransact = 0x70,
    EX_AutoRtfmStopTransact = 0x71,
    EX_AutoRtfmAbortIfNot = 0x72,
    EX_Max = 0xFF,
};

enum ECastToken : uint8 {
    CST_ObjectToInterface = 0x00,
    CST_ObjectToBool = 0x01,
    CST_InterfaceToBool = 0x02,
    CST_DoubleToFloat = 0x03,
    CST_FloatToDouble = 0x04,
    CST_Max = 0xFF,
};

enum class EBlueprintTextLiteralType : uint8 {
    Empty,
    LocalizedText,
    InvariantText,
    LiteralString,
    StringTableEntry,
};

// The exec functions of the opcodes and of the casts. Filled by the CoreUObject module's initialization.
extern FNativeFuncPtr GNatives[EX_Max];
extern FNativeFuncPtr GCasts[CST_Max];

// What a script did wrong, with the script stack: UE's FBlueprintExceptionInfo through
// FBlueprintCoreDelegates::ThrowScriptException. The default handler counts them; the application sets its own.
enum class EBlueprintExceptionType : uint8 {
    Breakpoint,
    Tracepoint,
    WireTracepoint,
    AccessViolation,
    InfiniteLoop,
    NonFatalError,
    FatalError,
    AbortExecution,
};

struct FBlueprintExceptionInfo {
    EBlueprintExceptionType Type;
    // A short code ("AccessNoneContext", "MissingNative") and the message, as UE's LOCTEXT key and text.
    const char* Code;
    std::string_view Message;
};

using FScriptExceptionHandler = void (*)(const UObject* ActiveObject, const FFrame& StackFrame, const FBlueprintExceptionInfo& Info);

namespace FBlueprintCoreDelegates {
void ThrowScriptException(const UObject* ActiveObject, const FFrame& StackFrame, const FBlueprintExceptionInfo& Info);
void SetScriptExceptionHandler(FScriptExceptionHandler Handler);
}

}

#pragma once

#include <type_traits>

#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Script.h"

namespace ue5parse::runtime {

class FArchive;
class FLinkerLoad;
class FObjectInitializer;
class UClass;
class UFunction;
class UPackage;
class UWorld;
struct FFrame;
struct FOutParmRec;

#define DECLARE_FUNCTION(func) static void func(UObject* Context, FFrame& Stack, RESULT_DECL)
#define DEFINE_FUNCTION(func) void func(UObject* Context, FFrame& Stack, RESULT_DECL)

#define DECLARE_CLASS(TClass, TSuperClass, TStaticCastFlags)                                                           \
public:                                                                                                                \
    using Super = TSuperClass;                                                                                         \
    using ThisClass = TClass;                                                                                          \
    static constexpr uint64 StaticClassCastFlags = TStaticCastFlags;                                                   \
    static UClass* StaticClass();

namespace FunctionCallspace {

enum Type { Absorbed = 0x0, Remote = 0x1, Local = 0x2 };

}

class UObjectBase {
public:
    UObjectBase();
    virtual ~UObjectBase();
    UObjectBase(const UObjectBase&) = delete;
    UObjectBase& operator=(const UObjectBase&) = delete;

    UClass* GetClass() const { return ClassPrivate; }
    UObject* GetOuter() const { return OuterPrivate; }
    FName GetFName() const { return NamePrivate; }
    EObjectFlags GetFlags() const { return ObjectFlags; }
    void SetFlags(EObjectFlags Flags) { ObjectFlags = ObjectFlags | Flags; }
    void ClearFlags(EObjectFlags Flags) { ObjectFlags = ObjectFlags & ~Flags; }
    bool HasAnyFlags(EObjectFlags Flags) const { return (ObjectFlags & Flags) != 0; }
    bool HasAllFlags(EObjectFlags Flags) const { return (ObjectFlags & Flags) == Flags; }
    uint32 GetUniqueID() const { return static_cast<uint32>(InternalIndex); }
    int32 GetInternalIndex() const { return InternalIndex; }
    FLinkerLoad* GetLinker() const { return Linker; }
    int32 GetLinkerIndex() const { return LinkerIndex; }

protected:
    friend class FObjectInitializer;
    friend class FLinkerLoad;
    friend UObject* StaticAllocateObject(const UClass*, UObject*, FName, EObjectFlags);
    friend void DestroyObject(UObjectBase*);
    friend void RenameObject(UObjectBase*, FName, UObject*);

    EObjectFlags ObjectFlags;
    int32 InternalIndex;
    UClass* ClassPrivate;
    FName NamePrivate;
    UObject* OuterPrivate;
    FLinkerLoad* Linker = nullptr;
    int32 LinkerIndex = INDEX_NONE;
};

class UObject : public UObjectBase {
public:
    using Super = UObject;
    using ThisClass = UObject;
    static constexpr uint64 StaticClassCastFlags = 0;
    static UClass* StaticClass();
    UObject() = default;
    explicit UObject(const FObjectInitializer&) {}

    FString GetName() const { return NameToString(GetFName()); }
    FString GetPathName(const UObject* StopOuter = nullptr) const;
    FString GetFullName() const;
    UPackage* GetOutermost() const;
    UPackage* GetPackage() const { return GetOutermost(); }
    bool IsA(const UClass* SomeBase) const;
    template <class T>
    bool IsA() const;
    UObject* GetTypedOuter(const UClass* Target) const;
    template <class T>
    T* GetTypedOuter() const {
        return static_cast<T*>(GetTypedOuter(T::StaticClass()));
    }
    bool IsInOuter(const UObject* SomeOuter) const;
    bool IsTemplate(EObjectFlags TemplateTypes = EObjectFlags(RF_ArchetypeObject | RF_ClassDefaultObject)) const;
    UObject* GetArchetype() const;
    void MarkAsGarbage() { SetFlags(RF_Garbage); }
    bool IsGarbage() const { return HasAnyFlags(RF_Garbage); }

    virtual void PostInitProperties() {}
    virtual void PreNetReceive() {}
    virtual void PostNetReceive() {}
    virtual void PostRepNotifies() {}
    virtual void Serialize(FArchive& Ar);
    void SerializeScriptProperties(FArchive& Ar);
    virtual void PostLoad() {}
    void ConditionalPostLoad();
    virtual void BeginDestroy() { SetFlags(RF_BeginDestroyed); }
    virtual void FinishDestroy() { SetFlags(RF_FinishDestroyed); }
    virtual UWorld* GetWorld() const;

    UFunction* FindFunction(FName InName) const;
    UFunction* FindFunctionChecked(FName InName) const;

    virtual void ProcessEvent(UFunction* Function, void* Parms);
    virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) { return FunctionCallspace::Local; }
    virtual bool CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack) { return false; }

    void CallFunction(FFrame& Stack, RESULT_DECL, UFunction* Function);
    void SkipFunction(FFrame& Stack, RESULT_DECL, UFunction* Function);
    void ProcessContextOpcode(FFrame& Stack, RESULT_DECL, bool bCanFailSilently);

    DECLARE_FUNCTION(ProcessInternal);
    DECLARE_FUNCTION(execUndefined);
    DECLARE_FUNCTION(execLocalVariable);
    DECLARE_FUNCTION(execInstanceVariable);
    DECLARE_FUNCTION(execDefaultVariable);
    DECLARE_FUNCTION(execLocalOutVariable);
    DECLARE_FUNCTION(execClassSparseDataVariable);
    DECLARE_FUNCTION(execInterfaceContext);
    DECLARE_FUNCTION(execEndFunctionParms);
    DECLARE_FUNCTION(execNothing);
    DECLARE_FUNCTION(execNothingInt32);
    DECLARE_FUNCTION(execNothingOp4a);
    DECLARE_FUNCTION(execBreakpoint);
    DECLARE_FUNCTION(execTracepoint);
    DECLARE_FUNCTION(execWireTracepoint);
    DECLARE_FUNCTION(execInstrumentation);
    DECLARE_FUNCTION(execEndOfScript);
    DECLARE_FUNCTION(execJump);
    DECLARE_FUNCTION(execJumpIfNot);
    DECLARE_FUNCTION(execAssert);
    DECLARE_FUNCTION(execPushExecutionFlow);
    DECLARE_FUNCTION(execPopExecutionFlow);
    DECLARE_FUNCTION(execComputedJump);
    DECLARE_FUNCTION(execPopExecutionFlowIfNot);
    DECLARE_FUNCTION(execLet);
    DECLARE_FUNCTION(execLetObj);
    DECLARE_FUNCTION(execLetWeakObjPtr);
    DECLARE_FUNCTION(execLetBool);
    DECLARE_FUNCTION(execLetDelegate);
    DECLARE_FUNCTION(execLetMulticastDelegate);
    DECLARE_FUNCTION(execAddMulticastDelegate);
    DECLARE_FUNCTION(execClearMulticastDelegate);
    DECLARE_FUNCTION(execRemoveMulticastDelegate);
    DECLARE_FUNCTION(execSelf);
    DECLARE_FUNCTION(execContext);
    DECLARE_FUNCTION(execContext_FailSilent);
    DECLARE_FUNCTION(execStructMemberContext);
    DECLARE_FUNCTION(execVirtualFunction);
    DECLARE_FUNCTION(execFinalFunction);
    DECLARE_FUNCTION(execLocalVirtualFunction);
    DECLARE_FUNCTION(execLocalFinalFunction);
    DECLARE_FUNCTION(execCallMathFunction);
    DECLARE_FUNCTION(execIntConst);
    DECLARE_FUNCTION(execInt64Const);
    DECLARE_FUNCTION(execUInt64Const);
    DECLARE_FUNCTION(execSkipOffsetConst);
    DECLARE_FUNCTION(execFloatConst);
    DECLARE_FUNCTION(execDoubleConst);
    DECLARE_FUNCTION(execStringConst);
    DECLARE_FUNCTION(execUnicodeStringConst);
    DECLARE_FUNCTION(execTextConst);
    DECLARE_FUNCTION(execPropertyConst);
    DECLARE_FUNCTION(execObjectConst);
    DECLARE_FUNCTION(execSoftObjectConst);
    DECLARE_FUNCTION(execFieldPathConst);
    DECLARE_FUNCTION(execInstanceDelegate);
    DECLARE_FUNCTION(execNameConst);
    DECLARE_FUNCTION(execByteConst);
    DECLARE_FUNCTION(execIntZero);
    DECLARE_FUNCTION(execIntOne);
    DECLARE_FUNCTION(execTrue);
    DECLARE_FUNCTION(execFalse);
    DECLARE_FUNCTION(execNoObject);
    DECLARE_FUNCTION(execNullInterface);
    DECLARE_FUNCTION(execIntConstByte);
    DECLARE_FUNCTION(execRotationConst);
    DECLARE_FUNCTION(execVectorConst);
    DECLARE_FUNCTION(execVector3fConst);
    DECLARE_FUNCTION(execTransformConst);
    DECLARE_FUNCTION(execStructConst);
    DECLARE_FUNCTION(execSetArray);
    DECLARE_FUNCTION(execSetSet);
    DECLARE_FUNCTION(execSetMap);
    DECLARE_FUNCTION(execArrayConst);
    DECLARE_FUNCTION(execSetConst);
    DECLARE_FUNCTION(execMapConst);
    DECLARE_FUNCTION(execBitFieldConst);
    DECLARE_FUNCTION(execClassContext);
    DECLARE_FUNCTION(execCast);
    DECLARE_FUNCTION(execDynamicCast);
    DECLARE_FUNCTION(execMetaCast);
    DECLARE_FUNCTION(execInterfaceCast);
    DECLARE_FUNCTION(execDoubleToFloatCast);
    DECLARE_FUNCTION(execFloatToDoubleCast);
    DECLARE_FUNCTION(execObjectToBool);
    DECLARE_FUNCTION(execInterfaceToBool);
    DECLARE_FUNCTION(execObjectToInterface);
    DECLARE_FUNCTION(execInterfaceToInterface);
    DECLARE_FUNCTION(execInterfaceToObject);
    DECLARE_FUNCTION(execBindDelegate);
    DECLARE_FUNCTION(execCallMulticastDelegate);
    DECLARE_FUNCTION(execLetValueOnPersistentFrame);
    DECLARE_FUNCTION(execSwitchValue);
    DECLARE_FUNCTION(execArrayGetByRef);
    DECLARE_FUNCTION(execAutoRtfmTransact);
    DECLARE_FUNCTION(execAutoRtfmStopTransact);
    DECLARE_FUNCTION(execAutoRtfmAbortIfNot);
};

inline bool IsValid(const UObject* Test) { return Test != nullptr && !Test->IsGarbage(); }
FString GetNameSafe(const UObject* Object);
FString GetPathNameSafe(const UObject* Object);

bool IsObjectOfCastFlags(const UObject* Object, uint64 CastFlags);

template <class T>
bool IsA_Cast(const UObject* Src) {
    if constexpr (std::is_same_v<T, UObject>) {
        return true;
    } else if constexpr (T::StaticClassCastFlags != 0) {
        return IsObjectOfCastFlags(Src, T::StaticClassCastFlags);
    } else {
        return Src->IsA(T::StaticClass());
    }
}
template <class T>
T* Cast(UObject* Src) {
    return Src && IsA_Cast<T>(Src) ? static_cast<T*>(Src) : nullptr;
}
template <class T>
const T* Cast(const UObject* Src) {
    return Src && IsA_Cast<T>(Src) ? static_cast<const T*>(Src) : nullptr;
}
template <class T>
bool UObject::IsA() const {
    return IsA_Cast<T>(this);
}

template <class T>
T* CastChecked(UObject* Src) {
    T* Result = Cast<T>(Src);
    UE5PARSE_CHECK(Src == nullptr || Result != nullptr);
    return Result;
}

}

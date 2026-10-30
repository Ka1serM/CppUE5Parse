#include "ue5parse/runtime/Engine/Kismet/KismetSystemLibrary.h"

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"
#include "ue5parse/runtime/CoreUObject/UObject/ScriptMacros.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/LatentActions.h"

namespace ue5parse::runtime {

namespace {

UWorld* GetWorldFromContextObject(const UObject* WorldContextObject) {
    return WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
}

void ReportScriptError(std::string_view Message) {
    if (const FFrame* Frame = FFrame::GetThreadLocalTopStackFrame()) Frame->KismetExecutionMessage("KismetSystem", Message);
}

// A latent asset load: the runtime loads synchronously, so the load is complete on the first update, which calls the
// delegate with the asset and resumes the node (UE's FLoadAssetActionBase).
class FLoadAssetAction : public FPendingLatentAction {
public:
    FLoadAssetAction(const FSoftObjectPath& InSoftObjectPath, const FScriptDelegate& InOnLoaded, const FLatentActionInfo& InLatentInfo)
        : SoftObjectPath(InSoftObjectPath), OnLoaded(InOnLoaded), ExecutionFunction(InLatentInfo.ExecutionFunction),
          OutputLink(InLatentInfo.Linkage), CallbackTarget(InLatentInfo.CallbackTarget) {
        SoftObjectPath.TryLoad();
    }
    void UpdateOperation(FLatentResponse& Response) override {
        struct {
            UObject* Loaded;
        } Parms{SoftObjectPath.ResolveObject()};
        OnLoaded.ProcessDelegate(&Parms);
        Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
    }

private:
    FSoftObjectPath SoftObjectPath;
    FScriptDelegate OnLoaded;
    FName ExecutionFunction;
    int32 OutputLink;
    FWeakObjectPtr CallbackTarget;
};

void AddLoadAssetAction(const UObject* WorldContextObject, const FSoftObjectPath& Path, const FScriptDelegate& OnLoaded, const FLatentActionInfo& LatentInfo) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (World == nullptr) return;
    FLatentActionManager& LatentManager = World->GetLatentActionManager();
    if (LatentManager.FindExistingAction<FLoadAssetAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) != nullptr) return;
    LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, std::make_unique<FLoadAssetAction>(Path, OnLoaded, LatentInfo));
}

template <class TProperty>
TProperty* FindPropertyOf(UObject* Object, FName PropertyName) {
    return Object != nullptr ? FindFProperty<TProperty>(Object->GetClass(), PropertyName) : nullptr;
}

template <class TProperty>
void SetPropertyByName(UObject* Object, FName PropertyName, const typename TProperty::TCppType& Value) {
    if (TProperty* Property = FindPropertyOf<TProperty>(Object, PropertyName)) Property->SetPropertyValue_InContainer(Object, Value);
}

}

// --- Objects ---

bool UKismetSystemLibrary::IsValid(const UObject* Object) { return ::ue5parse::runtime::IsValid(Object); }
bool UKismetSystemLibrary::IsValidClass(UClass* Class) { return ::ue5parse::runtime::IsValid(Class); }
FString UKismetSystemLibrary::GetObjectName(const UObject* Object) { return Object != nullptr ? Object->GetName() : FString("None"); }
FString UKismetSystemLibrary::GetPathName(const UObject* Object) { return GetPathNameSafe(Object); }
// Without the editor an object's display name is its name.
FString UKismetSystemLibrary::GetDisplayName(const UObject* Object) { return Object != nullptr ? Object->GetName() : FString(); }
FString UKismetSystemLibrary::GetClassDisplayName(const UClass* Class) { return Class != nullptr ? Class->GetName() : FString(); }
FSoftClassPath UKismetSystemLibrary::GetSoftClassPath(const UClass* Class) { return FSoftClassPath(Class); }
UObject* UKismetSystemLibrary::GetOuterObject(const UObject* Object) { return Object != nullptr ? Object->GetOuter() : nullptr; }
bool UKismetSystemLibrary::DoesImplementInterface(const UObject* TestObject, TSubclassOf<UInterface> Interface) {
    return Interface.Get() != nullptr && TestObject != nullptr && TestObject->GetClass()->ImplementsInterface(Interface);
}
UObject* UKismetSystemLibrary::Conv_InterfaceToObject(const FScriptInterface& Interface) { return Interface.GetObject(); }

// --- World ---

double UKismetSystemLibrary::GetGameTimeInSeconds(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr ? World->GetTimeSeconds() : 0.0;
}
bool UKismetSystemLibrary::IsServer(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr && World->GetNetMode() != NM_Client;
}
bool UKismetSystemLibrary::IsDedicatedServer(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr && World->GetNetMode() == NM_DedicatedServer;
}
bool UKismetSystemLibrary::IsStandalone(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr && World->GetNetMode() == NM_Standalone;
}
// The cooked game is a shipping build.
bool UKismetSystemLibrary::IsPackagedForDistribution() { return true; }

// --- Soft references ---

FSoftObjectPath UKismetSystemLibrary::MakeSoftObjectPath(const FString& PathString) {
    FSoftObjectPath SoftObjectPath(PathString);
    if (!PathString.IsEmpty() && !SoftObjectPath.IsValid()) ReportScriptError("MakeSoftObjectPath: the string is no valid path");
    return SoftObjectPath;
}
void UKismetSystemLibrary::BreakSoftObjectPath(FSoftObjectPath InSoftObjectPath, FString& PathString) { PathString = InSoftObjectPath.ToString(); }
TSoftObjectPtr<UObject> UKismetSystemLibrary::Conv_SoftObjPathToSoftObjRef(const FSoftObjectPath& SoftObjectPath) {
    return TSoftObjectPtr<UObject>(SoftObjectPath);
}
FSoftObjectPath UKismetSystemLibrary::Conv_SoftObjRefToSoftObjPath(TSoftObjectPtr<UObject> SoftObjectReference) {
    return SoftObjectReference.ToSoftObjectPath();
}
bool UKismetSystemLibrary::IsValidSoftObjectReference(const TSoftObjectPtr<UObject>& SoftObjectReference) { return !SoftObjectReference.IsNull(); }
FString UKismetSystemLibrary::Conv_SoftObjectReferenceToString(const TSoftObjectPtr<UObject>& SoftObjectReference) {
    return SoftObjectReference.ToString();
}
UObject* UKismetSystemLibrary::LoadAsset_Blocking(TSoftObjectPtr<UObject> Asset) { return Asset.LoadSynchronous(); }
bool UKismetSystemLibrary::IsValidSoftClassReference(const TSoftClassPtr<UObject>& SoftClassReference) { return !SoftClassReference.IsNull(); }
FString UKismetSystemLibrary::Conv_SoftClassReferenceToString(const TSoftClassPtr<UObject>& SoftClassReference) {
    return SoftClassReference.ToString();
}
bool UKismetSystemLibrary::EqualEqual_SoftClassReference(const TSoftClassPtr<UObject>& A, const TSoftClassPtr<UObject>& B) { return A == B; }
bool UKismetSystemLibrary::NotEqual_SoftClassReference(const TSoftClassPtr<UObject>& A, const TSoftClassPtr<UObject>& B) { return !(A == B); }
UClass* UKismetSystemLibrary::LoadClassAsset_Blocking(TSoftClassPtr<UObject> AssetClass) { return AssetClass.LoadSynchronous(); }
UObject* UKismetSystemLibrary::Conv_SoftObjectReferenceToObject(const TSoftObjectPtr<UObject>& SoftObject) { return SoftObject.Get(); }
TSubclassOf<UObject> UKismetSystemLibrary::Conv_SoftClassReferenceToClass(const TSoftClassPtr<UObject>& SoftClass) { return SoftClass.Get(); }
TSoftObjectPtr<UObject> UKismetSystemLibrary::Conv_ObjectToSoftObjectReference(UObject* Object) { return TSoftObjectPtr<UObject>(Object); }
TSoftClassPtr<UObject> UKismetSystemLibrary::Conv_ClassToSoftClassReference(const TSubclassOf<UObject>& Class) {
    return TSoftClassPtr<UObject>(FSoftObjectPath(Class.Get()));
}
void UKismetSystemLibrary::LoadAsset(const UObject* WorldContextObject, TSoftObjectPtr<UObject> Asset, FOnAssetLoaded OnLoaded, FLatentActionInfo LatentInfo) {
    AddLoadAssetAction(WorldContextObject, Asset.ToSoftObjectPath(), OnLoaded, LatentInfo);
}
void UKismetSystemLibrary::LoadAssetClass(const UObject* WorldContextObject, TSoftClassPtr<UObject> AssetClass, FOnAssetClassLoaded OnLoaded,
                                          FLatentActionInfo LatentInfo) {
    AddLoadAssetAction(WorldContextObject, AssetClass.ToSoftObjectPath(), OnLoaded, LatentInfo);
}

// --- Literals and printing ---

int32 UKismetSystemLibrary::MakeLiteralInt(int32 Value) { return Value; }
double UKismetSystemLibrary::MakeLiteralDouble(double Value) { return Value; }
bool UKismetSystemLibrary::MakeLiteralBool(bool Value) { return Value; }
FName UKismetSystemLibrary::MakeLiteralName(FName Value) { return Value; }
uint8 UKismetSystemLibrary::MakeLiteralByte(uint8 Value) { return Value; }
FString UKismetSystemLibrary::MakeLiteralString(FString Value) { return Value; }
FText UKismetSystemLibrary::MakeLiteralText(FText Value) { return Value; }
void UKismetSystemLibrary::PrintString(const UObject*, const FString&, bool, bool, FLinearColor, float, const FName) {}
void UKismetSystemLibrary::PrintText(const UObject*, const FText, bool, bool, FLinearColor, float, const FName) {}
void UKismetSystemLibrary::PrintWarning(const FString&) {}
void UKismetSystemLibrary::LogString(const FString&, bool) {}

// --- Latent actions ---

void UKismetSystemLibrary::Delay(const UObject* WorldContextObject, float Duration, FLatentActionInfo LatentInfo) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (World == nullptr) return;
    FLatentActionManager& LatentManager = World->GetLatentActionManager();
    if (LatentManager.FindExistingAction<FDelayAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) != nullptr) return;
    LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, std::make_unique<FDelayAction>(Duration, LatentInfo));
}
void UKismetSystemLibrary::DelayUntilNextTick(const UObject* WorldContextObject, FLatentActionInfo LatentInfo) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (World == nullptr) return;
    FLatentActionManager& LatentManager = World->GetLatentActionManager();
    if (LatentManager.FindExistingAction<FDelayUntilNextTickAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) != nullptr) return;
    LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, std::make_unique<FDelayUntilNextTickAction>(LatentInfo));
}
// A running delay starts over.
void UKismetSystemLibrary::RetriggerableDelay(const UObject* WorldContextObject, float Duration, FLatentActionInfo LatentInfo) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (World == nullptr) return;
    FLatentActionManager& LatentManager = World->GetLatentActionManager();
    if (FDelayAction* Action = LatentManager.FindExistingAction<FDelayAction>(LatentInfo.CallbackTarget, LatentInfo.UUID)) {
        Action->TimeRemaining = Duration;
        return;
    }
    LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, std::make_unique<FDelayAction>(Duration, LatentInfo));
}

// --- Timers ---

// The first call comes after Time plus the start delay, which varies by up to the variance either way.
FTimerHandle UKismetSystemLibrary::K2_SetTimerDelegate(FTimerDynamicDelegate Delegate, float Time, bool bLooping, float InitialStartDelay,
                                                       float InitialStartDelayVariance) {
    FTimerHandle Handle;
    if (!Delegate.IsBound()) {
        ReportScriptError("SetTimer passed a bad function");
        return Handle;
    }
    UWorld* World = GetWorldFromContextObject(Delegate.GetUObject());
    if (World == nullptr) return Handle;
    if (InitialStartDelayVariance < 0.f) {
        ReportScriptError("SetTimer passed a negative InitialStartDelayVariance");
        return Handle;
    }
    InitialStartDelay += static_cast<float>(FMath::FRandRange(-InitialStartDelayVariance, InitialStartDelayVariance));
    if (Time <= 0.f || (Time + InitialStartDelay) - InitialStartDelayVariance < 0.f) {
        ReportScriptError("SetTimer passed a negative or zero time; the timer is cleared");
    }
    FTimerManager& TimerManager = World->GetTimerManager();
    Handle = TimerManager.K2_FindDynamicTimerHandle(Delegate);
    TimerManager.SetTimer(Handle, Delegate, Time, bLooping, Time + InitialStartDelay);
    return Handle;
}
FTimerHandle UKismetSystemLibrary::K2_SetTimerForNextTickDelegate(FTimerDynamicDelegate Delegate) {
    if (!Delegate.IsBound()) {
        ReportScriptError("SetTimerForNextTick passed a bad function");
        return FTimerHandle();
    }
    UWorld* World = GetWorldFromContextObject(Delegate.GetUObject());
    return World != nullptr ? World->GetTimerManager().SetTimerForNextTick(Delegate) : FTimerHandle();
}
FTimerHandle UKismetSystemLibrary::K2_SetTimer(UObject* Object, FString FunctionName, float Time, bool bLooping, float InitialStartDelay,
                                               float InitialStartDelayVariance) {
    const FName FunctionFName(FunctionName.ToUtf8());
    if (Object != nullptr) {
        const UFunction* Function = Object->FindFunction(FunctionFName);
        if (Function != nullptr && Function->ParmsSize > 0) {
            ReportScriptError("SetTimer passed a function that expects parameters");
            return FTimerHandle();
        }
    }
    FTimerDynamicDelegate Delegate;
    Delegate.BindUFunction(Object, FunctionFName);
    return K2_SetTimerDelegate(Delegate, Time, bLooping, InitialStartDelay, InitialStartDelayVariance);
}
void UKismetSystemLibrary::K2_ClearTimer(UObject* Object, FString FunctionName) {
    FTimerDynamicDelegate Delegate;
    Delegate.BindUFunction(Object, FName(FunctionName.ToUtf8()));
    if (!Delegate.IsBound()) return;
    UWorld* World = GetWorldFromContextObject(Delegate.GetUObject());
    if (World == nullptr) return;
    FTimerHandle Handle = World->GetTimerManager().K2_FindDynamicTimerHandle(Delegate);
    World->GetTimerManager().ClearTimer(Handle);
}
bool UKismetSystemLibrary::K2_IsValidTimerHandle(FTimerHandle Handle) { return Handle.IsValid(); }
FTimerHandle UKismetSystemLibrary::K2_InvalidateTimerHandle(FTimerHandle& Handle) {
    Handle.Invalidate();
    return Handle;
}
void UKismetSystemLibrary::K2_ClearAndInvalidateTimerHandle(const UObject* WorldContextObject, FTimerHandle& Handle) {
    if (UWorld* World = GetWorldFromContextObject(WorldContextObject)) World->GetTimerManager().ClearTimer(Handle);
}
void UKismetSystemLibrary::K2_PauseTimerHandle(const UObject* WorldContextObject, FTimerHandle Handle) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (Handle.IsValid() && World != nullptr) World->GetTimerManager().PauseTimer(Handle);
}
void UKismetSystemLibrary::K2_UnPauseTimerHandle(const UObject* WorldContextObject, FTimerHandle Handle) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (Handle.IsValid() && World != nullptr) World->GetTimerManager().UnPauseTimer(Handle);
}
bool UKismetSystemLibrary::K2_IsTimerActiveHandle(const UObject* WorldContextObject, FTimerHandle Handle) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return Handle.IsValid() && World != nullptr && World->GetTimerManager().IsTimerActive(Handle);
}
bool UKismetSystemLibrary::K2_IsTimerPausedHandle(const UObject* WorldContextObject, FTimerHandle Handle) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return Handle.IsValid() && World != nullptr && World->GetTimerManager().IsTimerPaused(Handle);
}
bool UKismetSystemLibrary::K2_TimerExistsHandle(const UObject* WorldContextObject, FTimerHandle Handle) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return Handle.IsValid() && World != nullptr && World->GetTimerManager().TimerExists(Handle);
}
float UKismetSystemLibrary::K2_GetTimerElapsedTimeHandle(const UObject* WorldContextObject, FTimerHandle Handle) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return Handle.IsValid() && World != nullptr ? World->GetTimerManager().GetTimerElapsed(Handle) : 0.f;
}
float UKismetSystemLibrary::K2_GetTimerRemainingTimeHandle(const UObject* WorldContextObject, FTimerHandle Handle) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return Handle.IsValid() && World != nullptr ? World->GetTimerManager().GetTimerRemaining(Handle) : 0.f;
}

// --- Properties by name ---

void UKismetSystemLibrary::SetIntPropertyByName(UObject* Object, FName PropertyName, int32 Value) {
    SetPropertyByName<FIntProperty>(Object, PropertyName, Value);
}
void UKismetSystemLibrary::SetBytePropertyByName(UObject* Object, FName PropertyName, uint8 Value) {
    if (FByteProperty* ByteProperty = FindPropertyOf<FByteProperty>(Object, PropertyName)) {
        ByteProperty->SetPropertyValue_InContainer(Object, Value);
    } else if (FEnumProperty* EnumProperty = FindPropertyOf<FEnumProperty>(Object, PropertyName)) {
        EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(EnumProperty->ContainerPtrToValuePtr<void>(Object), static_cast<uint64>(Value));
    }
}
// The property may be a native float.
void UKismetSystemLibrary::SetDoublePropertyByName(UObject* Object, FName PropertyName, double Value) {
    if (FDoubleProperty* DoubleProperty = FindPropertyOf<FDoubleProperty>(Object, PropertyName)) {
        DoubleProperty->SetPropertyValue_InContainer(Object, Value);
    } else if (FFloatProperty* FloatProperty = FindPropertyOf<FFloatProperty>(Object, PropertyName)) {
        FloatProperty->SetPropertyValue_InContainer(Object, static_cast<float>(Value));
    }
}
void UKismetSystemLibrary::SetBoolPropertyByName(UObject* Object, FName PropertyName, bool Value) {
    if (FBoolProperty* Property = FindPropertyOf<FBoolProperty>(Object, PropertyName)) Property->SetPropertyValue_InContainer(Object, Value);
}
void UKismetSystemLibrary::SetObjectPropertyByName(UObject* Object, FName PropertyName, UObject* Value) {
    if (Value == nullptr) return;
    FObjectPropertyBase* Property = FindPropertyOf<FObjectPropertyBase>(Object, PropertyName);
    if (Property != nullptr && Value->IsA(Property->PropertyClass)) Property->SetObjectPropertyValue_InContainer(Object, Value);
}
void UKismetSystemLibrary::SetClassPropertyByName(UObject* Object, FName PropertyName, TSubclassOf<UObject> Value) {
    if (Value.Get() == nullptr) return;
    FClassProperty* Property = FindPropertyOf<FClassProperty>(Object, PropertyName);
    if (Property != nullptr && Value->IsChildOf(Property->MetaClass)) Property->SetObjectPropertyValue_InContainer(Object, Value.Get());
}
void UKismetSystemLibrary::SetNamePropertyByName(UObject* Object, FName PropertyName, const FName& Value) {
    SetPropertyByName<FNameProperty>(Object, PropertyName, Value);
}
void UKismetSystemLibrary::SetSoftClassPropertyByName(UObject* Object, FName PropertyName, const TSoftClassPtr<UObject>& Value) {
    if (FSoftClassProperty* Property = FindPropertyOf<FSoftClassProperty>(Object, PropertyName))
        Property->SetPropertyValue_InContainer(Object, FSoftObjectPtr(Value.ToSoftObjectPath()));
}
void UKismetSystemLibrary::SetStringPropertyByName(UObject* Object, FName PropertyName, const FString& Value) {
    SetPropertyByName<FStrProperty>(Object, PropertyName, Value);
}
void UKismetSystemLibrary::SetTextPropertyByName(UObject* Object, FName PropertyName, const FText& Value) {
    SetPropertyByName<FTextProperty>(Object, PropertyName, Value);
}
void UKismetSystemLibrary::SetVectorPropertyByName(UObject* Object, FName PropertyName, const FVector& Value) {
    FStructProperty* Property = FindPropertyOf<FStructProperty>(Object, PropertyName);
    if (Property != nullptr && Property->Struct == TBaseStructure<FVector>::Get()) *Property->ContainerPtrToValuePtr<FVector>(Object) = Value;
}
void UKismetSystemLibrary::SetRotatorPropertyByName(UObject* Object, FName PropertyName, const FRotator& Value) {
    FStructProperty* Property = FindPropertyOf<FStructProperty>(Object, PropertyName);
    if (Property != nullptr && Property->Struct == TBaseStructure<FRotator>::Get()) *Property->ContainerPtrToValuePtr<FRotator>(Object) = Value;
}
// Only the custom thunk below runs.
void UKismetSystemLibrary::SetStructurePropertyByName(UObject*, FName, const FGenericStruct&) {}

// The struct parameter is of any type: the thunk takes the address of the caller's value and copies it into the
// property of the name (UE's Generic_SetStructurePropertyByName).
DEFINE_FUNCTION(UKismetSystemLibrary::execSetStructurePropertyByName) {
    P_GET_OBJECT(UObject, OwnerObject);
    P_GET_PROPERTY(FNameProperty, StructPropertyName);
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.StepCompiledIn(nullptr);
    const void* SrcStructAddr = Stack.MostRecentPropertyAddress;
    P_FINISH;
    FStructProperty* Property = FindPropertyOf<FStructProperty>(OwnerObject, StructPropertyName);
    if (Property == nullptr || SrcStructAddr == nullptr) return;
    Property->CopyValuesInternal(Property->ContainerPtrToValuePtr<void>(OwnerObject), SrcStructAddr, 1);
}

// --- Collision queries: the runtime has no physics scene, so nothing is hit or overlapped ---

bool UKismetSystemLibrary::SphereOverlapActors(const UObject*, const FVector, float, const TArray<TEnumAsByte<EObjectTypeQuery>>&, UClass*,
                                               const TArray<AActor*>&, TArray<AActor*>& OutActors) {
    OutActors.Empty();
    return false;
}
bool UKismetSystemLibrary::SphereOverlapComponents(const UObject*, const FVector, float, const TArray<TEnumAsByte<EObjectTypeQuery>>&, UClass*,
                                                   const TArray<AActor*>&, TArray<UPrimitiveComponent*>& OutComponents) {
    OutComponents.Empty();
    return false;
}
bool UKismetSystemLibrary::BoxOverlapActors(const UObject*, const FVector, FVector, const TArray<TEnumAsByte<EObjectTypeQuery>>&, UClass*,
                                            const TArray<AActor*>&, TArray<AActor*>& OutActors) {
    OutActors.Empty();
    return false;
}
bool UKismetSystemLibrary::CapsuleOverlapActors(const UObject*, const FVector, float, float, const TArray<TEnumAsByte<EObjectTypeQuery>>&, UClass*,
                                                const TArray<AActor*>&, TArray<AActor*>& OutActors) {
    OutActors.Empty();
    return false;
}
bool UKismetSystemLibrary::CapsuleOverlapComponents(const UObject*, const FVector, float, float, const TArray<TEnumAsByte<EObjectTypeQuery>>&,
                                                    UClass*, const TArray<AActor*>&, TArray<UPrimitiveComponent*>& OutComponents) {
    OutComponents.Empty();
    return false;
}
bool UKismetSystemLibrary::ComponentOverlapActors(UPrimitiveComponent*, const FTransform&, const TArray<TEnumAsByte<EObjectTypeQuery>>&, UClass*,
                                                  const TArray<AActor*>&, TArray<AActor*>& OutActors) {
    OutActors.Empty();
    return false;
}
bool UKismetSystemLibrary::ComponentOverlapComponents(UPrimitiveComponent*, const FTransform&, const TArray<TEnumAsByte<EObjectTypeQuery>>&,
                                                      UClass*, const TArray<AActor*>&, TArray<UPrimitiveComponent*>& OutComponents) {
    OutComponents.Empty();
    return false;
}
bool UKismetSystemLibrary::LineTraceSingle(const UObject*, const FVector, const FVector, ETraceTypeQuery, bool, const TArray<AActor*>&,
                                           EDrawDebugTrace::Type, FHitResult& OutHit, bool, FLinearColor, FLinearColor, float) {
    OutHit = FHitResult();
    return false;
}
bool UKismetSystemLibrary::LineTraceMulti(const UObject*, const FVector, const FVector, ETraceTypeQuery, bool, const TArray<AActor*>&,
                                          EDrawDebugTrace::Type, TArray<FHitResult>& OutHits, bool, FLinearColor, FLinearColor, float) {
    OutHits.Empty();
    return false;
}
bool UKismetSystemLibrary::SphereTraceSingle(const UObject*, const FVector, const FVector, float, ETraceTypeQuery, bool, const TArray<AActor*>&,
                                             EDrawDebugTrace::Type, FHitResult& OutHit, bool, FLinearColor, FLinearColor, float) {
    OutHit = FHitResult();
    return false;
}
bool UKismetSystemLibrary::SphereTraceMulti(const UObject*, const FVector, const FVector, float, ETraceTypeQuery, bool, const TArray<AActor*>&,
                                            EDrawDebugTrace::Type, TArray<FHitResult>& OutHits, bool, FLinearColor, FLinearColor, float) {
    OutHits.Empty();
    return false;
}
bool UKismetSystemLibrary::BoxTraceSingle(const UObject*, const FVector, const FVector, const FVector, const FRotator, ETraceTypeQuery, bool,
                                          const TArray<AActor*>&, EDrawDebugTrace::Type, FHitResult& OutHit, bool, FLinearColor, FLinearColor,
                                          float) {
    OutHit = FHitResult();
    return false;
}
bool UKismetSystemLibrary::CapsuleTraceSingle(const UObject*, const FVector, const FVector, float, float, ETraceTypeQuery, bool,
                                              const TArray<AActor*>&, EDrawDebugTrace::Type, FHitResult& OutHit, bool, FLinearColor,
                                              FLinearColor, float) {
    OutHit = FHitResult();
    return false;
}
bool UKismetSystemLibrary::LineTraceSingleForObjects(const UObject*, const FVector, const FVector, const TArray<TEnumAsByte<EObjectTypeQuery>>&,
                                                     bool, const TArray<AActor*>&, EDrawDebugTrace::Type, FHitResult& OutHit, bool, FLinearColor,
                                                     FLinearColor, float) {
    OutHit = FHitResult();
    return false;
}
bool UKismetSystemLibrary::LineTraceMultiForObjects(const UObject*, const FVector, const FVector, const TArray<TEnumAsByte<EObjectTypeQuery>>&,
                                                    bool, const TArray<AActor*>&, EDrawDebugTrace::Type, TArray<FHitResult>& OutHits, bool,
                                                    FLinearColor, FLinearColor, float) {
    OutHits.Empty();
    return false;
}
bool UKismetSystemLibrary::SphereTraceMultiForObjects(const UObject*, const FVector, const FVector, float,
                                                      const TArray<TEnumAsByte<EObjectTypeQuery>>&, bool, const TArray<AActor*>&,
                                                      EDrawDebugTrace::Type, TArray<FHitResult>& OutHits, bool, FLinearColor, FLinearColor, float) {
    OutHits.Empty();
    return false;
}
bool UKismetSystemLibrary::CapsuleTraceMultiForObjects(const UObject*, const FVector, const FVector, float, float,
                                                       const TArray<TEnumAsByte<EObjectTypeQuery>>&, bool, const TArray<AActor*>&,
                                                       EDrawDebugTrace::Type, TArray<FHitResult>& OutHits, bool, FLinearColor, FLinearColor,
                                                       float) {
    OutHits.Empty();
    return false;
}
bool UKismetSystemLibrary::SphereTraceSingleByProfile(const UObject*, const FVector, const FVector, float, FName, bool, const TArray<AActor*>&,
                                                      EDrawDebugTrace::Type, FHitResult& OutHit, bool, FLinearColor, FLinearColor, float) {
    OutHit = FHitResult();
    return false;
}

// --- Debug drawing: compiled out of a shipping build ---

void UKismetSystemLibrary::DrawDebugLine(const UObject*, const FVector, const FVector, FLinearColor, float, float) {}
void UKismetSystemLibrary::DrawDebugPoint(const UObject*, const FVector, float, FLinearColor, float) {}
void UKismetSystemLibrary::DrawDebugArrow(const UObject*, const FVector, const FVector, float, FLinearColor, float, float) {}
void UKismetSystemLibrary::DrawDebugBox(const UObject*, const FVector, FVector, FLinearColor, const FRotator, float, float) {}
void UKismetSystemLibrary::DrawDebugSphere(const UObject*, const FVector, float, int32, FLinearColor, float, float) {}
void UKismetSystemLibrary::DrawDebugCylinder(const UObject*, const FVector, const FVector, float, int32, FLinearColor, float, float) {}
void UKismetSystemLibrary::DrawDebugCapsule(const UObject*, const FVector, float, float, const FRotator, FLinearColor, float, float) {}
void UKismetSystemLibrary::DrawDebugString(const UObject*, const FVector, const FString&, AActor*, FLinearColor, float) {}
void UKismetSystemLibrary::DrawDebugPlane(const UObject*, const FPlane&, const FVector, float, FLinearColor, float) {}

}

#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"
#include "ue5parse/runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "ue5parse/runtime/CoreUObject/Templates/SubclassOf.h"
#include "ue5parse/runtime/Core/Internationalization/Text.h"
#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"
#include "ue5parse/runtime/Engine/Engine/LatentActionManager.h"
#include "ue5parse/runtime/Engine/Engine/TimerHandle.h"
#include "ue5parse/runtime/Engine/Kismet/BlueprintFunctionLibrary.h"

#include "KismetSystemLibrary.generated.h"

namespace ue5parse::runtime {

class AActor;
class APlayerController;
class UInterface;
class UPrimitiveComponent;
class USceneComponent;

namespace EDrawDebugTrace {
enum Type : int { None, ForOneFrame, ForDuration, Persistent };
}

// The parameter type of the generic struct setter, whose thunk reads the struct of any type (UE's FGenericStruct).
USTRUCT()
struct FGenericStruct {
    GENERATED_BODY()
    UPROPERTY()
    int32 Data = 0;
};

// UKismetSystemLibrary: object, timer, latent, soft reference and collision query functions of Blueprints (UE's
// Kismet/KismetSystemLibrary.h). Only the functions the cooked game calls are declared.
//
// The runtime has no physics scene: the traces and overlaps find nothing, as in a UE world without one. Debug drawing
// and printing do nothing, as in a shipping build.
UCLASS()
class UKismetSystemLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    // The delegates of the latent asset loads (UE's DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAssetLoaded, UObject*, Loaded)).
    using FOnAssetLoaded = FScriptDelegate;
    using FOnAssetClassLoaded = FScriptDelegate;

    UFUNCTION(BlueprintCallable)
    static void PrintString(const UObject* WorldContextObject, const FString& InString = FString(), bool bPrintToScreen = true,
                            bool bPrintToLog = true, FLinearColor TextColor = FLinearColor(0.0, 0.66, 1.0), float Duration = 2.f,
                            const FName Key = FName());
    UFUNCTION(BlueprintCallable)
    static void LogString(const FString& InString = FString(), bool bPrintToLog = true);
    DECLARE_FUNCTION(execSetStructurePropertyByName);
    UFUNCTION(BlueprintPure)
    static bool IsValid(const UObject* Object);
    UFUNCTION(BlueprintPure)
    static bool IsValidClass(UClass* Class);
    UFUNCTION(BlueprintPure)
    static FString GetObjectName(const UObject* Object);
    UFUNCTION(BlueprintPure)
    static FString GetPathName(const UObject* Object);
    UFUNCTION(BlueprintPure)
    static FString GetDisplayName(const UObject* Object);
    UFUNCTION(BlueprintPure)
    static FString GetClassDisplayName(const UClass* Class);
    UFUNCTION(BlueprintPure)
    static FSoftClassPath GetSoftClassPath(const UClass* Class);
    UFUNCTION(BlueprintPure)
    static UObject* GetOuterObject(const UObject* Object);
    UFUNCTION(BlueprintPure)
    static bool DoesImplementInterface(const UObject* TestObject, TSubclassOf<UInterface> Interface);
    UFUNCTION(BlueprintPure)
    static double GetGameTimeInSeconds(const UObject* WorldContextObject);
    UFUNCTION(BlueprintPure)
    static bool IsServer(const UObject* WorldContextObject);
    UFUNCTION(BlueprintPure)
    static bool IsDedicatedServer(const UObject* WorldContextObject);
    UFUNCTION(BlueprintPure)
    static bool IsStandalone(const UObject* WorldContextObject);
    UFUNCTION(BlueprintPure)
    static bool IsPackagedForDistribution();
    UFUNCTION(BlueprintPure)
    static UObject* Conv_InterfaceToObject(const FScriptInterface& Interface);
    UFUNCTION(BlueprintPure)
    static FSoftObjectPath MakeSoftObjectPath(const FString& PathString);
    UFUNCTION(BlueprintPure)
    static void BreakSoftObjectPath(FSoftObjectPath InSoftObjectPath, FString& PathString);
    UFUNCTION(BlueprintPure)
    static TSoftObjectPtr<UObject> Conv_SoftObjPathToSoftObjRef(const FSoftObjectPath& SoftObjectPath);
    UFUNCTION(BlueprintPure)
    static FSoftObjectPath Conv_SoftObjRefToSoftObjPath(TSoftObjectPtr<UObject> SoftObjectReference);
    UFUNCTION(BlueprintPure)
    static bool IsValidSoftObjectReference(const TSoftObjectPtr<UObject>& SoftObjectReference);
    UFUNCTION(BlueprintPure)
    static FString Conv_SoftObjectReferenceToString(const TSoftObjectPtr<UObject>& SoftObjectReference);
    UFUNCTION(BlueprintCallable)
    static UObject* LoadAsset_Blocking(TSoftObjectPtr<UObject> Asset);
    UFUNCTION(BlueprintPure)
    static bool IsValidSoftClassReference(const TSoftClassPtr<UObject>& SoftClassReference);
    UFUNCTION(BlueprintPure)
    static FString Conv_SoftClassReferenceToString(const TSoftClassPtr<UObject>& SoftClassReference);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_SoftClassReference(const TSoftClassPtr<UObject>& A, const TSoftClassPtr<UObject>& B);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_SoftClassReference(const TSoftClassPtr<UObject>& A, const TSoftClassPtr<UObject>& B);
    UFUNCTION(BlueprintCallable)
    static UClass* LoadClassAsset_Blocking(TSoftClassPtr<UObject> AssetClass);
    UFUNCTION(BlueprintPure)
    static UObject* Conv_SoftObjectReferenceToObject(const TSoftObjectPtr<UObject>& SoftObject);
    UFUNCTION(BlueprintPure)
    static TSubclassOf<UObject> Conv_SoftClassReferenceToClass(const TSoftClassPtr<UObject>& SoftClass);
    UFUNCTION(BlueprintPure)
    static TSoftObjectPtr<UObject> Conv_ObjectToSoftObjectReference(UObject* Object);
    UFUNCTION(BlueprintPure)
    static TSoftClassPtr<UObject> Conv_ClassToSoftClassReference(const TSubclassOf<UObject>& Class);
    UFUNCTION(BlueprintCallable)
    static void LoadAsset(const UObject* WorldContextObject, TSoftObjectPtr<UObject> Asset, FOnAssetLoaded OnLoaded, FLatentActionInfo LatentInfo);
    UFUNCTION(BlueprintCallable)
    static void LoadAssetClass(const UObject* WorldContextObject, TSoftClassPtr<UObject> AssetClass, FOnAssetClassLoaded OnLoaded, FLatentActionInfo LatentInfo);
    UFUNCTION(BlueprintPure)
    static int32 MakeLiteralInt(int32 Value);
    UFUNCTION(BlueprintPure)
    static double MakeLiteralDouble(double Value);
    UFUNCTION(BlueprintPure)
    static bool MakeLiteralBool(bool Value);
    UFUNCTION(BlueprintPure)
    static FName MakeLiteralName(FName Value);
    UFUNCTION(BlueprintPure)
    static uint8 MakeLiteralByte(uint8 Value);
    UFUNCTION(BlueprintPure)
    static FString MakeLiteralString(FString Value);
    UFUNCTION(BlueprintPure)
    static FText MakeLiteralText(FText Value);
    UFUNCTION(BlueprintCallable)
    static void PrintText(const UObject* WorldContextObject, const FText InText = FText(), bool bPrintToScreen = true, bool bPrintToLog = true, FLinearColor TextColor = FLinearColor(0.0, 0.66, 1.0), float Duration = 2.f, const FName Key = NAME_None);
    UFUNCTION(BlueprintCallable)
    static void PrintWarning(const FString& InString);
    UFUNCTION(BlueprintCallable)
    static void Delay(const UObject* WorldContextObject, float Duration, struct FLatentActionInfo LatentInfo );
    UFUNCTION(BlueprintCallable)
    static void DelayUntilNextTick(const UObject* WorldContextObject, struct FLatentActionInfo LatentInfo);
    UFUNCTION(BlueprintCallable)
    static void RetriggerableDelay(const UObject* WorldContextObject, float Duration, FLatentActionInfo LatentInfo);
    UFUNCTION(BlueprintCallable)
    static FTimerHandle K2_SetTimerDelegate(FTimerDynamicDelegate Delegate, float Time, bool bLooping, float InitialStartDelay = 0.f, float InitialStartDelayVariance = 0.f);
    UFUNCTION(BlueprintCallable)
    static FTimerHandle K2_SetTimerForNextTickDelegate(FTimerDynamicDelegate Delegate);
    UFUNCTION(BlueprintPure)
    static bool K2_IsValidTimerHandle(FTimerHandle Handle);
    UFUNCTION(BlueprintCallable)
    static FTimerHandle K2_InvalidateTimerHandle(UPARAM(ref) FTimerHandle& Handle);
    UFUNCTION(BlueprintCallable)
    static void K2_ClearAndInvalidateTimerHandle(const UObject* WorldContextObject, UPARAM(ref) FTimerHandle& Handle);
    UFUNCTION(BlueprintCallable)
    static void K2_PauseTimerHandle(const UObject* WorldContextObject, FTimerHandle Handle);
    UFUNCTION(BlueprintCallable)
    static void K2_UnPauseTimerHandle(const UObject* WorldContextObject, FTimerHandle Handle);
    UFUNCTION(BlueprintPure)
    static bool K2_IsTimerActiveHandle(const UObject* WorldContextObject, FTimerHandle Handle);
    UFUNCTION(BlueprintPure)
    static bool K2_IsTimerPausedHandle(const UObject* WorldContextObject, FTimerHandle Handle);
    UFUNCTION(BlueprintPure)
    static bool K2_TimerExistsHandle(const UObject* WorldContextObject, FTimerHandle Handle);
    UFUNCTION(BlueprintPure)
    static float K2_GetTimerElapsedTimeHandle(const UObject* WorldContextObject, FTimerHandle Handle);
    UFUNCTION(BlueprintPure)
    static float K2_GetTimerRemainingTimeHandle(const UObject* WorldContextObject, FTimerHandle Handle);
    UFUNCTION(BlueprintCallable)
    static FTimerHandle K2_SetTimer(UObject* Object, FString FunctionName, float Time, bool bLooping, float InitialStartDelay = 0.f, float InitialStartDelayVariance = 0.f);
    UFUNCTION(BlueprintCallable)
    static void K2_ClearTimer(UObject* Object, FString FunctionName);
    UFUNCTION(BlueprintCallable)
    static void SetIntPropertyByName(UObject* Object, FName PropertyName, int32 Value);
    UFUNCTION(BlueprintCallable)
    static void SetBytePropertyByName(UObject* Object, FName PropertyName, uint8 Value);
    UFUNCTION(BlueprintCallable)
    static void SetDoublePropertyByName(UObject* Object, FName PropertyName, double Value);
    UFUNCTION(BlueprintCallable)
    static void SetBoolPropertyByName(UObject* Object, FName PropertyName, bool Value);
    UFUNCTION(BlueprintCallable)
    static void SetObjectPropertyByName(UObject* Object, FName PropertyName, UObject* Value);
    UFUNCTION(BlueprintCallable)
    static void SetClassPropertyByName(UObject* Object, FName PropertyName, TSubclassOf<UObject> Value);
    UFUNCTION(BlueprintCallable)
    static void SetNamePropertyByName(UObject* Object, FName PropertyName, const FName& Value);
    UFUNCTION(BlueprintCallable)
    static void SetSoftClassPropertyByName(UObject* Object, FName PropertyName, const TSoftClassPtr<UObject>& Value);
    UFUNCTION(BlueprintCallable)
    static void SetStringPropertyByName(UObject* Object, FName PropertyName, const FString& Value);
    UFUNCTION(BlueprintCallable)
    static void SetTextPropertyByName(UObject* Object, FName PropertyName, const FText& Value);
    UFUNCTION(BlueprintCallable)
    static void SetVectorPropertyByName(UObject* Object, FName PropertyName, const FVector& Value);
    UFUNCTION(BlueprintCallable)
    static void SetRotatorPropertyByName(UObject* Object, FName PropertyName, const FRotator& Value);
    UFUNCTION(BlueprintCallable, CustomThunk)
    static void SetStructurePropertyByName(UObject* Object, FName PropertyName, const FGenericStruct& Value);
    UFUNCTION(BlueprintCallable)
    static bool SphereOverlapActors(const UObject* WorldContextObject, const FVector SpherePos, float SphereRadius, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, UClass* ActorClassFilter, const TArray<AActor*>& ActorsToIgnore, TArray<class AActor*>& OutActors);
    UFUNCTION(BlueprintCallable)
    static bool SphereOverlapComponents(const UObject* WorldContextObject, const FVector SpherePos, float SphereRadius, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, UClass* ComponentClassFilter, const TArray<AActor*>& ActorsToIgnore, TArray<class UPrimitiveComponent*>& OutComponents);
    UFUNCTION(BlueprintCallable)
    static bool BoxOverlapActors(const UObject* WorldContextObject, const FVector BoxPos, FVector BoxExtent, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, UClass* ActorClassFilter, const TArray<AActor*>& ActorsToIgnore, TArray<class AActor*>& OutActors);
    UFUNCTION(BlueprintCallable)
    static bool CapsuleOverlapActors(const UObject* WorldContextObject, const FVector CapsulePos, float Radius, float HalfHeight, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, UClass* ActorClassFilter, const TArray<AActor*>& ActorsToIgnore, TArray<class AActor*>& OutActors);
    UFUNCTION(BlueprintCallable)
    static bool CapsuleOverlapComponents(const UObject* WorldContextObject, const FVector CapsulePos, float Radius, float HalfHeight, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, UClass* ComponentClassFilter, const TArray<AActor*>& ActorsToIgnore, TArray<class UPrimitiveComponent*>& OutComponents);
    UFUNCTION(BlueprintCallable)
    static bool ComponentOverlapActors(UPrimitiveComponent* Component, const FTransform& ComponentTransform, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, UClass* ActorClassFilter, const TArray<AActor*>& ActorsToIgnore, TArray<class AActor*>& OutActors);
    UFUNCTION(BlueprintCallable)
    static bool ComponentOverlapComponents(UPrimitiveComponent* Component, const FTransform& ComponentTransform, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, UClass* ComponentClassFilter, const TArray<AActor*>& ActorsToIgnore, TArray<class UPrimitiveComponent*>& OutComponents);
    UFUNCTION(BlueprintCallable)
    static bool LineTraceSingle(const UObject* WorldContextObject, const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
    UFUNCTION(BlueprintCallable)
    static bool LineTraceMulti(const UObject* WorldContextObject, const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
    UFUNCTION(BlueprintCallable)
    static bool SphereTraceSingle(const UObject* WorldContextObject, const FVector Start, const FVector End, float Radius, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
    UFUNCTION(BlueprintCallable)
    static bool SphereTraceMulti(const UObject* WorldContextObject, const FVector Start, const FVector End, float Radius, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
    UFUNCTION(BlueprintCallable)
    static bool BoxTraceSingle(const UObject* WorldContextObject, const FVector Start, const FVector End, const FVector HalfSize, const FRotator Orientation, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
    UFUNCTION(BlueprintCallable)
    static bool CapsuleTraceSingle(const UObject* WorldContextObject, const FVector Start, const FVector End, float Radius, float HalfHeight, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
    UFUNCTION(BlueprintCallable)
    static bool LineTraceSingleForObjects(const UObject* WorldContextObject, const FVector Start, const FVector End, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f );
    UFUNCTION(BlueprintCallable)
    static bool LineTraceMultiForObjects(const UObject* WorldContextObject, const FVector Start, const FVector End, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
    UFUNCTION(BlueprintCallable)
    static bool SphereTraceMultiForObjects(const UObject* WorldContextObject, const FVector Start, const FVector End, float Radius, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
    UFUNCTION(BlueprintCallable)
    static bool CapsuleTraceMultiForObjects(const UObject* WorldContextObject, const FVector Start, const FVector End, float Radius, float HalfHeight, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
    UFUNCTION(BlueprintCallable)
    static bool SphereTraceSingleByProfile(const UObject* WorldContextObject, const FVector Start, const FVector End, float Radius, FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
    UFUNCTION(BlueprintCallable)
    static void DrawDebugLine(const UObject* WorldContextObject, const FVector LineStart, const FVector LineEnd, FLinearColor LineColor, float Duration=0.f, float Thickness = 0.f);
    UFUNCTION(BlueprintCallable)
    static void DrawDebugPoint(const UObject* WorldContextObject, const FVector Position, float Size, FLinearColor PointColor, float Duration=0.f);
    UFUNCTION(BlueprintCallable)
    static void DrawDebugArrow(const UObject* WorldContextObject, const FVector LineStart, const FVector LineEnd, float ArrowSize, FLinearColor LineColor, float Duration=0.f, float Thickness = 0.f);
    UFUNCTION(BlueprintCallable)
    static void DrawDebugBox(const UObject* WorldContextObject, const FVector Center, FVector Extent, FLinearColor LineColor, const FRotator Rotation=FRotator::ZeroRotator, float Duration=0.f, float Thickness = 0.f);
    UFUNCTION(BlueprintCallable)
    static void DrawDebugSphere(const UObject* WorldContextObject, const FVector Center, float Radius=100.f, int32 Segments=12, FLinearColor LineColor = FLinearColor::White, float Duration=0.f, float Thickness = 0.f);
    UFUNCTION(BlueprintCallable)
    static void DrawDebugCylinder(const UObject* WorldContextObject, const FVector Start, const FVector End, float Radius=100.f, int32 Segments=12, FLinearColor LineColor = FLinearColor::White, float Duration=0.f, float Thickness = 0.f);
    UFUNCTION(BlueprintCallable)
    static void DrawDebugCapsule(const UObject* WorldContextObject, const FVector Center, float HalfHeight, float Radius, const FRotator Rotation, FLinearColor LineColor = FLinearColor::White, float Duration=0.f, float Thickness = 0.f);
    UFUNCTION(BlueprintCallable)
    static void DrawDebugString(const UObject* WorldContextObject, const FVector TextLocation, const FString& Text, class AActor* TestBaseActor = nullptr, FLinearColor TextColor = FLinearColor::White, float Duration=0.f);
    UFUNCTION(BlueprintCallable)
    static void DrawDebugPlane(const UObject* WorldContextObject, const FPlane& PlaneCoordinates, const FVector Location, float Size, FLinearColor PlaneColor = FLinearColor::White, float Duration=0.f);
};

}

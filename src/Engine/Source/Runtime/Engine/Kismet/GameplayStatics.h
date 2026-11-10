#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

#include "GameplayStatics.generated.h"

namespace ue5parse::runtime {

class AActor;
class ACharacter;
class AGameModeBase;
class AGameStateBase;
class APawn;
class APlayerController;
class UInterface;
class ULevelStreaming;
class UPhysicalMaterial;
class UPrimitiveComponent;
class UParticleSystem;
class UParticleSystemComponent;
class USceneComponent;

UCLASS()
class UGameplayStatics : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static UObject* SpawnObject(TSubclassOf<UObject> ObjectClass, UObject* Outer);
    UFUNCTION(BlueprintCallable)
    static class AActor*
    BeginDeferredActorSpawnFromClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform,
                                     ESpawnActorCollisionHandlingMethod CollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined,
                                     AActor* Owner = nullptr, ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot);
    UFUNCTION(BlueprintCallable)
    static class AActor* FinishSpawningActor(class AActor* Actor, const FTransform& SpawnTransform,
                                             ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot);
    UFUNCTION(BlueprintCallable)
    static class AActor* GetActorOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass);
    UFUNCTION(BlueprintCallable)
    static void GetAllActorsOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors);
    UFUNCTION(BlueprintCallable)
    static void GetAllActorsWithInterface(const UObject* WorldContextObject, TSubclassOf<UInterface> Interface, TArray<AActor*>& OutActors);
    UFUNCTION(BlueprintCallable)
    static void GetAllActorsWithTag(const UObject* WorldContextObject, FName Tag, TArray<AActor*>& OutActors);
    UFUNCTION(BlueprintCallable)
    static void GetAllActorsOfClassWithTag(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, FName Tag, TArray<AActor*>& OutActors);
    UFUNCTION(BlueprintPure)
    static AActor* FindNearestActor(FVector Origin, const TArray<AActor*>& ActorsToCheck, float& Distance);
    UFUNCTION(BlueprintPure)
    static class APlayerController* GetPlayerController(const UObject* WorldContextObject, int32 PlayerIndex);
    UFUNCTION(BlueprintPure)
    static class APawn* GetPlayerPawn(const UObject* WorldContextObject, int32 PlayerIndex);
    UFUNCTION(BlueprintPure)
    static class ACharacter* GetPlayerCharacter(const UObject* WorldContextObject, int32 PlayerIndex);
    UFUNCTION(BlueprintPure)
    static class ULevelStreaming* GetStreamingLevel(const UObject* WorldContextObject, FName PackageName);
    UFUNCTION(BlueprintCallable)
    static FString GetCurrentLevelName(const UObject* WorldContextObject, bool bRemovePrefixString = true);
    UFUNCTION(BlueprintPure)
    static class AGameModeBase* GetGameMode(const UObject* WorldContextObject);
    UFUNCTION(BlueprintPure)
    static class AGameStateBase* GetGameState(const UObject* WorldContextObject);
    UFUNCTION(BlueprintPure)
    static class UClass* GetObjectClass(const UObject* Object);
    UFUNCTION(BlueprintPure)
    static bool ObjectIsA(const UObject* Object, TSubclassOf<UObject> ObjectClass);
    UFUNCTION(BlueprintPure)
    static float GetGlobalTimeDilation(const UObject* WorldContextObject);
    UFUNCTION(BlueprintCallable)
    static void SetGlobalTimeDilation(const UObject* WorldContextObject, float TimeDilation);
    UFUNCTION(BlueprintPure)
    static bool IsGamePaused(const UObject* WorldContextObject);
    UFUNCTION(BlueprintPure)
    static void BreakHitResult(const struct FHitResult& Hit, bool& bBlockingHit, bool& bInitialOverlap, float& Time, float& Distance, FVector& Location,
                               FVector& ImpactPoint, FVector& Normal, FVector& ImpactNormal, class UPhysicalMaterial*& PhysMat, class AActor*& HitActor,
                               class UPrimitiveComponent*& HitComponent, FName& HitBoneName, FName& BoneName, int32& HitItem, int32& ElementIndex,
                               int32& FaceIndex, FVector& TraceStart, FVector& TraceEnd);
    UFUNCTION(BlueprintPure)
    static FHitResult MakeHitResult(bool bBlockingHit, bool bInitialOverlap, float Time, float Distance, FVector Location, FVector ImpactPoint, FVector Normal,
                                    FVector ImpactNormal, class UPhysicalMaterial* PhysMat, class AActor* HitActor, class UPrimitiveComponent* HitComponent,
                                    FName HitBoneName, FName BoneName, int32 HitItem, int32 ElementIndex, int32 FaceIndex, FVector TraceStart,
                                    FVector TraceEnd);
    UFUNCTION(BlueprintPure)
    static double GetWorldDeltaSeconds(const UObject* WorldContextObject);
    UFUNCTION(BlueprintPure)
    static double GetTimeSeconds(const UObject* WorldContextObject);
    UFUNCTION(BlueprintPure)
    static double GetRealTimeSeconds(const UObject* WorldContextObject);
    UFUNCTION(BlueprintPure)
    static FString GetPlatformName();
    UFUNCTION(BlueprintPure)
    static bool HasLaunchOption(const FString& OptionToCheck);

    UFUNCTION(BlueprintCallable)
    static UParticleSystemComponent* SpawnEmitterAtLocation(const UObject* WorldContextObject, UParticleSystem* EmitterTemplate, FVector Location,
                                                            FRotator Rotation = FRotator::ZeroRotator, FVector Scale = FVector(1.f), bool bAutoDestroy = true,
                                                            EPSCPoolMethod PoolingMethod = EPSCPoolMethod::None, bool bAutoActivateSystem = true,
                                                            bool bIsFirstPerson = false, float WarmupTime = 0);
    static UParticleSystemComponent* SpawnEmitterAtLocation(const UObject* WorldContextObject, UParticleSystem* EmitterTemplate, FVector Location,
                                                            FRotator Rotation, bool bAutoDestroy, EPSCPoolMethod PoolingMethod = EPSCPoolMethod::None,
                                                            bool bAutoActivateSystem = true);
    static UParticleSystemComponent* SpawnEmitterAtLocation(UWorld* World, UParticleSystem* EmitterTemplate, const FTransform& SpawnTransform,
                                                            bool bAutoDestroy = true, EPSCPoolMethod PoolingMethod = EPSCPoolMethod::None,
                                                            bool bAutoActivate = true);
    UFUNCTION(BlueprintCallable)
    static UParticleSystemComponent* SpawnEmitterAttached(UParticleSystem* EmitterTemplate, USceneComponent* AttachToComponent,
                                                          FName AttachPointName = NAME_None, FVector Location = FVector::ZeroVector,
                                                          FRotator Rotation = FRotator::ZeroRotator, FVector Scale = FVector(1.f),
                                                          EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bAutoDestroy = true,
                                                          EPSCPoolMethod PoolingMethod = EPSCPoolMethod::None, bool bAutoActivate = true,
                                                          bool bIsFirstPerson = false, float WarmupTime = 0, uint8 ParticleVariantColor = 0);
    static UParticleSystemComponent* SpawnEmitterAttached(UParticleSystem* EmitterTemplate, USceneComponent* AttachToComponent, FName AttachPointName,
                                                          FVector Location, FRotator Rotation, EAttachLocation::Type LocationType, bool bAutoDestroy = true,
                                                          EPSCPoolMethod PoolingMethod = EPSCPoolMethod::None, bool bAutoActivate = true);

private:
    static UParticleSystemComponent* InternalSpawnEmitterAtLocation(UWorld* World, UParticleSystem* EmitterTemplate, FVector SpawnLocation,
                                                                    FRotator SpawnRotation, FVector SpawnScale, bool bAutoDestroy, EPSCPoolMethod PoolingMethod,
                                                                    bool bAutoActivateSystem, bool bIsFirstPerson);
};

}

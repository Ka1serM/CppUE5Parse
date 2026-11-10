#pragma once

#include <functional>

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/TimerHandle.h"

#include "Actor.generated.h"

namespace ue5parse::runtime {

class AController;
class APawn;
class AActor;
class UDamageType;
class UPrimitiveComponent;
class ULevel;
class UWorld;
struct FMinimalViewInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActorDestroyedSignature, AActor*, DestroyedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActorEndPlaySignature, AActor*, Actor, EEndPlayReason::Type, EndPlayReason);

UCLASS()
class AActor : public UObject {
    GENERATED_BODY()
public:
    explicit AActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FActorTickFunction PrimaryActorTick;
    UPROPERTY(Replicated)
    bool bHidden = false;
    UPROPERTY()
    bool bLocallyHidden = false;
    UPROPERTY(Replicated)
    bool bTearOff = false;
    UPROPERTY()
    bool bExchangedRoles = false;
    UPROPERTY()
    bool bReplicates = false;
    UPROPERTY(ReplicatedUsing = OnRep_ReplicateMovement)
    bool bReplicateMovement = false;
    UPROPERTY()
    bool bAllowTickBeforeBeginPlay = false;
    UPROPERTY()
    bool bIsEditorOnlyActor = false;
    UPROPERTY()
    bool bActorEnableCollision = true;
    UPROPERTY()
    bool bActorIsBeingDestroyed = false;
    UPROPERTY()
    bool bAllowReceiveTickEventOnDedicatedServer = true;
    UPROPERTY()
    bool bFindCameraComponentWhenViewTarget = true;
    UPROPERTY()
    float InitialLifeSpan = 0;
    UPROPERTY()
    float CustomTimeDilation = 1.0f;
    UPROPERTY(Replicated)
    TEnumAsByte<ENetRole> RemoteRole = ROLE_None;
    UPROPERTY(ReplicatedUsing = OnRep_AttachmentReplication)
    FRepAttachment AttachmentReplication;
    UPROPERTY(ReplicatedUsing = OnRep_ReplicatedMovement)
    FRepMovement ReplicatedMovement;
    UPROPERTY(ReplicatedUsing = OnRep_Owner)
    TObjectPtr<AActor> Owner = nullptr;
    UPROPERTY(Replicated)
    TEnumAsByte<ENetRole> Role = ROLE_Authority;
    UPROPERTY()
    TObjectPtr<APawn> Instigator = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<AActor>> Children;
    UPROPERTY()
    TObjectPtr<USceneComponent> RootComponent = nullptr;
    UPROPERTY()
    FTimerHandle TimerHandle_LifeSpanExpired;
    UPROPERTY()
    TArray<FName> Tags;
    UPROPERTY()
    FMulticastScriptDelegate OnDestroyed;
    UPROPERTY()
    FMulticastScriptDelegate OnEndPlay;
    UPROPERTY()
    TArray<TObjectPtr<UActorComponent>> InstanceComponents;
    UPROPERTY()
    TArray<TObjectPtr<UActorComponent>> BlueprintCreatedComponents;

    void PostInitProperties() override;
    void PostLoad() override;
    UWorld* GetWorld() const override;
    int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;

    const TArray<UActorComponent*>& GetComponents() const { return OwnedComponents; }
    void AddOwnedComponent(UActorComponent* Component);
    void RemoveOwnedComponent(UActorComponent* Component);
    template <class T>
    T* FindComponentByClass() const {
        return static_cast<T*>(FindComponentByClass(T::StaticClass()));
    }
    UActorComponent* FindComponentByClass(const UClass* ComponentClass) const;
    USceneComponent* GetRootComponent() const { return RootComponent; }
    virtual FVector GetVelocity() const;
    bool SetRootComponent(USceneComponent* NewRootComponent);
    UActorComponent* CreateComponentFromTemplate(UActorComponent* Template, FName InName);
    static void ForEachComponentOfActorClassDefault(const UClass* ActorClass, const UClass* ComponentClass,
                                                    const std::function<bool(const UActorComponent*)>& Func);

    UFUNCTION(BlueprintCallable)
    UActorComponent* GetComponentByClass(TSubclassOf<UActorComponent> ComponentClass) const;
    UFUNCTION(BlueprintCallable)
    TArray<UActorComponent*> K2_GetComponentsByClass(TSubclassOf<UActorComponent> ComponentClass) const;
    UFUNCTION(BlueprintCallable)
    TArray<UActorComponent*> GetComponentsByTag(TSubclassOf<UActorComponent> ComponentClass, FName Tag) const;
    UFUNCTION(BlueprintCallable)
    USceneComponent* K2_GetRootComponent() const { return RootComponent; }
    UFUNCTION(BlueprintCallable)
    UActorComponent* AddComponent(FName TemplateName, bool bManualAttachment, const FTransform& RelativeTransform, UObject* ComponentTemplateContext,
                                  bool bDeferredFinish);
    UFUNCTION(BlueprintCallable)
    UActorComponent* AddComponentByClass(TSubclassOf<UActorComponent> Class, bool bManualAttachment, const FTransform& RelativeTransform, bool bDeferredFinish);
    UFUNCTION(BlueprintCallable)
    void FinishAddComponent(UActorComponent* Component, bool bManualAttachment, const FTransform& RelativeTransform);

    virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult);
    virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

    void PostSpawnInitialize(const FTransform& SpawnTransform, AActor* InOwner, APawn* InInstigator, bool bRemoteOwned, bool bNoFail, bool bDeferConstruction,
                             ESpawnActorScaleMethod TransformScaleMethod);
    void FinishSpawning(const FTransform& Transform, bool bIsDefaultTransform = false);
    bool ExecuteConstruction(const FTransform& Transform, bool bIsDefaultTransform);
    void PostActorConstruction();
    void RegisterAllComponents();
    void UnregisterAllComponents();
    void InitializeComponents();
    void UninitializeComponents();
    virtual void PostInitializeComponents();
    bool IsActorInitialized() const { return bActorInitialized; }
    void DispatchBeginPlay(bool bFromLevelStreaming = false);
    bool HasActorBegunPlay() const { return ActorHasBegunPlay == EActorBeginPlayState::HasBegunPlay; }
    bool IsActorBeginningPlay() const { return ActorHasBegunPlay == EActorBeginPlayState::BeginningPlay; }
    virtual void PostNetInit();
    virtual void OnSubobjectCreatedFromReplication(UObject* NewSubobject);
    virtual void OnSubobjectDestroyFromReplication(UObject* Subobject);
    void PreNetReceive() override {}
    void PostNetReceive() override {}
    UFUNCTION()
    virtual void OnRep_ReplicatedMovement();
    UFUNCTION()
    virtual void OnRep_AttachmentReplication();
    UFUNCTION()
    virtual void OnRep_ReplicateMovement() {}
    UFUNCTION()
    virtual void OnRep_Owner() {}
    UFUNCTION()
    virtual void OnRep_Instigator() {}
    virtual void PostNetReceiveLocationAndRotation();
    bool Destroy(bool bNetForce = false);
    virtual void Destroyed();
    virtual void EndPlay(EEndPlayReason::Type EndPlayReason);
    void RouteEndPlay(EEndPlayReason::Type EndPlayReason);
    virtual void LifeSpanExpired();
    virtual void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction);
    virtual void Tick(float DeltaSeconds);
    void RegisterAllActorTickFunctions(bool bRegister, bool bDoComponents);
    virtual void RegisterActorTickFunctions(bool bRegister);
    void AddTickPrerequisiteActor(AActor* PrerequisiteActor);
    void AddTickPrerequisiteComponent(UActorComponent* PrerequisiteComponent);

    UFUNCTION(BlueprintCallable)
    void SetLifeSpan(float InLifespan);
    UFUNCTION(BlueprintCallable)
    float GetLifeSpan() const;
    UFUNCTION(BlueprintCallable)
    FTimerHandle K2_GetLifeSpanTimer() const { return TimerHandle_LifeSpanExpired; }
    UFUNCTION(BlueprintCallable)
    void K2_DestroyActor();
    UFUNCTION(BlueprintCallable)
    bool IsActorBeingDestroyed() const { return bActorIsBeingDestroyed; }
    UFUNCTION(BlueprintCallable)
    void SetActorTickInterval(float TickInterval);
    UFUNCTION(BlueprintCallable)
    float GetGameTimeSinceCreation() const;
    UFUNCTION(BlueprintCallable)
    float GetActorTimeDilation() const;
    UFUNCTION(BlueprintCallable)
    bool IsSpawnedActor() const;

    UFUNCTION(BlueprintImplementableEvent)
    void UserConstructionScript();
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveBeginPlay();
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveEndPlay(TEnumAsByte<EEndPlayReason::Type> EndPlayReason);
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveTick(float DeltaSeconds);
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveDestroyed();
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveActorBeginOverlap(AActor* OtherActor);
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveAnyDamage(float Damage, UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
    UFUNCTION(BlueprintImplementableEvent)
    void ReceivePointDamage(float Damage, UDamageType* DamageType, FVector HitLocation, FVector HitNormal, UPrimitiveComponent* HitComponent, FName BoneName,
                            FVector ShotFromDirection, AController* InstigatedBy, AActor* DamageCauser, const FHitResult& HitInfo);

    const FTransform& GetActorTransform() const;
    FVector GetActorLocation() const;
    FRotator GetActorRotation() const;
    FQuat GetActorQuat() const;
    bool SetActorLocation(const FVector& NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                          ETeleportType Teleport = ETeleportType::None);
    bool SetActorRotation(FRotator NewRotation, ETeleportType Teleport = ETeleportType::None);
    bool SetActorRotation(const FQuat& NewRotation, ETeleportType Teleport = ETeleportType::None);
    bool SetActorLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                                     ETeleportType Teleport = ETeleportType::None);
    bool SetActorTransform(const FTransform& NewTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                           ETeleportType Teleport = ETeleportType::None);
    void AddActorWorldOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
    void AddActorWorldRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                               ETeleportType Teleport = ETeleportType::None);
    void AddActorLocalOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
    void AddActorLocalRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                               ETeleportType Teleport = ETeleportType::None);
    void SetActorRelativeScale3D(FVector NewRelativeScale) {
        if (RootComponent != nullptr)
            RootComponent->SetRelativeScale3D(NewRelativeScale);
    }
    void SetActorRelativeLocation(FVector NewRelativeLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                                  ETeleportType Teleport = ETeleportType::None);

    UFUNCTION(BlueprintCallable)
    FVector K2_GetActorLocation() const { return GetActorLocation(); }
    UFUNCTION(BlueprintCallable)
    FRotator K2_GetActorRotation() const { return GetActorRotation(); }
    UFUNCTION(BlueprintCallable)
    FTransform GetTransform() const { return GetActorTransform(); }
    UFUNCTION(BlueprintCallable)
    FVector GetActorForwardVector() const;
    UFUNCTION(BlueprintCallable)
    FVector GetActorRightVector() const;
    UFUNCTION(BlueprintCallable)
    FVector GetActorUpVector() const;
    UFUNCTION(BlueprintCallable)
    FVector GetActorScale3D() const;
    UFUNCTION(BlueprintCallable)
    void SetActorScale3D(FVector NewScale3D);
    UFUNCTION(BlueprintCallable)
    bool K2_SetActorLocation(FVector NewLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    bool K2_SetActorRotation(FRotator NewRotation, bool bTeleportPhysics);
    UFUNCTION(BlueprintCallable)
    bool K2_SetActorLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    bool K2_SetActorTransform(const FTransform& NewTransform, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_AddActorWorldOffset(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_AddActorWorldRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_AddActorLocalOffset(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_AddActorLocalRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_SetActorRelativeLocation(FVector NewRelativeLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    bool K2_TeleportTo(FVector DestLocation, FRotator DestRotation);
    UFUNCTION(BlueprintCallable)
    float GetDistanceTo(AActor* OtherActor) const;
    UFUNCTION(BlueprintCallable)
    float GetSquaredDistanceTo(AActor* OtherActor) const;
    UFUNCTION(BlueprintCallable)
    float GetHorizontalDistanceTo(AActor* OtherActor) const;
    UFUNCTION(BlueprintCallable)
    float GetSquaredHorizontalDistanceTo(AActor* OtherActor) const;

    bool AttachToComponent(USceneComponent* Parent, const FAttachmentTransformRules& AttachmentRules, FName SocketName = FName());
    bool AttachToActor(AActor* ParentActor, const FAttachmentTransformRules& AttachmentRules, FName SocketName = FName());
    void DetachFromActor(const FDetachmentTransformRules& DetachmentRules);
    UFUNCTION(BlueprintCallable)
    bool K2_AttachToComponent(USceneComponent* Parent, FName SocketName, EAttachmentRule LocationRule, EAttachmentRule RotationRule, EAttachmentRule ScaleRule,
                              bool bWeldSimulatedBodies);
    UFUNCTION(BlueprintCallable)
    bool K2_AttachToActor(AActor* ParentActor, FName SocketName, EAttachmentRule LocationRule, EAttachmentRule RotationRule, EAttachmentRule ScaleRule,
                          bool bWeldSimulatedBodies);
    UFUNCTION(BlueprintCallable)
    void K2_DetachFromActor(EDetachmentRule LocationRule, EDetachmentRule RotationRule, EDetachmentRule ScaleRule);
    UFUNCTION(BlueprintCallable)
    AActor* GetAttachParentActor() const;
    UFUNCTION(BlueprintCallable)
    FName GetAttachParentSocketName() const;
    UFUNCTION(BlueprintCallable)
    void GetAttachedActors(TArray<AActor*>& OutActors, bool bResetArray, bool bRecursivelyIncludeAttachedActors) const;
    UFUNCTION(BlueprintCallable)
    AActor* GetParentActor() const { return nullptr; }

    bool IsHidden() const { return bHidden; }
    bool IsEditorOnly() const { return bIsEditorOnlyActor; }
    UFUNCTION(BlueprintCallable)
    virtual void SetActorHiddenInGame(bool bNewHidden);
    UFUNCTION(BlueprintCallable)
    bool GetHiddenInGame() const { return bHidden; }
    UFUNCTION(BlueprintCallable)
    void SetActorLocallyHidden(bool NewHidden, FName Tag);
    UFUNCTION(BlueprintCallable)
    bool GetLocallyHidden() const { return bLocallyHidden; }
    UFUNCTION(BlueprintCallable)
    void SetActorEnableCollision(bool bNewActorEnableCollision) { bActorEnableCollision = bNewActorEnableCollision; }
    UFUNCTION(BlueprintCallable)
    bool GetActorEnableCollision() const { return bActorEnableCollision; }
    UFUNCTION(BlueprintCallable)
    bool ActorHasTag(FName Tag) const { return Tag != FName() && Tags.Contains(Tag); }

    UFUNCTION(BlueprintCallable)
    AActor* GetOwner() const { return Owner; }
    virtual void SetOwner(AActor* NewOwner);
    UFUNCTION(BlueprintCallable)
    APawn* GetInstigator() const { return Instigator; }
    void SetInstigator(APawn* InInstigator) { Instigator = InInstigator; }
    UFUNCTION(BlueprintCallable)
    AController* GetInstigatorController() const;
    UFUNCTION(BlueprintCallable)
    bool HasAuthority() const { return Role == ROLE_Authority; }
    UFUNCTION(BlueprintCallable)
    TEnumAsByte<ENetRole> GetLocalRole() const { return Role; }
    TEnumAsByte<ENetRole> GetRemoteRole() const { return RemoteRole; }
    ENetMode GetNetMode() const;
    void ExchangeNetRoles(bool bRemoteOwned);
    UFUNCTION(BlueprintCallable)
    void FlushNetDormancy() {}
    UFUNCTION(BlueprintCallable)
    void SetReplicates(bool bInReplicates) { bReplicates = bInReplicates; }

    ULevel* GetLevel() const;

protected:
    enum class EActorBeginPlayState : uint8 { HasNotBegunPlay, BeginningPlay, HasBegunPlay };
    virtual void BeginPlay();
    void ApplySpawnTransform(const FTransform& SpawnTransform, ESpawnActorScaleMethod TransformScaleMethod);

    TArray<UActorComponent*> OwnedComponents;
    EActorBeginPlayState ActorHasBegunPlay = EActorBeginPlayState::HasNotBegunPlay;
    bool bActorInitialized = false;
    bool bHasFinishedSpawning = false;
    bool bHasDeferredSpawnTransform = false;
    FTransform DeferredSpawnTransform;
    bool bActorWantsDestroyDuringBeginPlay = false;
    bool bRunningUserConstructionScript = false;
    double CreationTime = 0;
};

}

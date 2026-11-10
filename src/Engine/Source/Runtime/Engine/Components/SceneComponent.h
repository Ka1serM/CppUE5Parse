#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "SceneComponent.generated.h"

namespace ue5parse::runtime {

enum class ERelativeTransformSpace : uint8 { RTS_World, RTS_Actor, RTS_Component, RTS_ParentBoneSpace };

UENUM()
enum EDetailMode : int32 { DM_Low, DM_Medium, DM_High, DM_Epic, DM_MAX };

UCLASS()
class USceneComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit USceneComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(ReplicatedUsing = OnRep_AttachParent)
    TObjectPtr<USceneComponent> AttachParent = nullptr;
    UPROPERTY(ReplicatedUsing = OnRep_AttachSocketName)
    FName AttachSocketName;
    UPROPERTY(ReplicatedUsing = OnRep_AttachChildren)
    TArray<TObjectPtr<USceneComponent>> AttachChildren;
    UPROPERTY(ReplicatedUsing = OnRep_Transform)
    FVector RelativeLocation = FVector::ZeroVector;
    UPROPERTY(ReplicatedUsing = OnRep_Transform)
    FRotator RelativeRotation = FRotator::ZeroRotator;
    UPROPERTY(ReplicatedUsing = OnRep_Transform)
    FVector RelativeScale3D = FVector(1, 1, 1);
    UPROPERTY()
    FVector ComponentVelocity = FVector::ZeroVector;
    UPROPERTY()
    bool bComponentToWorldUpdated = false;
    UPROPERTY(ReplicatedUsing = OnRep_Transform)
    bool bAbsoluteLocation = false;
    UPROPERTY(ReplicatedUsing = OnRep_Transform)
    bool bAbsoluteRotation = false;
    UPROPERTY(ReplicatedUsing = OnRep_Transform)
    bool bAbsoluteScale = false;
    UPROPERTY(ReplicatedUsing = OnRep_Visibility)
    bool bVisible = true;
    UPROPERTY(Replicated)
    bool bShouldBeAttached = false;
    UPROPERTY(Replicated)
    bool bShouldSnapLocationWhenAttached = false;
    UPROPERTY(Replicated)
    bool bShouldSnapRotationWhenAttached = false;
    UPROPERTY(Replicated)
    bool bShouldSnapScaleWhenAttached = false;
    bool bNetUpdateTransform = false;
    bool bNetUpdateAttachment = false;
    TObjectPtr<USceneComponent> NetOldAttachParent = nullptr;
    FName NetOldAttachSocketName;
    UPROPERTY()
    bool bHiddenInGame = false;
    UPROPERTY()
    bool bRenderInTargetViewMode = false;
    UPROPERTY(Replicated)
    TEnumAsByte<EComponentMobility::Type> Mobility = EComponentMobility::Movable;
    UPROPERTY()
    TEnumAsByte<EDetailMode> DetailMode = DM_Low;

    void OnRegister() override;
    void OnUnregister() override;
    void DestroyComponent(bool bPromoteChildren = false) override;

    const FTransform& GetComponentTransform() const { return ComponentToWorld; }
    const FTransform& GetComponentToWorld() const { return ComponentToWorld; }
    FVector GetComponentLocation() const { return ComponentToWorld.GetLocation(); }
    FRotator GetComponentRotation() const { return ComponentToWorld.Rotator(); }
    FQuat GetComponentQuat() const { return ComponentToWorld.GetRotation(); }
    FVector GetComponentScale() const { return ComponentToWorld.GetScale3D(); }
    const FVector& GetRelativeLocation() const { return RelativeLocation; }
    const FRotator& GetRelativeRotation() const { return RelativeRotation; }
    const FVector& GetRelativeScale3D() const { return RelativeScale3D; }
    void SetUsingAbsoluteLocation(bool bInAbsoluteLocation) { bAbsoluteLocation = bInAbsoluteLocation; }
    void SetUsingAbsoluteRotation(bool bInAbsoluteRotation) { bAbsoluteRotation = bInAbsoluteRotation; }
    void SetUsingAbsoluteScale(bool bInAbsoluteScale) { bAbsoluteScale = bInAbsoluteScale; }
    void SetRelativeLocation_Direct(const FVector NewRelativeLocation) { RelativeLocation = NewRelativeLocation; }
    void SetRelativeRotation_Direct(const FRotator NewRelativeRotation) { RelativeRotation = NewRelativeRotation; }
    void SetRelativeScale3D_Direct(const FVector NewRelativeScale3D) { RelativeScale3D = NewRelativeScale3D; }
    FTransform CalcNewComponentToWorld(const FTransform& NewRelativeTransform) const;
    void UpdateComponentToWorld(ETeleportType Teleport = ETeleportType::None);
    virtual void OnUpdateTransform(ETeleportType Teleport) {}
    void UpdateChildTransforms(ETeleportType Teleport = ETeleportType::None);
    bool MoveComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep = false, FHitResult* OutHit = nullptr,
                       ETeleportType Teleport = ETeleportType::None);
    void SetRelativeLocation(FVector NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
    void SetRelativeRotation(FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
    void SetRelativeRotation(const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                             ETeleportType Teleport = ETeleportType::None);
    void SetRelativeLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                                        ETeleportType Teleport = ETeleportType::None);
    void SetRelativeTransform(const FTransform& NewTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                              ETeleportType Teleport = ETeleportType::None);
    void AddRelativeLocation(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
    void AddRelativeRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                             ETeleportType Teleport = ETeleportType::None);
    void AddLocalOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
    void AddLocalRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                          ETeleportType Teleport = ETeleportType::None);
    void AddWorldOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
    void AddWorldRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                          ETeleportType Teleport = ETeleportType::None);
    void SetWorldLocation(FVector NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
    void SetWorldRotation(const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
    void SetWorldLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                                     ETeleportType Teleport = ETeleportType::None);
    void SetWorldTransform(const FTransform& NewTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                           ETeleportType Teleport = ETeleportType::None);

    UFUNCTION(BlueprintCallable)
    FVector K2_GetComponentLocation() const { return GetComponentLocation(); }
    UFUNCTION(BlueprintCallable)
    FRotator K2_GetComponentRotation() const { return GetComponentRotation(); }
    UFUNCTION(BlueprintCallable)
    FVector K2_GetComponentScale() const { return GetComponentScale(); }
    UFUNCTION(BlueprintCallable)
    FTransform K2_GetComponentToWorld() const { return ComponentToWorld; }
    UFUNCTION(BlueprintCallable)
    FTransform GetRelativeTransform() const { return FTransform(RelativeRotation, RelativeLocation, RelativeScale3D); }
    UFUNCTION(BlueprintCallable)
    FVector GetForwardVector() const { return ComponentToWorld.GetUnitAxis(0); }
    UFUNCTION(BlueprintCallable)
    FVector GetRightVector() const { return ComponentToWorld.GetUnitAxis(1); }
    UFUNCTION(BlueprintCallable)
    FVector GetUpVector() const { return ComponentToWorld.GetUnitAxis(2); }
    UFUNCTION(BlueprintCallable)
    void K2_SetRelativeLocation(FVector NewLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_SetRelativeRotation(FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_SetRelativeLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_SetRelativeTransform(const FTransform& NewTransform, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void SetRelativeScale3D(FVector NewScale3D);
    UFUNCTION(BlueprintCallable)
    void K2_AddRelativeLocation(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_AddRelativeRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_AddLocalOffset(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_AddLocalRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_AddWorldOffset(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_AddWorldRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_SetWorldLocation(FVector NewLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_SetWorldRotation(FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_SetWorldLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void K2_SetWorldTransform(const FTransform& NewTransform, bool bSweep, FHitResult& SweepHitResult, bool bTeleport);
    UFUNCTION(BlueprintCallable)
    void SetWorldScale3D(FVector NewScale);

    void SetupAttachment(USceneComponent* InParent, FName InSocketName = FName());
    bool AttachToComponent(USceneComponent* InParent, const FAttachmentTransformRules& AttachmentRules, FName InSocketName = FName());
    void DetachFromComponent(const FDetachmentTransformRules& DetachmentRules);
    static void ConvertAttachLocation(EAttachLocation::Type InAttachLocation, EAttachmentRule& InOutLocationRule, EAttachmentRule& InOutRotationRule,
                                      EAttachmentRule& InOutScaleRule);
    UFUNCTION(BlueprintCallable)
    virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = ERelativeTransformSpace::RTS_World) const;
    UFUNCTION(BlueprintCallable)
    virtual bool DoesSocketExist(FName InSocketName) const { return false; }
    virtual bool HasAnySockets() const { return false; }
    virtual void QuerySupportedSockets(TArray<FName>& OutSocketNames) const {}
    bool IsAttachedTo(const USceneComponent* TestComp) const;
    UFUNCTION(BlueprintCallable)
    void GetParentComponents(TArray<USceneComponent*>& Parents) const;
    virtual void OnAttachmentChanged() {}
    virtual void OnChildAttached(USceneComponent* ChildComponent) {}
    virtual void OnChildDetached(USceneComponent* ChildComponent) {}
    UFUNCTION()
    void OnRep_Transform() { bNetUpdateTransform = true; }
    UFUNCTION()
    void OnRep_AttachParent() { bNetUpdateAttachment = true; }
    UFUNCTION()
    void OnRep_AttachSocketName();
    UFUNCTION()
    void OnRep_AttachChildren();
    UFUNCTION()
    void OnRep_Visibility(bool OldValue);
    void PostRepNotifies() override;

    UFUNCTION(BlueprintCallable)
    bool K2_AttachToComponent(USceneComponent* Parent, FName SocketName, EAttachmentRule LocationRule, EAttachmentRule RotationRule, EAttachmentRule ScaleRule,
                              bool bWeldSimulatedBodies);
    UFUNCTION(BlueprintCallable)
    void K2_DetachFromComponent(EDetachmentRule LocationRule, EDetachmentRule RotationRule, EDetachmentRule ScaleRule, bool bCallModify);
    UFUNCTION(BlueprintCallable)
    USceneComponent* GetAttachParent() const { return AttachParent; }
    UFUNCTION(BlueprintCallable)
    FName GetAttachSocketName() const { return AttachSocketName; }
    UFUNCTION(BlueprintCallable)
    void GetChildrenComponents(bool bIncludeAllDescendants, TArray<USceneComponent*>& Children) const;
    UFUNCTION(BlueprintCallable)
    int32 GetNumChildrenComponents() const { return AttachChildren.Num(); }
    UFUNCTION(BlueprintCallable)
    USceneComponent* GetChildComponent(int32 ChildIndex) const;
    UFUNCTION(BlueprintCallable)
    FVector GetSocketLocation(FName InSocketName) const { return GetSocketTransform(InSocketName).GetTranslation(); }
    UFUNCTION(BlueprintCallable)
    FRotator GetSocketRotation(FName InSocketName) const { return GetSocketTransform(InSocketName).Rotator(); }
    UFUNCTION(BlueprintCallable)
    FQuat GetSocketQuaternion(FName InSocketName) const { return GetSocketTransform(InSocketName).GetRotation(); }
    UFUNCTION(BlueprintCallable)
    TArray<FName> GetAllSocketNames() const;

    UFUNCTION(BlueprintCallable)
    bool IsVisible() const { return !bHiddenInGame && bVisible; }
    bool GetVisibleFlag() const { return bVisible; }
    bool ShouldRender() const;
    UFUNCTION(BlueprintCallable)
    void SetVisibility(bool bNewVisibility, bool bPropagateToChildren);
    UFUNCTION(BlueprintCallable)
    void ToggleVisibility(bool bPropagateToChildren) { SetVisibility(!bVisible, bPropagateToChildren); }
    UFUNCTION(BlueprintCallable)
    void SetHiddenInGame(bool NewHidden, bool bPropagateToChildren);
    virtual void OnVisibilityChanged() {}
    virtual void OnHiddenInGameChanged() {}
    UFUNCTION(BlueprintCallable)
    void SetMobility(TEnumAsByte<EComponentMobility::Type> NewMobility) { Mobility = NewMobility; }
    UFUNCTION(BlueprintCallable)
    void SetShouldUpdatePhysicsVolume(bool bInShouldUpdatePhysicsVolume) {}
    UFUNCTION(BlueprintCallable)
    bool GetShouldUpdatePhysicsVolume() const { return false; }

protected:
    FTransform ComponentToWorld;

private:
    void PropagateTransformUpdate(ETeleportType Teleport);
};

}

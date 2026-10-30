#pragma once

#include "ue5parse/runtime/Engine/Components/ActorComponent.h"
#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"

#include "SceneComponent.generated.h"

namespace ue5parse::runtime {

enum class ERelativeTransformSpace : uint8 { RTS_World, RTS_Actor, RTS_Component, RTS_ParentBoneSpace };

// USceneComponent: a component with a transform, attached to a parent (UE's Components/SceneComponent.h). Its world
// transform (ComponentToWorld) follows its parent's, and moving it updates its children.
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
    // What the RepNotifies of a bunch changed, applied in PostRepNotifies.
    bool bNetUpdateTransform = false;
    bool bNetUpdateAttachment = false;
    TObjectPtr<USceneComponent> NetOldAttachParent = nullptr;
    FName NetOldAttachSocketName;
    UPROPERTY()
    bool bHiddenInGame = false;
    UPROPERTY(Replicated)
    TEnumAsByte<EComponentMobility::Type> Mobility = EComponentMobility::Movable;

    void OnRegister() override;
    void OnUnregister() override;
    void DestroyComponent(bool bPromoteChildren = false) override;

    // -- Transforms --
    const FTransform& GetComponentTransform() const { return ComponentToWorld; }
    const FTransform& GetComponentToWorld() const { return ComponentToWorld; }
    FVector GetComponentLocation() const { return ComponentToWorld.GetLocation(); }
    FRotator GetComponentRotation() const { return ComponentToWorld.Rotator(); }
    FQuat GetComponentQuat() const { return ComponentToWorld.GetRotation(); }
    FVector GetComponentScale() const { return ComponentToWorld.GetScale3D(); }
    const FVector& GetRelativeLocation() const { return RelativeLocation; }
    const FRotator& GetRelativeRotation() const { return RelativeRotation; }
    const FVector& GetRelativeScale3D() const { return RelativeScale3D; }
    bool IsUsingAbsoluteLocation() const { return bAbsoluteLocation; }
    bool IsUsingAbsoluteRotation() const { return bAbsoluteRotation; }
    bool IsUsingAbsoluteScale() const { return bAbsoluteScale; }
    void SetUsingAbsoluteLocation(bool bInAbsoluteLocation) { bAbsoluteLocation = bInAbsoluteLocation; }
    void SetUsingAbsoluteRotation(bool bInAbsoluteRotation) { bAbsoluteRotation = bInAbsoluteRotation; }
    void SetUsingAbsoluteScale(bool bInAbsoluteScale) { bAbsoluteScale = bInAbsoluteScale; }
    // The world transform from the relative one and the parent's socket (UE's CalcNewComponentToWorld).
    FTransform CalcNewComponentToWorld(const FTransform& NewRelativeTransform) const;
    // Recomputes ComponentToWorld and the children's (UE's UpdateComponentToWorld).
    void UpdateComponentToWorld(ETeleportType Teleport = ETeleportType::None);
    virtual void OnUpdateTransform(ETeleportType Teleport) {}
    void UpdateChildTransforms(ETeleportType Teleport = ETeleportType::None);
    // Moves the component by a delta in world space and sets its rotation (UE's MoveComponent; the runtime has no
    // collision, so it never sweeps).
    bool MoveComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep = false, FHitResult* OutHit = nullptr,
                       ETeleportType Teleport = ETeleportType::None);
    void SetRelativeLocation(FVector NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                             ETeleportType Teleport = ETeleportType::None);
    void SetRelativeRotation(FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                             ETeleportType Teleport = ETeleportType::None);
    void SetRelativeRotation(const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                             ETeleportType Teleport = ETeleportType::None);
    void SetRelativeLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep = false,
                                        FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
    void SetRelativeTransform(const FTransform& NewTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                              ETeleportType Teleport = ETeleportType::None);
    void AddRelativeLocation(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                             ETeleportType Teleport = ETeleportType::None);
    void AddRelativeRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                             ETeleportType Teleport = ETeleportType::None);
    void AddLocalOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                        ETeleportType Teleport = ETeleportType::None);
    void AddLocalRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                          ETeleportType Teleport = ETeleportType::None);
    void AddWorldOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                        ETeleportType Teleport = ETeleportType::None);
    void AddWorldRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                          ETeleportType Teleport = ETeleportType::None);
    void SetWorldLocation(FVector NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                          ETeleportType Teleport = ETeleportType::None);
    void SetWorldRotation(const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
                          ETeleportType Teleport = ETeleportType::None);
    void SetWorldLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep = false,
                                     FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
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
    void K2_SetRelativeLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult,
                                           bool bTeleport);
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

    // -- Attachment --
    // A constructor's attachment: the parent and socket that the component attaches to when it registers.
    void SetupAttachment(USceneComponent* InParent, FName InSocketName = FName());
    bool AttachToComponent(USceneComponent* InParent, const FAttachmentTransformRules& AttachmentRules, FName InSocketName = FName());
    void DetachFromComponent(const FDetachmentTransformRules& DetachmentRules);
    // The socket transform, or the component's for a name it has no socket of (UE's GetSocketTransform).
    UFUNCTION(BlueprintCallable)
    virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = ERelativeTransformSpace::RTS_World) const;
    UFUNCTION(BlueprintCallable)
    virtual bool DoesSocketExist(FName InSocketName) const { return false; }
    virtual bool HasAnySockets() const { return false; }
    virtual void QuerySupportedSockets(TArray<FName>& OutSocketNames) const {}
    bool IsAttachedTo(const USceneComponent* TestComp) const;
    // The parents up the attach chain, nearest first (UE's GetParentComponents).
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
    // The attachment and the transform that the RepNotifies of this bunch changed (UE's USceneComponent::PostRepNotifies).
    void PostRepNotifies() override;

    UFUNCTION(BlueprintCallable)
    bool K2_AttachToComponent(USceneComponent* Parent, FName SocketName, EAttachmentRule LocationRule, EAttachmentRule RotationRule,
                              EAttachmentRule ScaleRule, bool bWeldSimulatedBodies);
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

    // -- Visibility --
    UFUNCTION(BlueprintCallable)
    bool IsVisible() const { return !bHiddenInGame && bVisible; }
    bool GetVisibleFlag() const { return bVisible; }
    // Whether the component renders in the game world: visible, and its owner is not hidden (UE's ShouldRender; the
    // runtime has no child actor components).
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

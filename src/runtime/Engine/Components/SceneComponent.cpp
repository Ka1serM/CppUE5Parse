#include "ue5parse/runtime/Engine/Components/SceneComponent.h"

#include <utility>

#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

// UE's Components/SceneComponent.cpp (UE 5.3). The runtime has no physics or overlaps: moves never sweep.

namespace ue5parse::runtime {

const FAttachmentTransformRules FAttachmentTransformRules::KeepRelativeTransform(EAttachmentRule::KeepRelative, false);
const FAttachmentTransformRules FAttachmentTransformRules::KeepWorldTransform(EAttachmentRule::KeepWorld, false);
const FAttachmentTransformRules FAttachmentTransformRules::SnapToTargetNotIncludingScale(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
                                                                                       EAttachmentRule::KeepWorld, false);
const FAttachmentTransformRules FAttachmentTransformRules::SnapToTargetIncludingScale(EAttachmentRule::SnapToTarget, false);
const FDetachmentTransformRules FDetachmentTransformRules::KeepRelativeTransform(EDetachmentRule::KeepRelative, true);
const FDetachmentTransformRules FDetachmentTransformRules::KeepWorldTransform(EDetachmentRule::KeepWorld, true);

USceneComponent::USceneComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

void USceneComponent::OnRegister() {
    // AttachChildren is transient: a component attaches to its AttachParent when it registers.
    if (AttachParent != nullptr && !AttachToComponent(AttachParent, FAttachmentTransformRules::KeepRelativeTransform, AttachSocketName)) {
        AttachParent = nullptr;
        AttachSocketName = FName();
        bShouldBeAttached = false;
    }
    Super::OnRegister();
    UpdateComponentToWorld();
}

void USceneComponent::OnUnregister() { Super::OnUnregister(); }

void USceneComponent::DestroyComponent(bool bPromoteChildren) {
    if (IsBeingDestroyed()) return;
    // The children detach, keeping their world transforms (UE's OnComponentDestroyed).
    TArray<TObjectPtr<USceneComponent>> Children = AttachChildren;
    for (USceneComponent* Child : Children)
        if (Child != nullptr) Child->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    Super::DestroyComponent(bPromoteChildren);
}

FTransform USceneComponent::CalcNewComponentToWorld(const FTransform& NewRelativeTransform) const {
    if (AttachParent == nullptr) return NewRelativeTransform;
    FTransform NewCompToWorld = NewRelativeTransform * AttachParent->GetSocketTransform(AttachSocketName);
    if (bAbsoluteLocation) NewCompToWorld.SetTranslation(NewRelativeTransform.GetTranslation());
    if (bAbsoluteRotation) NewCompToWorld.SetRotation(NewRelativeTransform.GetRotation());
    if (bAbsoluteScale) NewCompToWorld.SetScale3D(NewRelativeTransform.GetScale3D());
    return NewCompToWorld;
}

void USceneComponent::UpdateComponentToWorld(ETeleportType Teleport) {
    // A parent not updated yet updates first, and may update this component on the way.
    if (AttachParent != nullptr && !AttachParent->bComponentToWorldUpdated) {
        AttachParent->UpdateComponentToWorld();
        if (bComponentToWorldUpdated) return;
    }
    bComponentToWorldUpdated = true;
    const FTransform NewTransform = CalcNewComponentToWorld(GetRelativeTransform());
    if (!ComponentToWorld.Equals(NewTransform, UE_SMALL_NUMBER) || Teleport != ETeleportType::None) {
        ComponentToWorld = NewTransform;
        PropagateTransformUpdate(Teleport);
    } else {
        UpdateChildTransforms(Teleport);
    }
}

void USceneComponent::PropagateTransformUpdate(ETeleportType Teleport) {
    OnUpdateTransform(Teleport);
    UpdateChildTransforms(Teleport);
}

void USceneComponent::UpdateChildTransforms(ETeleportType Teleport) {
    for (USceneComponent* Child : AttachChildren) {
        if (Child == nullptr) continue;
        // A child that was never updated updates fully; else only when it is not absolute in every part.
        if (!Child->bComponentToWorldUpdated || !(Child->bAbsoluteLocation && Child->bAbsoluteRotation && Child->bAbsoluteScale))
            Child->UpdateComponentToWorld(Teleport);
    }
}

bool USceneComponent::MoveComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit,
                                    ETeleportType Teleport) {
    if (OutHit != nullptr) *OutHit = FHitResult();
    // The new relative transform from the world delta (UE's InternalSetWorldLocationAndRotation).
    FVector NewLocation = GetComponentLocation() + Delta;
    FQuat NewWorldRotation = NewRotation;
    if (AttachParent != nullptr) {
        const FTransform ParentToWorld = AttachParent->GetSocketTransform(AttachSocketName);
        if (!bAbsoluteLocation) NewLocation = ParentToWorld.InverseTransformPosition(NewLocation);
        if (!bAbsoluteRotation) NewWorldRotation = ParentToWorld.GetRotation().Inverse() * NewWorldRotation;
    }
    const FRotator NewRelativeRotator = NewWorldRotation.Rotator();
    const bool bDiffLocation = !NewLocation.Equals(RelativeLocation);
    const bool bDiffRotation = !NewRelativeRotator.Equals(RelativeRotation);
    if (!bDiffLocation && !bDiffRotation) return true;
    RelativeLocation = NewLocation;
    if (bDiffRotation) RelativeRotation = NewRelativeRotator;
    UpdateComponentToWorld(Teleport);
    return true;
}

void USceneComponent::SetRelativeLocation(FVector NewLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    SetRelativeLocationAndRotation(NewLocation, RelativeRotation.Quaternion(), bSweep, OutSweepHitResult, Teleport);
}

void USceneComponent::SetRelativeRotation(FRotator NewRotation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    if (OutSweepHitResult != nullptr) *OutSweepHitResult = FHitResult();
    if (NewRotation.Equals(RelativeRotation, 0)) return;
    RelativeRotation = NewRotation;
    UpdateComponentToWorld(Teleport);
}

void USceneComponent::SetRelativeRotation(const FQuat& NewRotation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    SetRelativeLocationAndRotation(RelativeLocation, NewRotation, bSweep, OutSweepHitResult, Teleport);
}

void USceneComponent::SetRelativeLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep, FHitResult* OutSweepHitResult,
                                                     ETeleportType Teleport) {
    if (OutSweepHitResult != nullptr) *OutSweepHitResult = FHitResult();
    const FRotator NewRotator = NewRotation.Rotator();
    // The rotator is kept when it is the same rotation, so that a winding Blueprint rotator stays as set.
    const bool bSameRotation = RelativeRotation.Quaternion().Equals(NewRotation, UE_SMALL_NUMBER);
    if (NewLocation.Equals(RelativeLocation, 0) && bSameRotation) return;
    RelativeLocation = NewLocation;
    if (!bSameRotation) RelativeRotation = NewRotator;
    UpdateComponentToWorld(Teleport);
}

void USceneComponent::SetRelativeTransform(const FTransform& NewTransform, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    RelativeScale3D = NewTransform.GetScale3D();
    SetRelativeLocationAndRotation(NewTransform.GetTranslation(), NewTransform.GetRotation(), bSweep, OutSweepHitResult, Teleport);
    UpdateComponentToWorld(Teleport);
}

void USceneComponent::SetRelativeScale3D(FVector NewScale3D) {
    if (NewScale3D == RelativeScale3D) return;
    RelativeScale3D = NewScale3D;
    UpdateComponentToWorld();
}

void USceneComponent::AddRelativeLocation(FVector DeltaLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    SetRelativeLocation(RelativeLocation + DeltaLocation, bSweep, OutSweepHitResult, Teleport);
}

void USceneComponent::AddRelativeRotation(const FQuat& DeltaRotation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    SetRelativeRotation(DeltaRotation * RelativeRotation.Quaternion(), bSweep, OutSweepHitResult, Teleport);
}

void USceneComponent::AddLocalOffset(FVector DeltaLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    const FVector LocalOffset = RelativeRotation.Quaternion().RotateVector(DeltaLocation);
    SetRelativeLocation(RelativeLocation + LocalOffset, bSweep, OutSweepHitResult, Teleport);
}

void USceneComponent::AddLocalRotation(const FQuat& DeltaRotation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    SetRelativeRotation(RelativeRotation.Quaternion() * DeltaRotation, bSweep, OutSweepHitResult, Teleport);
}

void USceneComponent::AddWorldOffset(FVector DeltaLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    SetWorldLocation(GetComponentLocation() + DeltaLocation, bSweep, OutSweepHitResult, Teleport);
}

void USceneComponent::AddWorldRotation(const FQuat& DeltaRotation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    SetWorldRotation(DeltaRotation * GetComponentQuat(), bSweep, OutSweepHitResult, Teleport);
}

void USceneComponent::SetWorldLocation(FVector NewLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    FVector NewRelLocation = NewLocation;
    if (AttachParent != nullptr && !bAbsoluteLocation)
        NewRelLocation = AttachParent->GetSocketTransform(AttachSocketName).InverseTransformPosition(NewLocation);
    SetRelativeLocation(NewRelLocation, bSweep, OutSweepHitResult, Teleport);
}

void USceneComponent::SetWorldRotation(const FQuat& NewRotation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    FQuat NewRelRotation = NewRotation;
    if (AttachParent != nullptr && !bAbsoluteRotation)
        NewRelRotation = AttachParent->GetSocketTransform(AttachSocketName).GetRotation().Inverse() * NewRotation;
    SetRelativeRotation(NewRelRotation, bSweep, OutSweepHitResult, Teleport);
}

void USceneComponent::SetWorldLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep, FHitResult* OutSweepHitResult,
                                                  ETeleportType Teleport) {
    FVector NewRelLocation = NewLocation;
    FQuat NewRelRotation = NewRotation;
    if (AttachParent != nullptr) {
        const FTransform ParentToWorld = AttachParent->GetSocketTransform(AttachSocketName);
        if (!bAbsoluteLocation) NewRelLocation = ParentToWorld.InverseTransformPosition(NewLocation);
        if (!bAbsoluteRotation) NewRelRotation = ParentToWorld.GetRotation().Inverse() * NewRotation;
    }
    SetRelativeLocationAndRotation(NewRelLocation, NewRelRotation, bSweep, OutSweepHitResult, Teleport);
}

void USceneComponent::SetWorldTransform(const FTransform& NewTransform, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    if (AttachParent != nullptr) {
        const FTransform ParentToWorld = AttachParent->GetSocketTransform(AttachSocketName);
        FTransform RelativeTM = NewTransform.GetRelativeTransform(ParentToWorld);
        if (bAbsoluteLocation) RelativeTM.SetTranslation(NewTransform.GetTranslation());
        if (bAbsoluteRotation) RelativeTM.SetRotation(NewTransform.GetRotation());
        if (bAbsoluteScale) RelativeTM.SetScale3D(NewTransform.GetScale3D());
        SetRelativeTransform(RelativeTM, bSweep, OutSweepHitResult, Teleport);
    } else {
        SetRelativeTransform(NewTransform, bSweep, OutSweepHitResult, Teleport);
    }
}

void USceneComponent::SetWorldScale3D(FVector NewScale) {
    FVector NewRelScale = NewScale;
    if (AttachParent != nullptr && !bAbsoluteScale) {
        const FVector ParentScale = AttachParent->GetSocketTransform(AttachSocketName).GetScale3D();
        NewRelScale = NewScale * FTransform::GetSafeScaleReciprocal(ParentScale);
    }
    SetRelativeScale3D(NewRelScale);
}

void USceneComponent::K2_SetRelativeLocation(FVector NewLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    SetRelativeLocation(NewLocation, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_SetRelativeRotation(FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    SetRelativeRotation(NewRotation, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_SetRelativeLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult,
                                                        bool bTeleport) {
    SetRelativeLocationAndRotation(NewLocation, NewRotation.Quaternion(), bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_SetRelativeTransform(const FTransform& NewTransform, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    SetRelativeTransform(NewTransform, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_AddRelativeLocation(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    AddRelativeLocation(DeltaLocation, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_AddRelativeRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    AddRelativeRotation(DeltaRotation.Quaternion(), bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_AddLocalOffset(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    AddLocalOffset(DeltaLocation, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_AddLocalRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    AddLocalRotation(DeltaRotation.Quaternion(), bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_AddWorldOffset(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    AddWorldOffset(DeltaLocation, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_AddWorldRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    AddWorldRotation(DeltaRotation.Quaternion(), bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_SetWorldLocation(FVector NewLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    SetWorldLocation(NewLocation, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_SetWorldRotation(FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    SetWorldRotation(NewRotation.Quaternion(), bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_SetWorldLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult,
                                                     bool bTeleport) {
    SetWorldLocationAndRotation(NewLocation, NewRotation.Quaternion(), bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}
void USceneComponent::K2_SetWorldTransform(const FTransform& NewTransform, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    SetWorldTransform(NewTransform, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}

void USceneComponent::SetupAttachment(USceneComponent* InParent, FName InSocketName) {
    if (bRegistered || InParent == this || (InParent != nullptr && InParent->IsAttachedTo(this))) return;
    if (AttachParent != nullptr && AttachParent->AttachChildren.Contains(this)) return;
    AttachParent = InParent;
    AttachSocketName = InSocketName;
    bShouldBeAttached = AttachParent != nullptr;
}

bool USceneComponent::AttachToComponent(USceneComponent* Parent, const FAttachmentTransformRules& AttachmentRules, FName SocketName) {
    if (Parent == nullptr || Parent == this) return false;
    const int32 LastAttachIndex = Parent->AttachChildren.Find(this);
    if (Parent == AttachParent && SocketName == AttachSocketName && LastAttachIndex != INDEX_NONE) return true;
    AActor* MyActor = GetOwner();
    if (MyActor != nullptr && MyActor == Parent->GetOwner() && MyActor->GetRootComponent() == this) return false;
    if (Parent->IsAttachedTo(this)) return false;
    if (Parent->IsTemplate() != IsTemplate()) return false;
    const bool bSameAttachParentAndSocket = Parent == AttachParent && SocketName == AttachSocketName;
    const bool bKeepRelative = AttachmentRules.LocationRule == EAttachmentRule::KeepRelative &&
                               AttachmentRules.RotationRule == EAttachmentRule::KeepRelative &&
                               AttachmentRules.ScaleRule == EAttachmentRule::KeepRelative;
    // A component registering with its saved parent keeps its relative transform without detaching first.
    if (!(bSameAttachParentAndSocket && !IsRegistered() && bKeepRelative && LastAttachIndex == INDEX_NONE))
        DetachFromComponent(FDetachmentTransformRules(AttachmentRules, true));

    PrimaryComponentTick.AddPrerequisite(Parent, Parent->PrimaryComponentTick);
    AttachParent = Parent;
    AttachSocketName = SocketName;
    bShouldBeAttached = true;
    OnAttachmentChanged();
    Parent->AttachChildren.Add(this);

    const FTransform SocketTransform = Parent->GetSocketTransform(SocketName);
    const FTransform RelativeTM = ComponentToWorld.GetRelativeTransform(SocketTransform);
    switch (AttachmentRules.LocationRule) {
    case EAttachmentRule::KeepRelative: break;
    case EAttachmentRule::KeepWorld:
        RelativeLocation = bAbsoluteLocation ? ComponentToWorld.GetTranslation() : RelativeTM.GetTranslation();
        break;
    case EAttachmentRule::SnapToTarget: RelativeLocation = FVector::ZeroVector; break;
    }
    switch (AttachmentRules.RotationRule) {
    case EAttachmentRule::KeepRelative: break;
    case EAttachmentRule::KeepWorld:
        RelativeRotation = bAbsoluteRotation ? GetComponentRotation() : RelativeTM.GetRotation().Rotator();
        break;
    case EAttachmentRule::SnapToTarget: RelativeRotation = FRotator::ZeroRotator; break;
    }
    switch (AttachmentRules.ScaleRule) {
    case EAttachmentRule::KeepRelative: break;
    case EAttachmentRule::KeepWorld:
        RelativeScale3D = bAbsoluteScale ? ComponentToWorld.GetScale3D() : RelativeTM.GetScale3D();
        break;
    case EAttachmentRule::SnapToTarget: RelativeScale3D = FVector(1, 1, 1); break;
    }
    Parent->OnChildAttached(this);
    UpdateComponentToWorld(ETeleportType::TeleportPhysics);
    return true;
}

void USceneComponent::DetachFromComponent(const FDetachmentTransformRules& DetachmentRules) {
    if (AttachParent == nullptr) return;
    USceneComponent* OldParent = AttachParent;
    PrimaryComponentTick.RemovePrerequisite(OldParent, OldParent->PrimaryComponentTick);
    OldParent->AttachChildren.Remove(this);
    OldParent->OnChildDetached(this);
    AttachParent = nullptr;
    AttachSocketName = FName();
    bShouldBeAttached = false;
    OnAttachmentChanged();
    switch (DetachmentRules.LocationRule) {
    case EDetachmentRule::KeepRelative: break;
    case EDetachmentRule::KeepWorld: RelativeLocation = ComponentToWorld.GetTranslation(); break;
    }
    switch (DetachmentRules.RotationRule) {
    case EDetachmentRule::KeepRelative: break;
    case EDetachmentRule::KeepWorld: RelativeRotation = GetComponentRotation(); break;
    }
    switch (DetachmentRules.ScaleRule) {
    case EDetachmentRule::KeepRelative: break;
    case EDetachmentRule::KeepWorld: RelativeScale3D = ComponentToWorld.GetScale3D(); break;
    }
    UpdateComponentToWorld();
}

bool USceneComponent::K2_AttachToComponent(USceneComponent* Parent, FName SocketName, EAttachmentRule LocationRule, EAttachmentRule RotationRule,
                                           EAttachmentRule ScaleRule, bool bWeldSimulatedBodies) {
    return AttachToComponent(Parent, FAttachmentTransformRules(LocationRule, RotationRule, ScaleRule, bWeldSimulatedBodies), SocketName);
}

void USceneComponent::K2_DetachFromComponent(EDetachmentRule LocationRule, EDetachmentRule RotationRule, EDetachmentRule ScaleRule, bool bCallModify) {
    FDetachmentTransformRules Rules;
    Rules.LocationRule = LocationRule;
    Rules.RotationRule = RotationRule;
    Rules.ScaleRule = ScaleRule;
    Rules.bCallModify = bCallModify;
    DetachFromComponent(Rules);
}

FTransform USceneComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const {
    switch (TransformSpace) {
    case ERelativeTransformSpace::RTS_Actor:
        if (const AActor* Actor = GetOwner()) return ComponentToWorld.GetRelativeTransform(Actor->GetActorTransform());
        return ComponentToWorld;
    case ERelativeTransformSpace::RTS_Component:
    case ERelativeTransformSpace::RTS_ParentBoneSpace: return FTransform::Identity;
    case ERelativeTransformSpace::RTS_World: break;
    }
    return ComponentToWorld;
}

bool USceneComponent::IsAttachedTo(const USceneComponent* TestComp) const {
    for (const USceneComponent* Comp = AttachParent; Comp != nullptr; Comp = Comp->AttachParent)
        if (Comp == TestComp) return true;
    return false;
}

void USceneComponent::GetParentComponents(TArray<USceneComponent*>& Parents) const {
    Parents.Reset();
    for (USceneComponent* Parent = AttachParent; Parent != nullptr; Parent = Parent->AttachParent) Parents.Add(Parent);
}

void USceneComponent::GetChildrenComponents(bool bIncludeAllDescendants, TArray<USceneComponent*>& Children) const {
    Children.Reset();
    for (USceneComponent* Child : AttachChildren)
        if (Child != nullptr) Children.Add(Child);
    if (!bIncludeAllDescendants) return;
    for (int32 Index = 0; Index < Children.Num(); Index++)
        for (USceneComponent* Grandchild : Children[Index]->AttachChildren)
            if (Grandchild != nullptr) Children.Add(Grandchild);
}

USceneComponent* USceneComponent::GetChildComponent(int32 ChildIndex) const {
    return AttachChildren.IsValidIndex(ChildIndex) ? AttachChildren[ChildIndex] : nullptr;
}

TArray<FName> USceneComponent::GetAllSocketNames() const {
    TArray<FName> Names;
    QuerySupportedSockets(Names);
    return Names;
}

void USceneComponent::SetVisibility(bool bNewVisibility, bool bPropagateToChildren) {
    if (bNewVisibility != bVisible) {
        const bool OldValue = bVisible;
        bVisible = bNewVisibility;
        OnVisibilityChanged();
        (void)OldValue;
    }
    if (!bPropagateToChildren) return;
    TArray<USceneComponent*> Descendants;
    GetChildrenComponents(true, Descendants);
    for (USceneComponent* Child : Descendants) {
        if (Child->bVisible == bNewVisibility) continue;
        Child->bVisible = bNewVisibility;
        Child->OnVisibilityChanged();
    }
}

bool USceneComponent::ShouldRender() const {
    const AActor* Owner = GetOwner();
    const bool bShowInGame = IsVisible() && (Owner == nullptr || !Owner->IsHidden());
    return bShowInGame && GetVisibleFlag();
}

void USceneComponent::SetHiddenInGame(bool NewHidden, bool bPropagateToChildren) {
    if (NewHidden != bHiddenInGame) {
        bHiddenInGame = NewHidden;
        OnHiddenInGameChanged();
    }
    if (!bPropagateToChildren) return;
    TArray<USceneComponent*> Descendants;
    GetChildrenComponents(true, Descendants);
    for (USceneComponent* Child : Descendants) {
        if (Child->bHiddenInGame == NewHidden) continue;
        Child->bHiddenInGame = NewHidden;
        Child->OnHiddenInGameChanged();
    }
}

void USceneComponent::OnRep_AttachSocketName() {
    if (IsValid(AttachParent)) bNetUpdateAttachment = true;
}

void USceneComponent::OnRep_AttachChildren() {
    // AttachChildren and the children's AttachParent replicate apart, so the array can hold a child twice: the later
    // entry goes.
    for (int32 SearchIndex = AttachChildren.Num() - 1; SearchIndex >= 1; --SearchIndex) {
        USceneComponent* PossibleDuplicate = AttachChildren[SearchIndex];
        if (PossibleDuplicate == nullptr) continue;
        for (int32 DuplicateCheckIndex = SearchIndex - 1; DuplicateCheckIndex >= 0; --DuplicateCheckIndex) {
            if (PossibleDuplicate != AttachChildren[DuplicateCheckIndex]) continue;
            AttachChildren.RemoveAt(SearchIndex);
            break;
        }
    }
    // A child that arrived before its parent never had AttachParent set.
    for (USceneComponent* ChildComponent : AttachChildren) {
        if (ChildComponent == nullptr || ChildComponent->GetAttachParent() == this) continue;
        ChildComponent->AttachParent = this;
        ChildComponent->UpdateComponentToWorld();
    }
}

void USceneComponent::OnRep_Visibility(bool OldValue) {
    const bool ReppedValue = bVisible;
    bVisible = OldValue;
    SetVisibility(ReppedValue, false);
}

void USceneComponent::PostRepNotifies() {
    if (bNetUpdateAttachment) {
        std::swap(NetOldAttachParent, AttachParent);
        std::swap(NetOldAttachSocketName, AttachSocketName);
        if (bShouldSnapLocationWhenAttached && !bNetUpdateTransform) RelativeLocation = FVector::ZeroVector;
        if (bShouldSnapRotationWhenAttached && !bNetUpdateTransform) RelativeRotation = FRotator::ZeroRotator;
        if (bShouldSnapScaleWhenAttached && !bNetUpdateTransform) RelativeScale3D = FVector(1, 1, 1);
        if (AttachParent != nullptr && !bShouldBeAttached) {
            DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        } else {
            const bool bOldShouldBeAttached = bShouldBeAttached;
            const bool bOldShouldSnapLocationWhenAttached = bShouldSnapLocationWhenAttached;
            const bool bOldShouldSnapRotationWhenAttached = bShouldSnapRotationWhenAttached;
            const bool bOldShouldSnapScaleWhenAttached = bShouldSnapScaleWhenAttached;
            AttachToComponent(NetOldAttachParent, FAttachmentTransformRules::KeepRelativeTransform, NetOldAttachSocketName);
            // What the server sent stays.
            bShouldBeAttached = bOldShouldBeAttached;
            bShouldSnapLocationWhenAttached = bOldShouldSnapLocationWhenAttached;
            bShouldSnapRotationWhenAttached = bOldShouldSnapRotationWhenAttached;
            bShouldSnapScaleWhenAttached = bOldShouldSnapScaleWhenAttached;
        }
        bNetUpdateAttachment = false;
    }
    if (bNetUpdateTransform) {
        UpdateComponentToWorld();
        bNetUpdateTransform = false;
    }
}

}

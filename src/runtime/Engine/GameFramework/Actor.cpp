#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectHash.h"
#include "ue5parse/runtime/Engine/Components/SceneComponent.h"
#include "ue5parse/runtime/Engine/Engine/Level.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/Pawn.h"

// UE's GameFramework/Actor.cpp (UE 5.3): lifecycle, ticking, transform and attachment. Construction is in
// ActorConstruction.cpp, as in UE.

namespace ue5parse::runtime {

AActor::AActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

void AActor::PostInitProperties() {
    Super::PostInitProperties();
    RemoteRole = bReplicates ? ROLE_SimulatedProxy : ROLE_None;
    // The default subobjects made before this actor's properties were set registered with the archetype's list: the
    // components whose outer is this actor are its own.
    for (int32 Index = OwnedComponents.Num() - 1; Index >= 0; Index--)
        if (OwnedComponents[Index] == nullptr || OwnedComponents[Index]->GetOuter() != this) OwnedComponents.RemoveAt(Index);
}

void AActor::PostLoad() {
    Super::PostLoad();
    // A placed actor's subobjects loaded before it; the ones of this actor are its owned components.
    TArray<UObject*> Subobjects;
    GetObjectsWithOuter(this, Subobjects, false);
    for (UObject* Subobject : Subobjects)
        if (UActorComponent* Component = Cast<UActorComponent>(Subobject)) AddOwnedComponent(Component);
}

FVector AActor::GetVelocity() const {
    return RootComponent != nullptr ? RootComponent->ComponentVelocity : FVector::ZeroVector;
}

UWorld* AActor::GetWorld() const {
    if (HasAnyFlags(RF_ClassDefaultObject)) return nullptr;
    if (ULevel* Level = GetLevel()) return Level->OwningWorld;
    return nullptr;
}

ULevel* AActor::GetLevel() const { return GetTypedOuter<ULevel>(); }

int32 AActor::GetFunctionCallspace(UFunction* Function, FFrame* Stack) {
    // A client's copy runs everything but server RPCs; an RPC to the owning client or a multicast runs here as well.
    if ((Function->FunctionFlags & FUNC_Static) || GetWorld() == nullptr) return FunctionCallspace::Local;
    if (Function->FunctionFlags & FUNC_NetServer) return Role == ROLE_Authority ? FunctionCallspace::Local : FunctionCallspace::Remote;
    if ((Function->FunctionFlags & FUNC_BlueprintAuthorityOnly) && Role != ROLE_Authority) return FunctionCallspace::Absorbed;
    return FunctionCallspace::Local;
}

ENetMode AActor::GetNetMode() const {
    UWorld* World = GetWorld();
    return World ? World->GetNetMode() : NM_Standalone;
}

void AActor::ExchangeNetRoles(bool bRemoteOwned) {
    if (bExchangedRoles) return;
    if (bRemoteOwned) std::swap(Role, RemoteRole);
    bExchangedRoles = true;
}

void AActor::SetOwner(AActor* NewOwner) {
    if (Owner == NewOwner) return;
    if (Owner != nullptr) Owner->Children.Remove(this);
    Owner = NewOwner;
    if (Owner != nullptr) Owner->Children.Add(this);
}

AController* AActor::GetInstigatorController() const { return Instigator ? Instigator->GetController() : nullptr; }

// -- Components --

void AActor::AddOwnedComponent(UActorComponent* Component) {
    if (Component != nullptr) OwnedComponents.AddUnique(Component);
}

void AActor::RemoveOwnedComponent(UActorComponent* Component) { OwnedComponents.Remove(Component); }

UActorComponent* AActor::FindComponentByClass(const UClass* ComponentClass) const {
    if (ComponentClass == nullptr) return nullptr;
    for (UActorComponent* Component : OwnedComponents)
        if (Component != nullptr && Component->IsA(ComponentClass)) return Component;
    return nullptr;
}

bool AActor::SetRootComponent(USceneComponent* NewRootComponent) {
    if (NewRootComponent != nullptr && NewRootComponent->GetOwner() != this) return false;
    RootComponent = NewRootComponent;
    return true;
}

UActorComponent* AActor::GetComponentByClass(TSubclassOf<UActorComponent> ComponentClass) const {
    return FindComponentByClass(ComponentClass.Get());
}

TArray<UActorComponent*> AActor::K2_GetComponentsByClass(TSubclassOf<UActorComponent> ComponentClass) const {
    TArray<UActorComponent*> Result;
    for (UActorComponent* Component : OwnedComponents)
        if (Component != nullptr && (ComponentClass.Get() == nullptr || Component->IsA(ComponentClass.Get()))) Result.Add(Component);
    return Result;
}

TArray<UActorComponent*> AActor::GetComponentsByTag(TSubclassOf<UActorComponent> ComponentClass, FName Tag) const {
    TArray<UActorComponent*> Result;
    for (UActorComponent* Component : K2_GetComponentsByClass(ComponentClass))
        if (Component->ComponentHasTag(Tag)) Result.Add(Component);
    return Result;
}

void AActor::RegisterAllComponents() {
    UWorld* World = GetWorld();
    if (World == nullptr) return;
    // The root first, so that its children attach to a registered parent (UE's IncrementalRegisterComponents).
    if (RootComponent != nullptr && !RootComponent->IsRegistered()) RootComponent->RegisterComponentWithWorld(World);
    TArray<UActorComponent*> Components = OwnedComponents;
    for (UActorComponent* Component : Components)
        if (Component != nullptr && !Component->IsRegistered() && IsValid(Component)) Component->RegisterComponentWithWorld(World);
}

void AActor::UnregisterAllComponents() {
    TArray<UActorComponent*> Components = OwnedComponents;
    for (UActorComponent* Component : Components)
        if (Component != nullptr && Component->IsRegistered()) Component->UnregisterComponent();
}

void AActor::InitializeComponents() {
    TArray<UActorComponent*> Components = OwnedComponents;
    for (UActorComponent* Component : Components) {
        if (Component == nullptr || !Component->IsRegistered()) continue;
        if (Component->bAutoActivate && !Component->IsActive()) Component->Activate(true);
        if (Component->bWantsInitializeComponent && !Component->HasBeenInitialized()) Component->InitializeComponent();
    }
}

void AActor::UninitializeComponents() {
    TArray<UActorComponent*> Components = OwnedComponents;
    for (UActorComponent* Component : Components)
        if (Component != nullptr && Component->HasBeenInitialized()) Component->UninitializeComponent();
}

void AActor::PostInitializeComponents() { bActorInitialized = true; }

// -- Play --

void AActor::DispatchBeginPlay(bool bFromLevelStreaming) {
    UWorld* World = GetWorld();
    if (World == nullptr || ActorHasBegunPlay != EActorBeginPlayState::HasNotBegunPlay || IsGarbage()) return;
    ActorHasBegunPlay = EActorBeginPlayState::BeginningPlay;
    BeginPlay();
    if (ActorHasBegunPlay == EActorBeginPlayState::BeginningPlay) ActorHasBegunPlay = EActorBeginPlayState::HasBegunPlay;
    if (bActorWantsDestroyDuringBeginPlay) Destroy();
}

void AActor::BeginPlay() {
    SetLifeSpan(InitialLifeSpan);
    RegisterAllActorTickFunctions(true, false);
    TArray<UActorComponent*> Components = OwnedComponents;
    for (UActorComponent* Component : Components) {
        if (Component == nullptr || !Component->IsRegistered() || Component->HasBegunPlay()) continue;
        Component->RegisterAllComponentTickFunctions(true);
        Component->BeginPlay();
    }
    if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native)) ReceiveBeginPlay();
}

void AActor::OnSubobjectCreatedFromReplication(UObject* NewSubobject) {
    UActorComponent* Component = Cast<UActorComponent>(NewSubobject);
    if (Component == nullptr) return;
    Component->RegisterComponent();
    Component->SetIsReplicated(true);
}

void AActor::OnSubobjectDestroyFromReplication(UObject* Subobject) {
    if (UActorComponent* Component = Cast<UActorComponent>(Subobject)) Component->DestroyComponent();
}

void AActor::PostNetInit() {
    if (RemoteRole != ROLE_Authority) return;
    UWorld* World = GetWorld();
    if (World != nullptr && World->HasBegunPlay() && !HasActorBegunPlay()) DispatchBeginPlay();
}

void AActor::OnRep_ReplicatedMovement() {
    if (Role == ROLE_SimulatedProxy) PostNetReceiveLocationAndRotation();
}

void AActor::PostNetReceiveLocationAndRotation() {
    if (RootComponent == nullptr || !RootComponent->IsRegistered()) return;
    const FVector NewLocation = ReplicatedMovement.Location;
    if (!NewLocation.Equals(GetActorLocation()) || !ReplicatedMovement.Rotation.Equals(GetActorRotation()))
        SetActorLocationAndRotation(NewLocation, ReplicatedMovement.Rotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AActor::OnRep_AttachmentReplication() {
    const FRepAttachment& Attachment = AttachmentReplication;
    if (Attachment.AttachParent == nullptr) {
        if (RootComponent != nullptr && RootComponent->GetAttachParent() != nullptr) DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        return;
    }
    if (RootComponent == nullptr) return;
    USceneComponent* AttachParentComponent =
        Attachment.AttachComponent ? Attachment.AttachComponent.Get() : Attachment.AttachParent->GetRootComponent();
    if (AttachParentComponent == nullptr) return;
    RootComponent->SetRelativeLocation(Attachment.LocationOffset);
    RootComponent->SetRelativeRotation(Attachment.RotationOffset);
    RootComponent->SetRelativeScale3D(Attachment.RelativeScale3D);
    RootComponent->AttachToComponent(AttachParentComponent, FAttachmentTransformRules::KeepRelativeTransform, Attachment.AttachSocket);
}

bool AActor::Destroy(bool bNetForce) {
    if (IsActorBeingDestroyed()) return true;
    if (IsActorBeginningPlay()) {
        bActorWantsDestroyDuringBeginPlay = true;
        return true;
    }
    UWorld* World = GetWorld();
    if (World == nullptr) {
        MarkAsGarbage();
        return true;
    }
    return World->DestroyActor(this, bNetForce);
}

void AActor::K2_DestroyActor() { Destroy(); }

void AActor::Destroyed() {
    RouteEndPlay(EEndPlayReason::Destroyed);
    ReceiveDestroyed();
    AActor* Parms = this;
    OnDestroyed.ProcessMulticastDelegate(&Parms);
}

void AActor::RouteEndPlay(EEndPlayReason::Type EndPlayReason) {
    if (bActorInitialized) {
        if (ActorHasBegunPlay == EActorBeginPlayState::HasBegunPlay) EndPlay(EndPlayReason);
        UninitializeComponents();
    }
}

void AActor::EndPlay(EEndPlayReason::Type EndPlayReason) {
    if (ActorHasBegunPlay != EActorBeginPlayState::HasBegunPlay) return;
    ActorHasBegunPlay = EActorBeginPlayState::HasNotBegunPlay;
    ReceiveEndPlay(EndPlayReason);
    struct {
        AActor* Actor;
        TEnumAsByte<EEndPlayReason::Type> EndPlayReason;
    } Parms{this, EndPlayReason};
    OnEndPlay.ProcessMulticastDelegate(&Parms);
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearAllTimersForObject(this);
        World->GetLatentActionManager().RemoveActionsForObject(this);
    }
    TArray<UActorComponent*> Components = OwnedComponents;
    for (UActorComponent* Component : Components)
        if (Component != nullptr && Component->HasBegunPlay()) Component->EndPlay(EndPlayReason);
}

void AActor::SetLifeSpan(float InLifespan) {
    UWorld* World = GetWorld();
    if (World == nullptr) return;
    InitialLifeSpan = InLifespan;
    if (InLifespan > 0.0f) {
        World->GetTimerManager().SetTimer(TimerHandle_LifeSpanExpired, [this] { LifeSpanExpired(); }, InLifespan, false);
    } else {
        World->GetTimerManager().ClearTimer(TimerHandle_LifeSpanExpired);
    }
}

float AActor::GetLifeSpan() const {
    UWorld* World = GetWorld();
    const float Remaining = World ? World->GetTimerManager().GetTimerRemaining(TimerHandle_LifeSpanExpired) : 0.0f;
    return Remaining >= 0.0f ? Remaining : 0.0f;
}

void AActor::LifeSpanExpired() { Destroy(); }

float AActor::GetGameTimeSinceCreation() const {
    UWorld* World = GetWorld();
    return World ? static_cast<float>(World->GetTimeSeconds() - CreationTime) : 0.0f;
}

float AActor::GetActorTimeDilation() const { return CustomTimeDilation; }

bool AActor::IsSpawnedActor() const { return !HasAnyFlags(RF_WasLoaded); }

// -- Ticking --

void AActor::RegisterAllActorTickFunctions(bool bRegister, bool bDoComponents) {
    RegisterActorTickFunctions(bRegister);
    if (!bDoComponents) return;
    for (UActorComponent* Component : OwnedComponents)
        if (Component != nullptr) Component->RegisterAllComponentTickFunctions(bRegister);
}

void AActor::RegisterActorTickFunctions(bool bRegister) {
    if (!bRegister) {
        if (PrimaryActorTick.IsTickFunctionRegistered()) PrimaryActorTick.UnRegisterTickFunction();
        return;
    }
    if (!PrimaryActorTick.bCanEverTick || IsTemplate()) return;
    PrimaryActorTick.Target = this;
    PrimaryActorTick.SetTickFunctionEnable(PrimaryActorTick.bStartWithTickEnabled || PrimaryActorTick.IsTickFunctionEnabled());
    PrimaryActorTick.RegisterTickFunction(GetLevel());
}

void AActor::SetActorTickEnabled(bool bEnabled) {
    if (!IsTemplate() && PrimaryActorTick.bCanEverTick) PrimaryActorTick.SetTickFunctionEnable(bEnabled);
}

void AActor::SetActorTickInterval(float TickInterval) { PrimaryActorTick.UpdateTickIntervalAndCoolDown(TickInterval); }

void AActor::AddTickPrerequisiteActor(AActor* PrerequisiteActor) {
    if (PrimaryActorTick.bCanEverTick && PrerequisiteActor && PrerequisiteActor->PrimaryActorTick.bCanEverTick)
        PrimaryActorTick.AddPrerequisite(PrerequisiteActor, PrerequisiteActor->PrimaryActorTick);
}

void AActor::AddTickPrerequisiteComponent(UActorComponent* PrerequisiteComponent) {
    if (PrimaryActorTick.bCanEverTick && PrerequisiteComponent && PrerequisiteComponent->PrimaryComponentTick.bCanEverTick)
        PrimaryActorTick.AddPrerequisite(PrerequisiteComponent, PrerequisiteComponent->PrimaryComponentTick);
}

void AActor::TickActor(float DeltaSeconds, ELevelTick TickType, FActorTickFunction& ThisTickFunction) {
    if (IsGarbage()) return;
    if (TickType != LEVELTICK_ViewportsOnly) Tick(DeltaSeconds);
}

void AActor::Tick(float DeltaSeconds) {
    if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native)) ReceiveTick(DeltaSeconds);
    if (UWorld* World = GetWorld()) World->GetLatentActionManager().ProcessLatentActions(this, World->GetDeltaSeconds());
}

void FActorTickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType) {
    if (Target == nullptr || !IsValid(Target) || Target->IsActorBeingDestroyed()) return;
    if (TickType != LEVELTICK_All && !bTickEvenWhenPaused) return;
    if (!Target->HasActorBegunPlay() && !Target->bAllowTickBeforeBeginPlay) return;
    Target->TickActor(DeltaTime * Target->CustomTimeDilation, TickType, *this);
}

FString FActorTickFunction::DiagnosticMessage() {
    return Target ? Target->GetFullName() + FString("[TickActor]") : FString("<NULL>[TickActor]");
}

// -- Transform --

const FTransform& AActor::GetActorTransform() const {
    return RootComponent ? RootComponent->GetComponentTransform() : FTransform::Identity;
}

FVector AActor::GetActorLocation() const { return RootComponent ? RootComponent->GetComponentLocation() : FVector::ZeroVector; }

FRotator AActor::GetActorRotation() const { return RootComponent ? RootComponent->GetComponentRotation() : FRotator::ZeroRotator; }

FQuat AActor::GetActorQuat() const { return RootComponent ? RootComponent->GetComponentQuat() : FQuat::Identity; }

FVector AActor::GetActorForwardVector() const { return RootComponent ? RootComponent->GetForwardVector() : FVector(1, 0, 0); }

FVector AActor::GetActorRightVector() const { return RootComponent ? RootComponent->GetRightVector() : FVector(0, 1, 0); }

FVector AActor::GetActorUpVector() const { return RootComponent ? RootComponent->GetUpVector() : FVector(0, 0, 1); }

FVector AActor::GetActorScale3D() const { return RootComponent ? RootComponent->GetComponentScale() : FVector(1, 1, 1); }

void AActor::SetActorScale3D(FVector NewScale3D) {
    if (RootComponent != nullptr) RootComponent->SetWorldScale3D(NewScale3D);
}

bool AActor::SetActorLocation(const FVector& NewLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    if (RootComponent == nullptr) return false;
    const FVector Delta = NewLocation - GetActorLocation();
    return RootComponent->MoveComponent(Delta, GetActorQuat(), bSweep, OutSweepHitResult, Teleport);
}

bool AActor::SetActorRotation(FRotator NewRotation, ETeleportType Teleport) {
    return RootComponent && RootComponent->MoveComponent(FVector::ZeroVector, NewRotation.Quaternion(), false, nullptr, Teleport);
}

bool AActor::SetActorRotation(const FQuat& NewRotation, ETeleportType Teleport) {
    return RootComponent && RootComponent->MoveComponent(FVector::ZeroVector, NewRotation, false, nullptr, Teleport);
}

bool AActor::SetActorLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep, FHitResult* OutSweepHitResult,
                                         ETeleportType Teleport) {
    if (RootComponent == nullptr) return false;
    const FVector Delta = NewLocation - GetActorLocation();
    return RootComponent->MoveComponent(Delta, NewRotation.Quaternion(), bSweep, OutSweepHitResult, Teleport);
}

bool AActor::SetActorTransform(const FTransform& NewTransform, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    if (RootComponent == nullptr) return false;
    RootComponent->SetWorldTransform(NewTransform, bSweep, OutSweepHitResult, Teleport);
    return true;
}

void AActor::AddActorWorldOffset(FVector DeltaLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    if (RootComponent != nullptr) RootComponent->AddWorldOffset(DeltaLocation, bSweep, OutSweepHitResult, Teleport);
}

void AActor::AddActorWorldRotation(const FQuat& DeltaRotation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    if (RootComponent != nullptr) RootComponent->AddWorldRotation(DeltaRotation, bSweep, OutSweepHitResult, Teleport);
}

void AActor::AddActorLocalOffset(FVector DeltaLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    if (RootComponent != nullptr) RootComponent->AddLocalOffset(DeltaLocation, bSweep, OutSweepHitResult, Teleport);
}

void AActor::AddActorLocalRotation(const FQuat& DeltaRotation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    if (RootComponent != nullptr) RootComponent->AddLocalRotation(DeltaRotation, bSweep, OutSweepHitResult, Teleport);
}

void AActor::SetActorRelativeLocation(FVector NewRelativeLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport) {
    if (RootComponent != nullptr) RootComponent->SetRelativeLocation(NewRelativeLocation, bSweep, OutSweepHitResult, Teleport);
}

bool AActor::K2_SetActorLocation(FVector NewLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    return SetActorLocation(NewLocation, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}

bool AActor::K2_SetActorRotation(FRotator NewRotation, bool bTeleportPhysics) {
    return SetActorRotation(NewRotation, TeleportFlagToEnum(bTeleportPhysics));
}

bool AActor::K2_SetActorLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    return SetActorLocationAndRotation(NewLocation, NewRotation, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}

bool AActor::K2_SetActorTransform(const FTransform& NewTransform, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    return SetActorTransform(NewTransform, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}

void AActor::K2_AddActorWorldOffset(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    AddActorWorldOffset(DeltaLocation, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}

void AActor::K2_AddActorWorldRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    AddActorWorldRotation(DeltaRotation.Quaternion(), bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}

void AActor::K2_AddActorLocalOffset(FVector DeltaLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    AddActorLocalOffset(DeltaLocation, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}

void AActor::K2_AddActorLocalRotation(FRotator DeltaRotation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    AddActorLocalRotation(DeltaRotation.Quaternion(), bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}

void AActor::K2_SetActorRelativeLocation(FVector NewRelativeLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport) {
    SetActorRelativeLocation(NewRelativeLocation, bSweep, &SweepHitResult, TeleportFlagToEnum(bTeleport));
}

bool AActor::K2_TeleportTo(FVector DestLocation, FRotator DestRotation) {
    return SetActorLocationAndRotation(DestLocation, DestRotation, false, nullptr, ETeleportType::TeleportPhysics);
}

float AActor::GetDistanceTo(AActor* OtherActor) const {
    return OtherActor ? static_cast<float>(FVector::Dist(GetActorLocation(), OtherActor->GetActorLocation())) : 0.0f;
}

float AActor::GetSquaredDistanceTo(AActor* OtherActor) const {
    return OtherActor ? static_cast<float>(FVector::DistSquared(GetActorLocation(), OtherActor->GetActorLocation())) : 0.0f;
}

float AActor::GetHorizontalDistanceTo(AActor* OtherActor) const {
    return OtherActor ? static_cast<float>(FVector::Dist2D(GetActorLocation(), OtherActor->GetActorLocation())) : 0.0f;
}

float AActor::GetSquaredHorizontalDistanceTo(AActor* OtherActor) const {
    return OtherActor ? static_cast<float>(FVector::DistSquared2D(GetActorLocation(), OtherActor->GetActorLocation())) : 0.0f;
}

// -- Attachment --

bool AActor::AttachToComponent(USceneComponent* Parent, const FAttachmentTransformRules& AttachmentRules, FName SocketName) {
    return RootComponent && Parent && RootComponent->AttachToComponent(Parent, AttachmentRules, SocketName);
}

bool AActor::AttachToActor(AActor* ParentActor, const FAttachmentTransformRules& AttachmentRules, FName SocketName) {
    if (RootComponent == nullptr || ParentActor == nullptr) return false;
    USceneComponent* ParentDefaultAttachComponent = ParentActor->GetRootComponent();
    return ParentDefaultAttachComponent && RootComponent->AttachToComponent(ParentDefaultAttachComponent, AttachmentRules, SocketName);
}

void AActor::DetachFromActor(const FDetachmentTransformRules& DetachmentRules) {
    if (RootComponent != nullptr) RootComponent->DetachFromComponent(DetachmentRules);
}

bool AActor::K2_AttachToComponent(USceneComponent* Parent, FName SocketName, EAttachmentRule LocationRule, EAttachmentRule RotationRule,
                                  EAttachmentRule ScaleRule, bool bWeldSimulatedBodies) {
    return AttachToComponent(Parent, FAttachmentTransformRules(LocationRule, RotationRule, ScaleRule, bWeldSimulatedBodies), SocketName);
}

bool AActor::K2_AttachToActor(AActor* ParentActor, FName SocketName, EAttachmentRule LocationRule, EAttachmentRule RotationRule,
                              EAttachmentRule ScaleRule, bool bWeldSimulatedBodies) {
    return AttachToActor(ParentActor, FAttachmentTransformRules(LocationRule, RotationRule, ScaleRule, bWeldSimulatedBodies), SocketName);
}

void AActor::K2_DetachFromActor(EDetachmentRule LocationRule, EDetachmentRule RotationRule, EDetachmentRule ScaleRule) {
    FDetachmentTransformRules Rules;
    Rules.LocationRule = LocationRule;
    Rules.RotationRule = RotationRule;
    Rules.ScaleRule = ScaleRule;
    DetachFromActor(Rules);
}

AActor* AActor::GetAttachParentActor() const {
    if (RootComponent == nullptr || RootComponent->GetAttachParent() == nullptr) return nullptr;
    return RootComponent->GetAttachParent()->GetOwner();
}

FName AActor::GetAttachParentSocketName() const { return RootComponent ? RootComponent->GetAttachSocketName() : FName(); }

void AActor::GetAttachedActors(TArray<AActor*>& OutActors, bool bResetArray, bool bRecursivelyIncludeAttachedActors) const {
    if (bResetArray) OutActors.Reset();
    if (RootComponent == nullptr) return;
    TArray<USceneComponent*> Descendants;
    RootComponent->GetChildrenComponents(true, Descendants);
    for (USceneComponent* Component : Descendants) {
        AActor* Actor = Component->GetOwner();
        if (Actor == nullptr || Actor == this || OutActors.Contains(Actor)) continue;
        OutActors.Add(Actor);
        if (bRecursivelyIncludeAttachedActors) Actor->GetAttachedActors(OutActors, false, true);
    }
}

// -- Visibility --

void AActor::SetActorHiddenInGame(bool bNewHidden) {
    if (bHidden == bNewHidden) return;
    bHidden = bNewHidden;
}

void AActor::SetActorLocallyHidden(bool NewHidden, FName Tag) { bLocallyHidden = NewHidden; }

}

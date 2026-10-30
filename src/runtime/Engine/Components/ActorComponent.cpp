#include "ue5parse/runtime/Engine/Components/ActorComponent.h"

#include "ue5parse/runtime/Engine/Engine/Level.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

// UE's Components/ActorComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

UActorComponent::UActorComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    OwnerPrivate = GetTypedOuter<AActor>();
    PrimaryComponentTick.TickGroup = TG_DuringPhysics;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.bCanEverTick = false;
}

void UActorComponent::PostInitProperties() {
    Super::PostInitProperties();
    // Instance components are added during the owner's initialization.
    if (OwnerPrivate != nullptr && CreationMethod != EComponentCreationMethod::Instance) OwnerPrivate->AddOwnedComponent(this);
}

void UActorComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd()) return;
    // UCSModifiedProperties: FSimpleMemberReference = MemberParent, MemberName, MemberGuid. Nothing reads them.
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(28)) * 28);
}

UWorld* UActorComponent::GetWorld() const {
    if (WorldPrivate != nullptr) return WorldPrivate;
    // Unregistered components find their world through their owner or their outers.
    return OwnerPrivate ? OwnerPrivate->GetWorld() : Super::GetWorld();
}

int32 UActorComponent::GetFunctionCallspace(UFunction* Function, FFrame* Stack) {
    return OwnerPrivate ? OwnerPrivate->GetFunctionCallspace(Function, Stack) : FunctionCallspace::Local;
}

ENetRole UActorComponent::GetOwnerRole() const { return OwnerPrivate ? OwnerPrivate->GetLocalRole().GetValue() : ROLE_Authority; }

bool UActorComponent::IsNetSimulating() const { return bReplicates && GetOwnerRole() != ROLE_Authority; }

ENetMode UActorComponent::GetNetMode() const { return OwnerPrivate ? OwnerPrivate->GetNetMode() : NM_Standalone; }

void UActorComponent::RegisterComponent() {
    AActor* Owner = GetOwner();
    UWorld* World = Owner ? Owner->GetWorld() : GetWorld();
    if (World != nullptr) RegisterComponentWithWorld(World);
}

void UActorComponent::RegisterComponentWithWorld(UWorld* InWorld) {
    if (IsGarbage() || bRegistered || InWorld == nullptr) return;
    // A component of a template never registers.
    if (IsTemplate()) return;
    AActor* Owner = GetOwner();
    if (Owner != nullptr && Owner->IsTemplate()) return;
    WorldPrivate = InWorld;
    OnRegister();
    // Components that register after the owner began play catch up (UE's ExecuteRegisterEvents and the tick part of
    // RegisterComponentWithWorld).
    if (Owner != nullptr && (Owner->HasActorBegunPlay() || Owner->IsActorBeginningPlay())) {
        RegisterAllComponentTickFunctions(true);
        if (Owner->IsActorInitialized() && bWantsInitializeComponent && !bHasBeenInitialized) InitializeComponent();
        if (!bHasBegunPlay) BeginPlay();
    }
}

void UActorComponent::UnregisterComponent() {
    if (!bRegistered) return;
    RegisterAllComponentTickFunctions(false);
    OnUnregister();
    WorldPrivate = nullptr;
}

void UActorComponent::OnRegister() {
    bRegistered = true;
    if (!bAutoActivate) return;
    AActor* Owner = GetOwner();
    if (WorldPrivate == nullptr || !WorldPrivate->IsGameWorld() || Owner == nullptr || Owner->IsActorInitialized()) Activate(true);
}

void UActorComponent::OnUnregister() { bRegistered = false; }

void UActorComponent::InitializeComponent() { bHasBeenInitialized = true; }

void UActorComponent::UninitializeComponent() { bHasBeenInitialized = false; }

void UActorComponent::BeginPlay() {
    if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native)) ReceiveBeginPlay();
    bHasBegunPlay = true;
}

void UActorComponent::EndPlay(EEndPlayReason::Type EndPlayReason) {
    if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
        ReceiveEndPlay(EndPlayReason);
    bHasBegunPlay = false;
}

void UActorComponent::RegisterAllComponentTickFunctions(bool bRegister) {
    if (!bRegistered || bTickFunctionsRegistered == bRegister) return;
    RegisterComponentTickFunctions(bRegister);
    bTickFunctionsRegistered = bRegister;
}

void UActorComponent::RegisterComponentTickFunctions(bool bRegister) {
    if (bRegister) {
        if (SetupActorComponentTickFunction(&PrimaryComponentTick)) PrimaryComponentTick.Target = this;
    } else if (PrimaryComponentTick.IsTickFunctionRegistered()) {
        PrimaryComponentTick.UnRegisterTickFunction();
    }
}

bool UActorComponent::SetupActorComponentTickFunction(FTickFunction* TickFunction) {
    if (!TickFunction->bCanEverTick || IsTemplate()) return false;
    AActor* MyOwner = GetOwner();
    if (MyOwner != nullptr && MyOwner->IsTemplate()) return false;
    ULevel* ComponentLevel = MyOwner ? MyOwner->GetLevel() : (GetWorld() ? GetWorld()->PersistentLevel.Get() : nullptr);
    TickFunction->SetTickFunctionEnable(TickFunction->bStartWithTickEnabled || TickFunction->IsTickFunctionEnabled());
    TickFunction->RegisterTickFunction(ComponentLevel);
    return true;
}

void UActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native)) ReceiveTick(DeltaTime);
}

void UActorComponent::SetComponentTickEnabled(bool bEnabled) {
    if (PrimaryComponentTick.bCanEverTick && !IsTemplate()) PrimaryComponentTick.SetTickFunctionEnable(bEnabled);
}

void UActorComponent::AddTickPrerequisiteActor(AActor* PrerequisiteActor) {
    if (PrimaryComponentTick.bCanEverTick && PrerequisiteActor && PrerequisiteActor->PrimaryActorTick.bCanEverTick)
        PrimaryComponentTick.AddPrerequisite(PrerequisiteActor, PrerequisiteActor->PrimaryActorTick);
}

void UActorComponent::AddTickPrerequisiteComponent(UActorComponent* PrerequisiteComponent) {
    if (PrimaryComponentTick.bCanEverTick && PrerequisiteComponent && PrerequisiteComponent->PrimaryComponentTick.bCanEverTick)
        PrimaryComponentTick.AddPrerequisite(PrerequisiteComponent, PrerequisiteComponent->PrimaryComponentTick);
}

void UActorComponent::DestroyComponent(bool bPromoteChildren) {
    if (bIsBeingDestroyed || IsGarbage()) return;
    bIsBeingDestroyed = true;
    if (bHasBegunPlay) EndPlay(EEndPlayReason::Destroyed);
    if (bRegistered) UnregisterComponent();
    if (bHasBeenInitialized) UninitializeComponent();
    if (AActor* Owner = GetOwner()) {
        Owner->RemoveOwnedComponent(this);
        if (Owner->GetRootComponent() == this) Owner->SetRootComponent(nullptr);
    }
    OnComponentDestroyed(false);
    MarkAsGarbage();
}

void UActorComponent::K2_DestroyComponent(UObject* Object) {
    AActor* Owner = GetOwner();
    if (Owner == nullptr || Owner == Object) DestroyComponent();
}

void UActorComponent::Activate(bool bReset) {
    if (!bReset && !ShouldActivate()) return;
    SetComponentTickEnabled(true);
    SetActiveFlag(true);
    // FActorComponentActivatedSignature(UActorComponent* Component, bool bReset).
    struct {
        UActorComponent* Component;
        bool bReset;
    } Parms{this, bReset};
    OnComponentActivated.ProcessMulticastDelegate(&Parms);
}

void UActorComponent::Deactivate() {
    if (ShouldActivate()) return;
    SetComponentTickEnabled(false);
    SetActiveFlag(false);
    // FActorComponentDeactivateSignature(UActorComponent* Component).
    UActorComponent* Parms = this;
    OnComponentDeactivated.ProcessMulticastDelegate(&Parms);
}

void UActorComponent::SetActive(bool bNewActive, bool bReset) {
    if (bNewActive != IsActive() || bReset) {
        if (bNewActive) Activate(bReset);
        else Deactivate();
    }
}

void FActorComponentTickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType) {
    if (Target == nullptr || !IsValid(Target) || Target->IsBeingDestroyed()) return;
    if (TickType == LEVELTICK_ViewportsOnly) return;
    Target->TickComponent(DeltaTime, TickType, this);
}

FString FActorComponentTickFunction::DiagnosticMessage() {
    return Target ? Target->GetFullName() + FString("[TickComponent]") : FString("<NULL>[TickComponent]");
}

}

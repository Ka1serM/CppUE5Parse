#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Engine/Engine/BlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Engine/InheritableComponentHandler.h"
#include "ue5parse/runtime/Engine/Engine/SCS_Node.h"
#include "ue5parse/runtime/Engine/Engine/SimpleConstructionScript.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

// UE's ActorConstruction.cpp (UE 5.3): spawning an actor's components from its class, the construction scripts, and
// the components Blueprints add at run time.

namespace ue5parse::runtime {

namespace {

// UE's FindFirstFreeName: the base name, else Base_1, Base_2... that no subobject of the outer has.
FName FindFirstFreeName(UObject* Outer, FName BaseName) {
    if (StaticFindObjectFast(nullptr, Outer, BaseName) == nullptr) return BaseName;
    const std::string_view Text = BaseName.getText();
    for (uint32 Number = std::max<uint32>(BaseName.getNumber(), 1);; Number++) {
        const FName Candidate(Text, BaseName.getHash(), Number + 1);
        if (StaticFindObjectFast(nullptr, Outer, Candidate) == nullptr) return Candidate;
    }
}

}

void AActor::ApplySpawnTransform(const FTransform& SpawnTransform, ESpawnActorScaleMethod TransformScaleMethod) {
    const FTransform RootTransform(RootComponent->GetRelativeRotation(), RootComponent->GetRelativeLocation(), RootComponent->GetRelativeScale3D());
    FTransform FinalRootComponentTransform = RootTransform;
    switch (TransformScaleMethod) {
    case ESpawnActorScaleMethod::OverrideRootScale: FinalRootComponentTransform = SpawnTransform; break;
    case ESpawnActorScaleMethod::MultiplyWithRoot:
    case ESpawnActorScaleMethod::SelectDefaultAtRuntime: FinalRootComponentTransform = RootTransform * SpawnTransform; break;
    }
    RootComponent->SetWorldTransform(FinalRootComponentTransform, false, nullptr, ETeleportType::ResetPhysics);
}

void AActor::PostSpawnInitialize(const FTransform& SpawnTransform, AActor* InOwner, APawn* InInstigator, bool bRemoteOwned, bool bNoFail,
                                 bool bDeferConstruction, ESpawnActorScaleMethod TransformScaleMethod) {
    UWorld* World = GetWorld();
    CreationTime = World ? World->GetTimeSeconds() : 0.0;
    ExchangeNetRoles(bRemoteOwned);
    SetOwner(InOwner);
    SetInstigator(InInstigator);
    // A native scene component becomes the root when the class set none (UE's FixupNativeActorComponents).
    if (RootComponent == nullptr)
        for (UActorComponent* Component : OwnedComponents) {
            USceneComponent* Scene = Cast<USceneComponent>(Component);
            if (Scene == nullptr || Scene->GetAttachParent() != nullptr || Scene->CreationMethod != EComponentCreationMethod::Native) continue;
            SetRootComponent(Scene);
            break;
        }
    if (RootComponent != nullptr) ApplySpawnTransform(SpawnTransform, TransformScaleMethod);
    for (UActorComponent* Component : TArray<UActorComponent*>(OwnedComponents))
        if (Component != nullptr && !Component->HasBeenCreated()) Component->OnComponentCreated();
    // A Blueprint actor without a root registers after its construction script made one.
    const bool bHasDeferredComponentRegistration = RootComponent == nullptr && Cast<UBlueprintGeneratedClass>(GetClass()) != nullptr;
    if (!bHasDeferredComponentRegistration && World != nullptr) RegisterAllComponents();
    if (IsGarbage() && !bNoFail) return;
    if (bDeferConstruction) {
        DeferredSpawnTransform = SpawnTransform;
        bHasDeferredSpawnTransform = true;
        return;
    }
    FinishSpawning(SpawnTransform, true);
}

void AActor::FinishSpawning(const FTransform& Transform, bool bIsDefaultTransform) {
    if (bHasFinishedSpawning) return;
    bHasFinishedSpawning = true;
    FTransform FinalRootComponentTransform = RootComponent ? RootComponent->GetComponentTransform() : Transform;
    // A deferred spawn that finishes with another transform: the template's transform is recovered from the spawn
    // transform and composed with the new one (UE's GSpawnActorDeferredTransformCache).
    if (RootComponent != nullptr && !bIsDefaultTransform && bHasDeferredSpawnTransform) {
        bHasDeferredSpawnTransform = false;
        if (!DeferredSpawnTransform.Equals(Transform))
            FinalRootComponentTransform = RootComponent->GetComponentTransform() * DeferredSpawnTransform.Inverse() * Transform;
    }
    ExecuteConstruction(FinalRootComponentTransform, bIsDefaultTransform);
    PostActorConstruction();
}

bool AActor::ExecuteConstruction(const FTransform& Transform, bool bIsDefaultTransform) {
    if (RootComponent != nullptr && !bIsDefaultTransform)
        RootComponent->SetWorldTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
    TArray<const UBlueprintGeneratedClass*> ParentBPClassStack;
    const bool bErrorFree = UBlueprintGeneratedClass::GetGeneratedClassesHierarchy(GetClass(), ParentBPClassStack);
    if (ParentBPClassStack.Num() == 0) return true;
    UWorld* World = GetWorld();
    if (!bErrorFree) {
        if (RootComponent == nullptr) {
            USceneComponent* Root = NewObject<USceneComponent>(this);
            Root->CreationMethod = EComponentCreationMethod::SimpleConstructionScript;
            Root->SetRelativeTransform(Transform);
            SetRootComponent(Root);
            Root->RegisterComponent();
        }
        return false;
    }
    const TArray<UActorComponent*> PreSCSComponents = OwnedComponents;
    TArray<USceneComponent*> NativeSceneComponents;
    for (UActorComponent* Component : PreSCSComponents) {
        USceneComponent* Scene = Cast<USceneComponent>(Component);
        if (Scene == nullptr || Scene->CreationMethod != EComponentCreationMethod::Native || !Scene->GetOuter()->IsA<AActor>()) continue;
        if (RootComponent == nullptr && Scene->GetAttachParent() == nullptr) {
            SetRootComponent(Scene);
            if (!bIsDefaultTransform) RootComponent->SetWorldTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
        }
        NativeSceneComponents.Add(Scene);
    }
    for (int32 Index = ParentBPClassStack.Num() - 1; Index >= 0; Index--) {
        const UBlueprintGeneratedClass* CurrentBPGClass = ParentBPClassStack[Index];
        if (USimpleConstructionScript* SCS = CurrentBPGClass->SimpleConstructionScript)
            SCS->ExecuteScriptOnActor(this, NativeSceneComponents, Transform, bIsDefaultTransform);
        UBlueprintGeneratedClass::CreateComponentsForActor(CurrentBPGClass, this);
    }
    if (World != nullptr) {
        RegisterAllComponents();
        for (UActorComponent* Component : TArray<UActorComponent*>(OwnedComponents))
            if (Component != nullptr && !Component->IsRegistered() && IsValid(Component)) Component->RegisterComponentWithWorld(World);
    }
    bRunningUserConstructionScript = true;
    UserConstructionScript();
    bRunningUserConstructionScript = false;
    UBlueprintGeneratedClass::BindDynamicDelegates(GetClass(), this);
    return true;
}

void AActor::PostActorConstruction() {
    UWorld* World = GetWorld();
    // A client's copy of a server actor begins play with its first replicated bunch (PostNetInit).
    const bool bDeferBeginPlay = bExchangedRoles && RemoteRole == ROLE_Authority;
    InitializeComponents();
    if (IsGarbage()) return;
    PostInitializeComponents();
    if (IsGarbage()) return;
    if (!bDeferBeginPlay && World != nullptr && World->HasBegunPlay()) DispatchBeginPlay();
}

UActorComponent* AActor::CreateComponentFromTemplate(UActorComponent* Template, FName InName) {
    if (Template == nullptr) return nullptr;
    if (InName.isNone()) InName = FindFirstFreeName(this, Template->GetFName());
    UActorComponent* NewActorComp = NewObject<UActorComponent>(this, Template->GetClass(), InName, RF_NoFlags, Template);
    // UE's PostCreateBlueprintComponent.
    NewActorComp->CreationMethod =
        bRunningUserConstructionScript ? EComponentCreationMethod::UserConstructionScript : EComponentCreationMethod::Instance;
    BlueprintCreatedComponents.AddUnique(NewActorComp);
    AddOwnedComponent(NewActorComp);
    return NewActorComp;
}

UActorComponent* AActor::AddComponent(FName TemplateName, bool bManualAttachment, const FTransform& RelativeTransform,
                                      UObject* ComponentTemplateContext, bool bDeferredFinish) {
    if (GetWorld() == nullptr) return nullptr;
    UActorComponent* Template = nullptr;
    for (UClass* TemplateOwnerClass = ComponentTemplateContext ? ComponentTemplateContext->GetClass() : GetClass();
         TemplateOwnerClass != nullptr && Template == nullptr; TemplateOwnerClass = TemplateOwnerClass->GetSuperClass())
        if (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(TemplateOwnerClass)) Template = BPGC->FindComponentTemplateByName(TemplateName);
    UActorComponent* NewActorComp = CreateComponentFromTemplate(Template, FName());
    if (!bDeferredFinish) FinishAddComponent(NewActorComp, bManualAttachment, RelativeTransform);
    return NewActorComp;
}

UActorComponent* AActor::AddComponentByClass(TSubclassOf<UActorComponent> Class, bool bManualAttachment, const FTransform& RelativeTransform,
                                             bool bDeferredFinish) {
    if (Class.Get() == nullptr || GetWorld() == nullptr) return nullptr;
    UActorComponent* NewActorComp = NewObject<UActorComponent>(this, Class.Get());
    NewActorComp->CreationMethod =
        bRunningUserConstructionScript ? EComponentCreationMethod::UserConstructionScript : EComponentCreationMethod::Instance;
    BlueprintCreatedComponents.AddUnique(NewActorComp);
    AddOwnedComponent(NewActorComp);
    if (!bDeferredFinish) FinishAddComponent(NewActorComp, bManualAttachment, RelativeTransform);
    return NewActorComp;
}

void AActor::FinishAddComponent(UActorComponent* Component, bool bManualAttachment, const FTransform& RelativeTransform) {
    if (Component == nullptr) return;
    Component->OnComponentCreated();
    if (USceneComponent* Scene = Cast<USceneComponent>(Component)) {
        if (!bManualAttachment) {
            if (RootComponent == nullptr) RootComponent = Scene;
            else Scene->AttachParent = RootComponent;
        }
        Scene->SetRelativeTransform(RelativeTransform);
    }
    Component->RegisterComponent();
}

void AActor::ForEachComponentOfActorClassDefault(const UClass* ActorClass, const UClass* ComponentClass,
                                                 const std::function<bool(const UActorComponent*)>& Func) {
    if (ActorClass == nullptr) return;
    const auto Filter = [&](const UActorComponent* Template) {
        if (Template == nullptr) return true;
        return ComponentClass != nullptr && !Template->IsA(ComponentClass) ? true : Func(Template);
    };
    const AActor* CDO = Cast<AActor>(const_cast<UClass*>(ActorClass)->GetDefaultObject());
    if (CDO != nullptr)
        for (const UActorComponent* Component : CDO->GetComponents())
            if (!Filter(Component)) return;
    UBlueprintGeneratedClass* ActorBPGC = Cast<UBlueprintGeneratedClass>(const_cast<UClass*>(ActorClass));
    if (ActorBPGC == nullptr) return;
    TArray<const UBlueprintGeneratedClass*> Hierarchy;
    UBlueprintGeneratedClass::GetGeneratedClassesHierarchy(ActorClass, Hierarchy);
    for (const UBlueprintGeneratedClass* Current : Hierarchy) {
        const USimpleConstructionScript* SCS = Current->SimpleConstructionScript;
        if (SCS == nullptr) continue;
        for (const USCS_Node* Node : SCS->GetAllNodes())
            if (!Filter(Node->GetActualComponentTemplate(ActorBPGC))) return;
    }
}

}

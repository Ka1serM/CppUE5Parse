#include "ue5parse/runtime/Engine/Engine/SCS_Node.h"

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/Engine/Engine/BlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Engine/InheritableComponentHandler.h"
#include "ue5parse/runtime/Engine/Engine/SimpleConstructionScript.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

// UE's SCS_Node.cpp (UE 5.3).

namespace ue5parse::runtime {

USimpleConstructionScript* USCS_Node::GetSCS() const { return Cast<USimpleConstructionScript>(GetOuter()); }

UActorComponent* USCS_Node::GetActualComponentTemplate(UBlueprintGeneratedClass* ActualBPGC) const {
    UActorComponent* Overridden = nullptr;
    const USimpleConstructionScript* SCS = GetSCS();
    if (ActualBPGC != nullptr && SCS != ActualBPGC->SimpleConstructionScript) {
        const FComponentKey ComponentKey(this);
        do {
            if (UInheritableComponentHandler* Handler = ActualBPGC->GetInheritableComponentHandler())
                Overridden = Handler->GetOverridenComponentTemplate(ComponentKey);
            ActualBPGC = Cast<UBlueprintGeneratedClass>(ActualBPGC->GetSuperClass());
        } while (Overridden == nullptr && ActualBPGC != nullptr && SCS != ActualBPGC->SimpleConstructionScript);
    }
    return Overridden ? Overridden : ComponentTemplate.Get();
}

UActorComponent* USCS_Node::ExecuteNodeOnActor(AActor* Actor, USceneComponent* ParentComponent, const FTransform* RootTransform,
                                               bool bIsDefaultTransform) {
    UBlueprintGeneratedClass* ActualBPGC = Cast<UBlueprintGeneratedClass>(Actor->GetClass());
    UActorComponent* NewActorComp = Actor->CreateComponentFromTemplate(GetActualComponentTemplate(ActualBPGC), InternalVariableName);
    if (NewActorComp == nullptr) return nullptr;
    NewActorComp->CreationMethod = EComponentCreationMethod::SimpleConstructionScript;
    if (!NewActorComp->HasBeenCreated()) NewActorComp->OnComponentCreated();
    USceneComponent* NewSceneComp = Cast<USceneComponent>(NewActorComp);
    if (NewSceneComp != nullptr) {
        if (!IsValid(ParentComponent) || ParentComponent == NewSceneComp) {
            // The root: the spawn transform, the component's relative transform applied (MultiplyWithRoot), its own
            // scale for the default transform.
            FTransform WorldTransform = NewSceneComp->GetRelativeTransform() * *RootTransform;
            if (bIsDefaultTransform) WorldTransform.SetScale3D(NewSceneComp->GetRelativeScale3D());
            NewSceneComp->SetWorldTransform(WorldTransform);
            Actor->SetRootComponent(NewSceneComp);
        } else {
            // UE's SetupAttachment: the component attaches when it registers.
            NewSceneComp->AttachParent = ParentComponent;
            NewSceneComp->AttachSocketName = AttachToName;
        }
        if (Actor->GetWorld() != nullptr) NewSceneComp->RegisterComponent();
    }
    if (!InternalVariableName.isNone())
        if (FObjectPropertyBase* Prop = FindFProperty<FObjectPropertyBase>(Actor->GetClass(), InternalVariableName))
            if (Prop->PropertyClass == nullptr || NewActorComp->IsA(Prop->PropertyClass)) Prop->SetObjectPropertyValue_InContainer(Actor, NewActorComp);
    for (USCS_Node* Child : ChildNodes)
        if (Child != nullptr) Child->ExecuteNodeOnActor(Actor, NewSceneComp, nullptr, bIsDefaultTransform);
    return NewActorComp;
}

}

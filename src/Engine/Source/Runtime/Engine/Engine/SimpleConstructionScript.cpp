#include "Engine/Source/Runtime/Engine/Engine/SimpleConstructionScript.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"
#include "Engine/Source/Runtime/Engine/Engine/SCS_Node.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

namespace ue5parse::runtime {

namespace {

void AddNodes(USCS_Node* Node, TArray<USCS_Node*>& OutNodes) {
    if (Node == nullptr)
        return;
    OutNodes.Add(Node);
    for (USCS_Node* Child : Node->GetChildNodes())
        AddNodes(Child, OutNodes);
}

}

TArray<USCS_Node*> USimpleConstructionScript::GetAllNodes() const {
    TArray<USCS_Node*> Nodes;
    for (USCS_Node* Root : RootNodes)
        AddNodes(Root, Nodes);
    return Nodes;
}

void USimpleConstructionScript::ExecuteScriptOnActor(AActor* Actor, const TArray<USceneComponent*>& NativeSceneComponents, const FTransform& RootTransform,
                                                     bool bIsDefaultTransform) {
    if (RootNodes.Num() == 0) {
        if (Actor->GetRootComponent() != nullptr)
            return;
        USceneComponent* SceneComp = NewObject<USceneComponent>(Actor);
        SceneComp->CreationMethod = EComponentCreationMethod::SimpleConstructionScript;
        SceneComp->SetWorldTransform(RootTransform);
        Actor->SetRootComponent(SceneComp);
        SceneComp->RegisterComponent();
        return;
    }
    USceneComponent* RootComponent = Actor->GetRootComponent();
    for (USCS_Node* RootNode : RootNodes) {
        if (RootNode == nullptr || (RootNode == DefaultSceneRootNode && RootComponent != nullptr))
            continue;
        USceneComponent* ParentComponent = nullptr;
        if (!RootNode->ParentComponentOrVariableName.isNone()) {
            if (RootNode->bIsParentComponentNative) {
                for (USceneComponent* Native : NativeSceneComponents)
                    if (Native->GetFName() == RootNode->ParentComponentOrVariableName) {
                        ParentComponent = Native;
                        break;
                    }
            } else if (FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Actor->GetClass(), RootNode->ParentComponentOrVariableName)) {
                ParentComponent = Cast<USceneComponent>(Property->GetObjectPropertyValue_InContainer(Actor));
            }
        }
        RootNode->ExecuteNodeOnActor(Actor, ParentComponent ? ParentComponent : RootComponent, &RootTransform, bIsDefaultTransform);
        RootComponent = Actor->GetRootComponent();
    }
}

}

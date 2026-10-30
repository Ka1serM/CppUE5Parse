#pragma once

#include "ue5parse/runtime/Core/Math/Transform.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "SimpleConstructionScript.generated.h"

namespace ue5parse::runtime {

class AActor;
class USCS_Node;
class USceneComponent;

// USimpleConstructionScript: the tree of components a Blueprint class adds (UE's Engine/SimpleConstructionScript.h).
UCLASS()
class USimpleConstructionScript : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<USCS_Node*> RootNodes;
    UPROPERTY()
    TArray<USCS_Node*> AllNodes;
    UPROPERTY()
    USCS_Node* DefaultSceneRootNode = nullptr;

    const TArray<USCS_Node*>& GetRootNodes() const { return RootNodes; }
    // Every node, depth first from the root nodes (UE's GetAllNodes).
    TArray<USCS_Node*> GetAllNodes() const;
    // Creates the nodes' components on the actor: each root node under its parent (a native component or a Blueprint
    // variable of the actor), else as the root at RootTransform (UE's ExecuteScriptOnActor).
    void ExecuteScriptOnActor(AActor* Actor, const TArray<USceneComponent*>& NativeSceneComponents, const FTransform& RootTransform,
                              bool bIsDefaultTransform);
};

}

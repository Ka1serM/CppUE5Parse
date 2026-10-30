#pragma once

#include "ue5parse/runtime/Core/Math/Transform.h"
#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"

#include "SCS_Node.generated.h"

namespace ue5parse::runtime {

class AActor;
class UActorComponent;
class UBlueprintGeneratedClass;
class USceneComponent;
class USimpleConstructionScript;

// USCS_Node: one component of a simple construction script: its template, its variable, and what it attaches to (UE's
// Engine/SCS_Node.h).
UCLASS()
class USCS_Node : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UActorComponent> ComponentTemplate = nullptr;
    UPROPERTY()
    FName AttachToName;
    // A root node's parent: a native component or a component of a parent Blueprint.
    UPROPERTY()
    FName ParentComponentOrVariableName;
    UPROPERTY()
    FName ParentComponentOwnerClassName;
    UPROPERTY()
    bool bIsParentComponentNative = false;
    UPROPERTY()
    TArray<TObjectPtr<USCS_Node>> ChildNodes;
    UPROPERTY()
    FGuid VariableGuid;
    UPROPERTY()
    FName InternalVariableName;

    FName GetVariableName() const { return InternalVariableName; }
    const TArray<TObjectPtr<USCS_Node>>& GetChildNodes() const { return ChildNodes; }
    USimpleConstructionScript* GetSCS() const;
    // The template the actor class uses: an override of a child class's InheritableComponentHandler, else the node's.
    UActorComponent* GetActualComponentTemplate(UBlueprintGeneratedClass* ActualBPGC) const;
    // Creates the node's component on the actor, attaches it, sets the actor's variable, then does the same for the
    // child nodes (UE's ExecuteNodeOnActor).
    UActorComponent* ExecuteNodeOnActor(AActor* Actor, USceneComponent* ParentComponent, const FTransform* RootTransform, bool bIsDefaultTransform);
};

}

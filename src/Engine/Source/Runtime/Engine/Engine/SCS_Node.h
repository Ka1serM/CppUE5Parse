#pragma once

#include "Engine/Source/Runtime/Core/Math/Transform.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "SCS_Node.generated.h"

namespace ue5parse::runtime {

class AActor;
class UActorComponent;
class UBlueprintGeneratedClass;
class USceneComponent;
class USimpleConstructionScript;

UCLASS()
class USCS_Node : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UActorComponent> ComponentTemplate = nullptr;
    UPROPERTY()
    FName AttachToName;
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
    UActorComponent* GetActualComponentTemplate(UBlueprintGeneratedClass* ActualBPGC) const;
    UActorComponent* ExecuteNodeOnActor(AActor* Actor, USceneComponent* ParentComponent, const FTransform* RootTransform, bool bIsDefaultTransform);
};

}

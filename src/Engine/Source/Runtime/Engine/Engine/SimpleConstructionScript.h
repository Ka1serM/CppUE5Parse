#pragma once

#include "Engine/Source/Runtime/Core/Math/Transform.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "SimpleConstructionScript.generated.h"

namespace ue5parse::runtime {

class AActor;
class USCS_Node;
class USceneComponent;

UCLASS()
class USimpleConstructionScript : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<USCS_Node>> RootNodes;
    UPROPERTY()
    TArray<TObjectPtr<USCS_Node>> AllNodes;
    UPROPERTY()
    TObjectPtr<USCS_Node> DefaultSceneRootNode = nullptr;

    const TArray<TObjectPtr<USCS_Node>>& GetRootNodes() const { return RootNodes; }
    TArray<USCS_Node*> GetAllNodes() const;
    void ExecuteScriptOnActor(AActor* Actor, const TArray<USceneComponent*>& NativeSceneComponents, const FTransform& RootTransform, bool bIsDefaultTransform);
};

}

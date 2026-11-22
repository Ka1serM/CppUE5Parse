#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "PCGComponent.generated.h"

namespace ue5parse::runtime {

class UPCGGraphInstance;

UCLASS()
class UPCGComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UPCGComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UObject> Graph_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UPCGGraphInstance> GraphInstance = nullptr;
    UPROPERTY()
    int32 Seed = 42;
    UPROPERTY()
    bool bActivated = true;
    UPROPERTY()
    bool bIsComponentPartitioned = false;
};

}

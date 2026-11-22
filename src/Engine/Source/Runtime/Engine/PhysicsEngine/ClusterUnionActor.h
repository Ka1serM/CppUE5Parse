#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "ClusterUnionActor.generated.h"

namespace ue5parse::runtime {

class UClusterUnionComponent;

UCLASS()
class AClusterUnionActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UClusterUnionComponent> ClusterUnion = nullptr;

    explicit AClusterUnionActor(const FObjectInitializer& ObjectInitializer);
};

}

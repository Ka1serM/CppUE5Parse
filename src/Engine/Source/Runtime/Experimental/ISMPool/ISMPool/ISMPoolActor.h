#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "ISMPoolActor.generated.h"

namespace ue5parse::runtime {

class UISMPoolComponent;
class UISMPoolDebugDrawComponent;

UCLASS()
class AISMPoolActor : public AActor {
    GENERATED_BODY()
public:
    explicit AISMPoolActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UISMPoolComponent> ISMPoolComp = nullptr;
    UPROPERTY()
    TObjectPtr<UISMPoolDebugDrawComponent> ISMPoolDebugDrawComp = nullptr;
};

}

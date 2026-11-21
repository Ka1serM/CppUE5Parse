#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "GroomActor.generated.h"

namespace ue5parse::runtime {

class UGroomComponent;
class UBillboardComponent;

UCLASS()
class AGroomActor : public AActor {
    GENERATED_BODY()
public:
    explicit AGroomActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UGroomComponent> GroomComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;
};

}

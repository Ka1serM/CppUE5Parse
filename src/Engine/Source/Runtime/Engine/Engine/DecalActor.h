#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "DecalActor.generated.h"

namespace ue5parse::runtime {

class UDecalComponent;

UCLASS()
class ADecalActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UDecalComponent> Decal = nullptr;

    explicit ADecalActor(const FObjectInitializer& ObjectInitializer);
};

}

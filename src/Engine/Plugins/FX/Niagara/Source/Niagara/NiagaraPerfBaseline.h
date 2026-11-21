#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "NiagaraPerfBaseline.generated.h"

namespace ue5parse::runtime {

class UTextRenderComponent;

UCLASS()
class ANiagaraPerfBaselineActor : public AActor {
    GENERATED_BODY()
public:
    explicit ANiagaraPerfBaselineActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UObject> Controller = nullptr;
    UPROPERTY()
    TObjectPtr<UTextRenderComponent> Label = nullptr;
};

}

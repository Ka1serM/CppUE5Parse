#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "FieldSystemActor.generated.h"

namespace ue5parse::runtime {

class UFieldSystemComponent;

UCLASS()
class AFieldSystemActor : public AActor {
    GENERATED_BODY()
public:
    explicit AFieldSystemActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UFieldSystemComponent> FieldSystemComponent = nullptr;
};

}

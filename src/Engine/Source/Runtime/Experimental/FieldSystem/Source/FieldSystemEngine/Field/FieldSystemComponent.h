#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "FieldSystemComponent.generated.h"

namespace ue5parse::runtime {

class UFieldSystem;

UCLASS()
class UFieldSystemComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UFieldSystemComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UFieldSystem> FieldSystem = nullptr;
};

}

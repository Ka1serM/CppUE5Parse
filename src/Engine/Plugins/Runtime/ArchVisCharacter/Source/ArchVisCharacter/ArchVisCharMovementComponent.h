#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/CharacterMovementComponent.h"

#include "ArchVisCharMovementComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UArchVisCharMovementComponent : public UCharacterMovementComponent {
    GENERATED_BODY()
public:
    explicit UArchVisCharMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

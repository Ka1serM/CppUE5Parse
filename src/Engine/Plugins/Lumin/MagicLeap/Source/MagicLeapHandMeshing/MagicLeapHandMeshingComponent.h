#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "MagicLeapHandMeshingComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMagicLeapHandMeshingComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UMagicLeapHandMeshingComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

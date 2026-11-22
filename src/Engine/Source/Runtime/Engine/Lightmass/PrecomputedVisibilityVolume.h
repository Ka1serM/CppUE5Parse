#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "PrecomputedVisibilityVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class APrecomputedVisibilityVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit APrecomputedVisibilityVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

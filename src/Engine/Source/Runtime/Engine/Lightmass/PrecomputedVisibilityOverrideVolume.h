#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "PrecomputedVisibilityOverrideVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class APrecomputedVisibilityOverrideVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit APrecomputedVisibilityOverrideVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeQualityLevel.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeQualityLevel : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 CookedQualityLevelIndex = INDEX_NONE;
};

}

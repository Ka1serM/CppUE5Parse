#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeRandom.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeRandom : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<float> Weights;
    UPROPERTY()
    TArray<bool> HasBeenUsed;
    UPROPERTY()
    int32 NumRandomUsed = 0;
    UPROPERTY()
    int32 PreselectAtLevelLoad = 0;
    UPROPERTY()
    uint8 bShouldExcludeFromBranchCulling : 1 = 0;
    UPROPERTY()
    uint8 bSoundCueExcludedFromBranchCulling : 1 = 0;
    UPROPERTY()
    uint8 bRandomizeWithoutReplacement : 1 = 1;
    UPROPERTY()
    TArray<int32> PIEHiddenNodes;
};

}

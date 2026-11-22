#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeDelay.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeDelay : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    float DelayMin = 0.0f;
    UPROPERTY()
    float DelayMax = 0.0f;
};

}

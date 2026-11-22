#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeDoppler.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeDoppler : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    float DopplerIntensity = 1.0f;
    UPROPERTY()
    bool bUseSmoothing = false;
    UPROPERTY()
    float SmoothingInterpSpeed = 5.0f;
};

}

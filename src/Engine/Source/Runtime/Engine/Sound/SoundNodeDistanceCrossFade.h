#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeDistanceCrossFade.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FDistanceDatum {
    GENERATED_BODY()
    UPROPERTY()
    float FadeInDistanceStart = 0;
    UPROPERTY()
    float FadeInDistanceEnd = 0;
    UPROPERTY()
    float FadeOutDistanceStart = 0;
    UPROPERTY()
    float FadeOutDistanceEnd = 0;
    UPROPERTY()
    float Volume = 1.0f;
};

UCLASS()
class USoundNodeDistanceCrossFade : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FDistanceDatum> CrossFadeInput;
};

}

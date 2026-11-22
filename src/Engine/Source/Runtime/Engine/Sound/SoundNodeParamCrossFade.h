#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNodeDistanceCrossFade.h"

#include "SoundNodeParamCrossFade.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeParamCrossFade : public USoundNodeDistanceCrossFade {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName ParamName;
};

}

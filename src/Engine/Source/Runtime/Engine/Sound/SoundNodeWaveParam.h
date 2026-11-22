#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeWaveParam.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeWaveParam : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName WaveParameterName;
};

}

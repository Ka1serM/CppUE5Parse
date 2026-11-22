#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeSwitch.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeSwitch : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName IntParameterName;
};

}

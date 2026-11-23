#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeBranch.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeBranch : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName BoolParameterName;
};

}

#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeGroupControl.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeGroupControl : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<int32> GroupSizes;
};

}

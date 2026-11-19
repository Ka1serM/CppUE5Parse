#pragma once

#include "Engine/Source/Runtime/Engine/Engine/LevelStreaming.h"

#include "LevelStreamingDynamic.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULevelStreamingDynamic : public ULevelStreaming {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint8 bInitiallyLoaded : 1 = false;
    UPROPERTY()
    uint8 bInitiallyVisible : 1 = false;
};

}

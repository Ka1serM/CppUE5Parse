#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "CullDistanceVolume.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FCullDistanceSizePair {
    GENERATED_BODY()
    UPROPERTY()
    float Size = 0.0f;
    UPROPERTY()
    float CullDistance = 0.0f;
};

UCLASS()
class ACullDistanceVolume : public AVolume {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FCullDistanceSizePair> CullDistances;
    UPROPERTY()
    uint8 bEnabled : 1 = true;

    explicit ACullDistanceVolume(const FObjectInitializer& ObjectInitializer);
};

}

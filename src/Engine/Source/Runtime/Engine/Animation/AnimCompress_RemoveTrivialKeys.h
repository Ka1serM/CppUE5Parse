#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCompress.h"

#include "AnimCompress_RemoveTrivialKeys.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAnimCompress_RemoveTrivialKeys : public UAnimCompress {
    GENERATED_BODY()
public:
    explicit UAnimCompress_RemoveTrivialKeys(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float MaxPosDiff;
    UPROPERTY()
    float MaxAngleDiff;
    UPROPERTY()
    float MaxScaleDiff;
};

}

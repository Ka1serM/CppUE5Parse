#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCompress.h"

#include "AnimCompress_RemoveLinearKeys.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAnimCompress_RemoveLinearKeys : public UAnimCompress {
    GENERATED_BODY()
public:
    explicit UAnimCompress_RemoveLinearKeys(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float MaxPosDiff;
    UPROPERTY()
    float MaxAngleDiff;
    UPROPERTY()
    float MaxScaleDiff;
    UPROPERTY()
    float MaxEffectorDiff;
    UPROPERTY()
    float MinEffectorDiff;
    UPROPERTY()
    float EffectorDiffSocket;
    UPROPERTY()
    float ParentKeyScale;
    UPROPERTY()
    bool bRetarget;
    UPROPERTY()
    bool bActuallyFilterLinearKeys;
};

}

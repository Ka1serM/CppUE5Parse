#pragma once

#include "Engine/Source/Runtime/MediaAssets/BaseMediaSource.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "ImgMediaSource.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UImgMediaSource : public UBaseMediaSource {
    GENERATED_BODY()
public:
    UPROPERTY()
    FDirectoryPath SequencePath;
    UPROPERTY()
    float FramesPerSecondOverride = 0.0f;
    UPROPERTY()
    FFrameRate FrameRateOverride = {.Numerator = 0, .Denominator = 0};
    UPROPERTY()
    FString ProxyOverride;
    UPROPERTY()
    FString LayerName;
    UPROPERTY()
    bool IsPathRelativeToProjectRoot_DEPRECATED = false;
    UPROPERTY()
    bool bFillGapsInSequence = true;
};

}

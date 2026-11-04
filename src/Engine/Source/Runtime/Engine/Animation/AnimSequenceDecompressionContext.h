#pragma once

#include <cmath>

#include "Engine/Source/Runtime/Engine/Animation/AnimCompressionTypes.h"

namespace ue5parse::runtime {

enum class EAnimInterpolationType : uint8;
class USkeleton;

struct FAnimSequenceDecompressionContext {
    FAnimSequenceDecompressionContext(const FFrameRate& InSamplingRate, int32 InNumberOfFrames, EAnimInterpolationType InInterpolation,
                                      FName InAnimName, const FCompressedAnimSequence& InCompressedAnimData,
                                      const TArray<FTransform>& InRefPoses, const USkeleton* InSourceSkeleton, bool bInIsBakedAdditive)
        : Interpolation(InInterpolation), AnimName(InAnimName), CompressedAnimData(InCompressedAnimData), RefPoses(InRefPoses),
          SamplingRate(InSamplingRate), NumberOfFrames(InNumberOfFrames), SourceSkeleton(InSourceSkeleton),
          bAdditiveAnimation(bInIsBakedAdditive) {}

    EAnimInterpolationType Interpolation;
    FName AnimName;
    const FCompressedAnimSequence& CompressedAnimData;

    double GetPlayableLength() const { return NumberOfFrames * SamplingRate.AsInterval(); }
    double GetRelativePosition() const { return NumberOfFrames > 0 ? FMath::Min(SamplingTime / static_cast<double>(NumberOfFrames), 1.0) : 0.0; }
    double GetInterpolatedEvaluationTime() const;
    void Seek(double SampleAtTime);
    const USkeleton* GetSourceSkeleton() const { return SourceSkeleton; }
    bool IsAdditiveAnimation() const { return bAdditiveAnimation; }
    const TArray<FTransform>& GetRefLocalPoses() const { return RefPoses; }

private:
    const TArray<FTransform>& RefPoses;
    FFrameRate SamplingRate;
    double SamplingTime = 0;
    int32 NumberOfFrames;
    const USkeleton* SourceSkeleton;
    bool bAdditiveAnimation;
};

}

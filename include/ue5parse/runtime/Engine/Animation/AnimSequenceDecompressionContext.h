#pragma once

#include <cmath>

#include "ue5parse/runtime/Engine/Animation/AnimCompressionTypes.h"

namespace ue5parse::runtime {

enum class EAnimInterpolationType : uint8;
class USkeleton;

// FAnimSequenceDecompressionContext: what the bone codecs sample from (UE's Animation/AnimSequenceDecompressionContext.h).
// The sampling time is in frames of the sampling rate.
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
    double GetEvaluationTime() const { return SamplingTime * SamplingRate.AsInterval(); }
    double GetRelativePosition() const { return NumberOfFrames > 0 ? FMath::Min(SamplingTime / static_cast<double>(NumberOfFrames), 1.0) : 0.0; }
    // The sampling time with step interpolation floored to the frame.
    double GetInterpolatedEvaluationTime() const;
    void Seek(double SampleAtTime);
    const USkeleton* GetSourceSkeleton() const { return SourceSkeleton; }
    bool IsAdditiveAnimation() const { return bAdditiveAnimation; }
    const TArray<FTransform>& GetRefLocalPoses() const { return RefPoses; }
    const TArray<FTrackToSkeletonMap>& GetTrackToSkeletonMap() const { return CompressedAnimData.CompressedTrackToSkeletonMapTable; }
    int32 GetNumberOfFrames() const { return NumberOfFrames; }
    const FFrameRate& GetSamplingRate() const { return SamplingRate; }

private:
    const TArray<FTransform>& RefPoses;
    FFrameRate SamplingRate;
    // FFrameTime as a decimal frame number.
    double SamplingTime = 0;
    int32 NumberOfFrames;
    const USkeleton* SourceSkeleton;
    bool bAdditiveAnimation;
};

}

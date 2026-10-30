#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"

namespace ue5parse::runtime {

void UAnimationAsset::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    // The skeleton's virtual bone guid when the asset was saved.
    Ar << SkeletonGuid;
}

void FBlendSampleData::NormalizeDataWeight(TArray<FBlendSampleData>& SampleDataList) {
    float TotalSum = 0.f;
    const int32 NumBones = SampleDataList[0].PerBoneBlendData.Num();
    TArray<float> PerBoneTotalSums;
    PerBoneTotalSums.SetNum(NumBones);
    for (float& Sum : PerBoneTotalSums) Sum = 0.f;
    for (const FBlendSampleData& Sample : SampleDataList) {
        UE5PARSE_CHECK(Sample.PerBoneBlendData.Num() == NumBones);
        TotalSum += Sample.TotalWeight;
        for (int32 BoneIndex = 0; BoneIndex < Sample.PerBoneBlendData.Num(); BoneIndex++) PerBoneTotalSums[BoneIndex] += Sample.PerBoneBlendData[BoneIndex];
    }
    if (TotalSum > ZERO_ANIMWEIGHT_THRESH) {
        if (FMath::Abs(TotalSum - 1.f) > ZERO_ANIMWEIGHT_THRESH)
            for (FBlendSampleData& Sample : SampleDataList) Sample.TotalWeight /= TotalSum;
    } else {
        for (FBlendSampleData& Sample : SampleDataList) Sample.TotalWeight = 1.0f / static_cast<float>(SampleDataList.Num());
    }
    for (int32 BoneIndex = 0; BoneIndex < NumBones; BoneIndex++) {
        if (PerBoneTotalSums[BoneIndex] > ZERO_ANIMWEIGHT_THRESH) {
            if (FMath::Abs(PerBoneTotalSums[BoneIndex] - 1.f) > ZERO_ANIMWEIGHT_THRESH)
                for (FBlendSampleData& Sample : SampleDataList) Sample.PerBoneBlendData[BoneIndex] /= PerBoneTotalSums[BoneIndex];
        } else {
            for (FBlendSampleData& Sample : SampleDataList) Sample.PerBoneBlendData[BoneIndex] = 1.0f / static_cast<float>(SampleDataList.Num());
        }
    }
}

}

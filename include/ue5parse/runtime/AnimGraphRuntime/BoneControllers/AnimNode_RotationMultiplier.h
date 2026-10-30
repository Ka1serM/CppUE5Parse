#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_RotationMultiplier.generated.h"

namespace ue5parse::runtime {

// Rotates a bone by a multiple of the twist of a source bone around an axis, relative to the reference pose (UE's
// BoneControllers/AnimNode_RotationMultiplier.h). A twist bone follows its limb with it.
USTRUCT()
struct FAnimNode_RotationMultiplier : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference TargetBone;
    UPROPERTY()
    FBoneReference SourceBone;
    UPROPERTY()
    float Multiplier = 0.f;
    UPROPERTY()
    TEnumAsByte<EBoneAxis> RotationAxisToRefer = BA_X;
    UPROPERTY()
    bool bIsAdditive = false;

protected:
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
};

}

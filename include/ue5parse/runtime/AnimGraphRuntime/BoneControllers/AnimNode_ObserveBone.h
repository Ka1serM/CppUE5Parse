#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"

#include "AnimNode_ObserveBone.generated.h"

namespace ue5parse::runtime {

// Records a bone's transform in a space without changing the pose (UE's BoneControllers/AnimNode_ObserveBone.h).
USTRUCT()
struct FAnimNode_ObserveBone : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference BoneToObserve;
    UPROPERTY()
    TEnumAsByte<EBoneControlSpace> DisplaySpace = BCS_ComponentSpace;
    UPROPERTY()
    bool bRelativeToRefPose = false;
    UPROPERTY()
    FVector Translation = FVector::ZeroVector;
    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY()
    FVector Scale = FVector(1.0);

protected:
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
};

}

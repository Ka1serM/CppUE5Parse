#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"
#include "Engine/Source/Runtime/Engine/AnimationRuntime.h"

#include "AnimNode_ObserveBone.generated.h"

namespace ue5parse::runtime {

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

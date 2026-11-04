#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"
#include "Engine/Source/Runtime/Engine/AnimationRuntime.h"

#include "AnimNode_CopyBone.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_CopyBone : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference SourceBone;
    UPROPERTY()
    FBoneReference TargetBone;
    UPROPERTY()
    bool bCopyTranslation = false;
    UPROPERTY()
    bool bCopyRotation = false;
    UPROPERTY()
    bool bCopyScale = false;
    UPROPERTY()
    TEnumAsByte<EBoneControlSpace> ControlSpace = BCS_ComponentSpace;

protected:
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
};

}

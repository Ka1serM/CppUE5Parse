#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"

#include "AnimNode_CopyBone.generated.h"

namespace ue5parse::runtime {

// Copies the translation, rotation or scale of a bone to another bone in a control space (UE's
// BoneControllers/AnimNode_CopyBone.h).
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

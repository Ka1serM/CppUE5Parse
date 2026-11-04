#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_BoneDrivenController.generated.h"

namespace ue5parse::runtime {

class UCurveFloat;

enum class EDrivenBoneModificationMode : uint8 { AddToInput, ReplaceComponent, AddToRefPose };

enum class EDrivenDestinationMode : uint8 { Bone, MorphTarget, MaterialParameter };

USTRUCT()
struct FAnimNode_BoneDrivenController : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference SourceBone;
    UPROPERTY()
    TObjectPtr<UCurveFloat> DrivingCurve = nullptr;
    UPROPERTY()
    float Multiplier = 1.0f;
    UPROPERTY()
    double RangeMin = -1.0;
    UPROPERTY()
    double RangeMax = 1.0;
    UPROPERTY()
    double RemappedMin = 0.0;
    UPROPERTY()
    double RemappedMax = 1.0;
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    FBoneReference TargetBone;
    UPROPERTY()
    EDrivenDestinationMode DestinationMode = EDrivenDestinationMode::Bone;
    UPROPERTY()
    EDrivenBoneModificationMode ModificationMode = EDrivenBoneModificationMode::AddToInput;
    UPROPERTY()
    TEnumAsByte<EComponentType::Type> SourceComponent = EComponentType::None;
    UPROPERTY()
    bool bUseRange = false;
    UPROPERTY()
    bool bAffectTargetTranslationX = false;
    UPROPERTY()
    bool bAffectTargetTranslationY = false;
    UPROPERTY()
    bool bAffectTargetTranslationZ = false;
    UPROPERTY()
    bool bAffectTargetRotationX = false;
    UPROPERTY()
    bool bAffectTargetRotationY = false;
    UPROPERTY()
    bool bAffectTargetRotationZ = false;
    UPROPERTY()
    bool bAffectTargetScaleX = false;
    UPROPERTY()
    bool bAffectTargetScaleY = false;
    UPROPERTY()
    bool bAffectTargetScaleZ = false;

protected:
    void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context) override;
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

private:
    double ExtractSourceValue(FCSPose<FCompactPose>& Pose) const;
};

}

#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "ue5parse/runtime/AnimationCore/CommonAnimTypes.h"
#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/Engine/Animation/BoneSocketReference.h"

#include "AnimNode_LookAt.generated.h"

namespace ue5parse::runtime {

namespace EInterpolationBlend {
enum Type : int { Linear, Cubic, Sinusoidal, EaseInOutExponent2, EaseInOutExponent3, EaseInOutExponent4, EaseInOutExponent5, MAX };
}

// Turns a bone so that an axis of it points at a location or at a bone or socket, optionally moving toward a new
// target over time (UE's BoneControllers/AnimNode_LookAt.h).
USTRUCT()
struct FAnimNode_LookAt : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference BoneToModify;
    UPROPERTY()
    FBoneSocketTarget LookAtTarget;
    UPROPERTY()
    FVector LookAtLocation = FVector(100.0, 0.0, 0.0);
    UPROPERTY()
    FAxis LookAt_Axis = {FVector(0.0, 1.0, 0.0), true};
    UPROPERTY()
    bool bUseLookUpAxis = false;
    UPROPERTY()
    TEnumAsByte<EInterpolationBlend::Type> InterpolationType = EInterpolationBlend::Linear;
    UPROPERTY()
    FAxis LookUp_Axis = {FVector(1.0, 0.0, 0.0), true};
    UPROPERTY()
    float LookAtClamp = 0.f;
    UPROPERTY()
    float InterpolationTime = 0.f;
    UPROPERTY()
    float InterpolationTriggerThreashold = 0.f;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;

protected:
    void UpdateInternal(const FAnimationUpdateContext& Context) override;
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

private:
    FVector CurrentLookAtLocation = FVector::ZeroVector;
    FVector CurrentTargetLocation = FVector::ZeroVector;
    FVector PreviousTargetLocation = FVector::ZeroVector;
    float AccumulatedInterpoolationTime = 0.f;
};

}

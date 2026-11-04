#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_SpringBone.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_SpringBone : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference SpringBone;
    UPROPERTY()
    double MaxDisplacement = 0.0;
    UPROPERTY()
    double SpringStiffness = 50.0;
    UPROPERTY()
    double SpringDamping = 4.0;
    UPROPERTY()
    double ErrorResetThresh = 256.0;
    UPROPERTY()
    bool bLimitDisplacement = false;
    UPROPERTY()
    bool bTranslateX = true;
    UPROPERTY()
    bool bTranslateY = true;
    UPROPERTY()
    bool bTranslateZ = true;
    UPROPERTY()
    bool bRotateX = false;
    UPROPERTY()
    bool bRotateY = false;
    UPROPERTY()
    bool bRotateZ = false;

    FVector BoneLocation = FVector::ZeroVector;
    FVector BoneVelocity = FVector::ZeroVector;
    FVector OwnerVelocity = FVector::ZeroVector;
    FVector LocalBoneTransform = FVector::ZeroVector;
    float RemainingTime = 0.f;
    float FixedTimeStep = 0.f;
    float TimeDilation = 1.f;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void UpdateInternal(const FAnimationUpdateContext& Context) override;
    bool HasPreUpdate() const override { return true; }
    void PreUpdate(const UAnimInstance* InAnimInstance) override;
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;

private:
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
};

}

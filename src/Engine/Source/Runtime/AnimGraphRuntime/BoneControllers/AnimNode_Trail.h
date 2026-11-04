#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_AnimDynamics.h"
#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/Core/Math/Axis.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"

#include "AnimNode_Trail.generated.h"

namespace ue5parse::runtime {

struct FPerJointTrailSetup {
    float TrailRelaxationSpeedPerSecond = 0.f;
};

USTRUCT()
struct FRotationLimit {
    GENERATED_BODY()
public:
    UPROPERTY()
    FVector LimitMin = FVector(-180, -180, -180);
    UPROPERTY()
    FVector LimitMax = FVector(+180, +180, +180);
};

USTRUCT()
struct FAnimNode_Trail : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    FAnimNode_Trail();

    FTransform OldBaseTransform;
    UPROPERTY()
    FBoneReference TrailBone;
    UPROPERTY()
    int32 ChainLength = 2;
    UPROPERTY()
    TEnumAsByte<EAxis::Type> ChainBoneAxis = EAxis::X;
    UPROPERTY()
    bool bInvertChainBoneAxis = false;
    UPROPERTY()
    bool bLimitStretch = false;
    UPROPERTY()
    bool bLimitRotation = false;
    UPROPERTY()
    bool bUsePlanarLimit = false;
    UPROPERTY()
    bool bActorSpaceFakeVel = false;
    UPROPERTY()
    bool bReorientParentToChild = true;
    bool bHadValidStrength = false;
    UPROPERTY()
    float MaxDeltaTime = 0.f;
    UPROPERTY()
    float RelaxationSpeedScale = 1.f;
    UPROPERTY()
    FRuntimeFloatCurve TrailRelaxationSpeed;
    UPROPERTY()
    FInputScaleBiasClamp RelaxationSpeedScaleInputProcessor;
    UPROPERTY()
    TArray<FRotationLimit> RotationLimits;
    UPROPERTY()
    TArray<FVector> RotationOffsets;
    UPROPERTY()
    TArray<FAnimPhysPlanarLimit> PlanarLimits;
    UPROPERTY()
    float StretchLimit = 0;
    UPROPERTY()
    FVector FakeVelocity = FVector::ZeroVector;
    UPROPERTY()
    FBoneReference BaseJoint;
    UPROPERTY()
    float LastBoneRotationAnimAlphaBlend = 0.f;

    float ThisTimstep = 0.f;
    TArray<FVector> TrailBoneLocations;
    TArray<FPerJointTrailSetup> PerJointTrailData;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void UpdateInternal(const FAnimationUpdateContext& Context) override;
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;

private:
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
    static FVector GetAlignVector(EAxis::Type AxisOption, bool bInvert);

    TArray<int32> ChainBoneIndices;
};

}

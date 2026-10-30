#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_AnimDynamics.h"
#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/Core/Math/Axis.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"
#include "ue5parse/runtime/Engine/Curves/CurveFloat.h"

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

// A chain that trails behind the movement of its base, relaxing toward the animated pose (UE's
// BoneControllers/AnimNode_Trail.h).
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
    // How the last bone's rotation is set: 0 copies the previous joint's, 1 keeps the animated one.
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

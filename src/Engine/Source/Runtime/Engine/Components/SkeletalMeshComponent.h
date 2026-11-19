#pragma once

#include <memory>

#include "Engine/Source/Runtime/ClothingSystemRuntimeInterface/ClothingSimulationFactory.h"
#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimCurveTypes.h"
#include "Engine/Source/Runtime/Engine/Components/SkinnedMeshComponent.h"
#include "Engine/Source/Runtime/Engine/ReferenceSkeleton.h"
#include "Engine/Source/Runtime/Engine/SingleAnimationPlayData.h"

#include "SkeletalMeshComponent.generated.h"

namespace ue5parse::runtime {

class UAnimInstance;
class UAnimSingleNodeInstance;
class UAnimationAsset;
class UAnimBlueprintGeneratedClass;
class UAnimSequence;
class USkeletalMesh;
struct FCompactPose;
class FParallelAnimationEvaluationTask;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAnimInitialized);

struct FAnimationEvaluationContext {
    const USkeletalMesh* SkeletalMesh = nullptr;
    UAnimInstance* AnimInstance = nullptr;
    UAnimInstance* PostProcessAnimInstance = nullptr;
    TArray<FTransform> ComponentSpaceTransforms;
    TArray<FTransform> BoneSpaceTransforms;
    FBlendedCurve Curve;
    bool bDoEvaluation = false;

    void Clear() {
        SkeletalMesh = nullptr;
        AnimInstance = nullptr;
        PostProcessAnimInstance = nullptr;
    }
};

namespace EAnimationMode {

UENUM()
enum Type : int32 { AnimationBlueprint, AnimationSingleNode, AnimationCustomMode };

}

namespace EKinematicBonesUpdateToPhysics {

UENUM()
enum Type : int32 { SkipSimulatingBones, SkipAllBones };

}

namespace EPhysicsTransformUpdateMode {

UENUM()
enum Type : int32 { SimulationUpatesComponentTransform, ComponentTransformIsKinematic };

}

UCLASS()
class USkeletalMeshComponent : public USkinnedMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UAnimBlueprintGeneratedClass> AnimBlueprintGeneratedClass = nullptr;
    UPROPERTY()
    TSubclassOf<UAnimInstance> AnimClass;
    UPROPERTY(transient)
    TObjectPtr<UAnimInstance> AnimScriptInstance = nullptr;
    UPROPERTY(transient)
    TObjectPtr<UAnimInstance> PostProcessAnimInstance = nullptr;
    UPROPERTY()
    FSingleAnimationPlayData AnimationData;
    UPROPERTY()
    FVector LineCheckBoundsScale = FVector(1.0f, 1.0f, 1.0f);
    UPROPERTY()
    float GlobalAnimRateScale = 1.0f;
    UPROPERTY()
    TEnumAsByte<EKinematicBonesUpdateToPhysics::Type> KinematicBonesUpdateType = EKinematicBonesUpdateToPhysics::SkipSimulatingBones;
    UPROPERTY()
    TEnumAsByte<EPhysicsTransformUpdateMode::Type> PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::SimulationUpatesComponentTransform;
    UPROPERTY()
    TEnumAsByte<EAnimationMode::Type> AnimationMode = EAnimationMode::AnimationBlueprint;
    UPROPERTY()
    uint8 bDisablePostProcessBlueprint : 1 = false;
    UPROPERTY()
    uint8 bUpdateOverlapsOnAnimationFinalize : 1 = true;
    UPROPERTY()
    uint8 bEnablePhysicsOnDedicatedServer : 1 = true;
    UPROPERTY()
    uint8 bUpdateMeshWhenKinematic : 1 = false;
    UPROPERTY()
    uint8 bUpdateJointsFromAnimation : 1 = false;
    UPROPERTY()
    uint8 bAllowClothActors : 1 = true;
    UPROPERTY()
    uint8 bDisableClothSimulation : 1 = false;
    UPROPERTY()
    uint8 bDisableRigidBodyAnimNode : 1 = false;
    UPROPERTY()
    uint8 bDisableAnimDynamicsAnimNode : 1 = false;
    UPROPERTY()
    uint8 bAllowAnimCurveEvaluation : 1 = true;
    UPROPERTY()
    uint8 bDisableAnimCurves_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bDeferMovementFromSceneQueries : 1 = false;
    UPROPERTY()
    uint8 bCollideWithEnvironment : 1 = false;
    UPROPERTY()
    uint8 bCollideWithAttachedChildren : 1 = false;
    UPROPERTY()
    uint8 bLocalSpaceSimulation : 1 = false;
    UPROPERTY()
    uint8 bForceCollisionUpdate : 1 = false;
    UPROPERTY()
    float ClothVelocityScale = 1.0f;
    UPROPERTY()
    uint8 bResetAfterTeleport : 1 = true;
    UPROPERTY()
    uint8 bDeferKinematicBoneUpdate : 1 = false;
    UPROPERTY()
    uint8 bNoSkeletonUpdate : 1 = false;
    UPROPERTY()
    uint8 bPauseAnims : 1 = false;
    UPROPERTY()
    uint8 bEnableAnimation : 1 = true;
    UPROPERTY()
    uint8 bUseRefPoseOnInitAnim : 1 = false;
    UPROPERTY()
    uint8 bEnablePerPolyCollision : 1 = false;
    UPROPERTY()
    uint8 bForceRefpose : 1 = false;
    UPROPERTY()
    uint8 bOldForceRefPose : 1 = false;
    UPROPERTY()
    uint8 bShowPrePhysBones : 1 = false;
    UPROPERTY()
    uint8 bEnableLineCheckWithBounds : 1 = false;
    UPROPERTY()
    uint8 bPropagateCurvesToFollowers : 1 = false;
    UPROPERTY()
    uint8 bSkipKinematicUpdateWhenInterpolating : 1 = false;
    UPROPERTY()
    uint8 bSkipBoundsUpdateWhenInterpolating : 1 = false;
    UPROPERTY()
    uint8 bSuppressNotifyEventDispatch : 1 = false;
    UPROPERTY()
    EDynamicActorScene UseAsyncScene = EDynamicActorScene::Default;
    UPROPERTY()
    float ClothBlendWeight = 1.0f;
    UPROPERTY()
    bool bWaitForParallelClothTask = false;
    UPROPERTY()
    float ClothMaxDistanceScale = 1.0f;
    UPROPERTY()
    float ClothGeometryScale = 1.0f;
    UPROPERTY()
    int32 PostProcessAnimBPLODThreshold = INDEX_NONE;
    UPROPERTY()
    FConstraintBrokenSignature OnConstraintBroken;
    UPROPERTY()
    FPlasticDeformationEventSignature OnPlasticDeformation;
    UPROPERTY()
    TSubclassOf<UClothingSimulationFactory> ClothingSimulationFactory;
    UPROPERTY()
    float TeleportDistanceThreshold = 300.0f;
    UPROPERTY()
    float TeleportRotationThreshold = 0.0f;
    UPROPERTY()
    FOnAnimInitialized OnAnimInitialized;
    UPROPERTY()
    TObjectPtr<UAnimSequence> SequenceToPlay_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UAnimationAsset> AnimToPlay_DEPRECATED = nullptr;
    UPROPERTY()
    uint8 bDefaultLooping_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bDefaultPlaying_DEPRECATED : 1 = true;
    UPROPERTY()
    float DefaultPosition_DEPRECATED = 0.0f;
    UPROPERTY()
    float DefaultPlayRate_DEPRECATED = 1.0f;
    UPROPERTY()
    uint8 bClothMorphTarget : 1 = false;
    UPROPERTY()
    int32 RagdollAggregateThreshold = 4;

    TArray<FBoneIndexType> RequiredBones;

    explicit USkeletalMeshComponent(const FObjectInitializer& ObjectInitializer);
    ~USkeletalMeshComponent() override;
    void OnRegister() override;
    void OnUnregister() override;
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable)
    USkeletalMesh* GetSkeletalMeshAsset() const;
    UFUNCTION(BlueprintCallable)
    void SetSkeletalMeshAsset(USkeletalMesh* NewMesh) { SetSkeletalMesh(NewMesh, false); }
    UFUNCTION(BlueprintCallable)
    void SetSkeletalMesh(USkeletalMesh* NewMesh, bool bReinitPose = true);
    void SetSkinnedAssetAndUpdate(USkinnedAsset* InSkinnedAsset, bool bReinitPose = true) override;
    UFUNCTION(BlueprintCallable)
    UAnimInstance* GetAnimInstance() const { return AnimScriptInstance; }
    UFUNCTION(BlueprintCallable)
    UAnimInstance* GetPostProcessInstance() const { return PostProcessAnimInstance; }
    UAnimSingleNodeInstance* GetSingleNodeInstance() const;
    UFUNCTION(BlueprintCallable)
    void SetAnimInstanceClass(UClass* NewClass);
    UFUNCTION(BlueprintCallable)
    void SetAnimClass(UClass* NewClass) { SetAnimInstanceClass(NewClass); }
    UFUNCTION(BlueprintCallable)
    UClass* GetAnimClass();
    UFUNCTION(BlueprintCallable)
    void SetAnimationMode(EAnimationMode::Type InAnimationMode, bool bForceInitAnimScriptInstance = true);
    UFUNCTION(BlueprintCallable)
    EAnimationMode::Type GetAnimationMode() const { return AnimationMode; }
    UFUNCTION(BlueprintCallable)
    void PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping);
    UFUNCTION(BlueprintCallable)
    void SetAnimation(UAnimationAsset* NewAnimToPlay);
    UFUNCTION(BlueprintCallable)
    void Play(bool bLooping);
    UFUNCTION(BlueprintCallable)
    void Stop();
    UFUNCTION(BlueprintCallable)
    bool IsPlaying() const;
    UFUNCTION(BlueprintCallable)
    void SetPosition(float InPos, bool bFireNotifies = true);
    UFUNCTION(BlueprintCallable)
    float GetPosition() const;
    UFUNCTION(BlueprintCallable)
    void SetPlayRate(float Rate);
    UFUNCTION(BlueprintCallable)
    float GetPlayRate() const;
    UFUNCTION(BlueprintCallable)
    void OverrideAnimationData(UAnimationAsset* InAnimToPlay, bool bIsLooping = true, bool bIsPlaying = true, float Position = 0.f, float PlayRate = 1.f);
    UFUNCTION(BlueprintCallable)
    void SetDisablePostProcessBlueprint(bool bInDisablePostProcess);
    UFUNCTION(BlueprintCallable)
    bool GetDisablePostProcessBlueprint() const { return bDisablePostProcessBlueprint; }
    int32 GetPredictedLODLevel() const { return 0; }
    template <class Function>
    void ForEachAnimInstance(Function&& InFunction) {
        if (AnimScriptInstance != nullptr)
            InFunction(AnimScriptInstance);
        for (UAnimInstance* LinkedInstance : LinkedInstances)
            InFunction(LinkedInstance);
        if (PostProcessAnimInstance != nullptr)
            InFunction(PostProcessAnimInstance);
    }
    TArray<UAnimInstance*>& GetLinkedAnimInstances() { return LinkedInstances; }
    const TArray<UAnimInstance*>& GetLinkedAnimInstances() const { return LinkedInstances; }

    void InitAnim(bool bForceReinit);
    bool InitializeAnimScriptInstance(bool bForceReinit, bool bInDeferRootNodeInitialization = false);
    void ClearAnimScriptInstance();
    void TickAnimation(float DeltaTime);
    void RefreshBoneTransforms(FActorComponentTickFunction* TickFunction = nullptr);
    bool IsRunningParallelEvaluation() const { return bRunningParallelEvaluation; }
    bool HandleExistingParallelEvaluationTask(bool bBlockOnTask, bool bPerformPostAnimEvaluation);
    void ConditionallyDispatchQueuedAnimEvents();
    const FBlendedCurve& GetAnimationCurves() const { return AnimCurves; }
    void ApplyAnimationCurvesToComponent(const TMap<FName, float>& InMaterialParameterCurves);

    const TArray<FTransform>& GetBoneSpaceTransforms();
    const TArray<FTransform>& GetComponentSpaceTransforms() const { return ComponentSpaceTransformsArray[CurrentReadComponentTransforms]; }

private:
    void RecalcRequiredBones();
    bool NeedToSpawnAnimScriptInstance() const;
    bool NeedToSpawnPostPhysicsInstance(bool bForceReinit) const;
    void ResetLinkedAnimInstances();
    bool ShouldUpdatePostProcessInstance() const;
    bool ShouldEvaluatePostProcessInstance() const;
    void EvaluatePostProcessMeshInstance(FCompactPose& InOutPose, FBlendedCurve& InOutCurve) const;
    void PerformAnimationProcessing(const USkeletalMesh* InSkeletalMesh, UAnimInstance* InAnimInstance, bool bInDoEvaluation, TArray<FTransform>& OutSpaceBases,
                                    TArray<FTransform>& OutBoneSpaceTransforms, FBlendedCurve& OutCurve) const;
    void ParallelAnimationEvaluation();
    void SwapEvaluationContextBuffers();
    void DispatchParallelEvaluationTasks(FActorComponentTickFunction* TickFunction);
    void DoParallelEvaluationTasks_OnGameThread();
    void CompleteParallelAnimationEvaluation(bool bDoPostAnimEvaluation);
    void PostAnimEvaluation(FAnimationEvaluationContext& EvaluationContext);
    void DoInstancePreEvaluation();
    void DoInstancePostEvaluation();
    void FinalizeAnimationUpdate();
    void FinalizeBoneTransform();
    void FlipEditableSpaceBases();
    TArray<FTransform>& GetEditableComponentSpaceTransforms() { return ComponentSpaceTransformsArray[CurrentEditableComponentTransforms]; }

    TArray<UAnimInstance*> LinkedInstances;
    FBlendedCurve AnimCurves;
    bool bNeedsQueuedAnimEventsDispatched = false;
    TArray<FTransform> BoneSpaceTransforms;
    TArray<FTransform> ComponentSpaceTransformsArray[2];
    int32 CurrentEditableComponentTransforms = 0;
    int32 CurrentReadComponentTransforms = 1;
    bool bNeedToFlipSpaceBaseBuffers = false;
    bool bHasValidBoneTransform = false;
    FAnimationEvaluationContext AnimEvaluationContext;
    std::unique_ptr<FParallelAnimationEvaluationTask> ParallelAnimationEvaluationTask;
    bool bRunningParallelEvaluation = false;
};

}

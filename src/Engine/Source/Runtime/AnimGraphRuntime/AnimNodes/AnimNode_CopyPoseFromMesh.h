#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_CopyPoseFromMesh.generated.h"

namespace ue5parse::runtime {

class USkeletalMesh;
class USkeletalMeshComponent;

USTRUCT()
struct FAnimNode_CopyPoseFromMesh : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    TWeakObjectPtr<USkeletalMeshComponent> SourceMeshComponent;
    UPROPERTY()
    bool bUseAttachedParent = false;
    UPROPERTY()
    bool bCopyCurves = false;
    UPROPERTY()
    bool bCopyCustomAttributes = false;
    UPROPERTY()
    bool bUseMeshPose = false;
    UPROPERTY()
    FName RootBoneToCopy;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;
    bool HasPreUpdate() const override { return true; }
    void PreUpdate(const UAnimInstance* InAnimInstance) override;
    const USkeletalMeshComponent* GetCurrentlyUsedSourceMeshComponent() const { return CurrentlyUsedSourceMeshComponent; }

private:
    void ReinitializeMeshComponent(USkeletalMeshComponent* NewSourceMeshComponent, USkeletalMeshComponent* TargetMeshComponent);
    void RefreshMeshComponent(USkeletalMeshComponent* TargetMeshComponent);
    void ResetMeshComponent(USkeletalMeshComponent* InMeshComponent, USkeletalMeshComponent* InTargetMeshComponent);

    USkeletalMeshComponent* CurrentlyUsedSourceMeshComponent = nullptr;
    const USkeletalMesh* CurrentlyUsedSourceMesh = nullptr;
    const USkeletalMesh* CurrentlyUsedMesh = nullptr;
    const USkeletalMesh* CurrentlyUsedTargetMesh = nullptr;
    TArray<int32> BoneMapToSource;
    TArray<FTransform> SourceMeshTransformArray;
    FBlendedCurve SourceCurves;
};

}

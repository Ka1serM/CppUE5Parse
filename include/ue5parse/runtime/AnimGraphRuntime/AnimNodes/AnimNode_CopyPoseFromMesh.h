#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_CopyPoseFromMesh.generated.h"

namespace ue5parse::runtime {

class USkeletalMesh;
class USkeletalMeshComponent;

// Copies the pose of another skeletal mesh component, matched by bone name (UE's
// AnimNodes/AnimNode_CopyPoseFromMesh.h). The source's component space transforms are read on the game thread
// before the update, so the pose is the source's last evaluated one.
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

private:
    void ReinitializeMeshComponent(USkeletalMeshComponent* NewSourceMeshComponent, USkeletalMeshComponent* TargetMeshComponent);
    void RefreshMeshComponent(USkeletalMeshComponent* TargetMeshComponent);
    void ResetMeshComponent(USkeletalMeshComponent* InMeshComponent, USkeletalMeshComponent* InTargetMeshComponent);

    USkeletalMeshComponent* CurrentlyUsedSourceMeshComponent = nullptr;
    const USkeletalMesh* CurrentlyUsedSourceMesh = nullptr;
    const USkeletalMesh* CurrentlyUsedMesh = nullptr;
    const USkeletalMesh* CurrentlyUsedTargetMesh = nullptr;
    // The source mesh bone of each target mesh bone; INDEX_NONE for a bone that is not copied.
    TArray<int32> BoneMapToSource;
    TArray<FTransform> SourceMeshTransformArray;
    FBlendedCurve SourceCurves;
};

}

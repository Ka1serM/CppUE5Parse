#pragma once

#include "ue5parse/runtime/CoreUObject/Templates/SubclassOf.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_CustomProperty.h"

#include "AnimNode_LinkedAnimGraph.generated.h"

namespace ue5parse::runtime {

class UAnimBlueprintGeneratedClass;
class UAnimInstance;
class UBlendProfile;
struct FAnimBlueprintFunction;

// Runs the AnimGraph of another anim instance of the same mesh, with this node's inputs as its input poses (UE's
// Animation/AnimNode_LinkedAnimGraph.h). The instance is made when this instance initializes, as a linked instance of
// the component.
USTRUCT()
struct FAnimNode_LinkedAnimGraph : public FAnimNode_CustomProperty {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FPoseLink> InputPoses;
    UPROPERTY()
    TArray<FName> InputPoseNames;
    UPROPERTY()
    TSubclassOf<UAnimInstance> InstanceClass;
    UPROPERTY()
    bool bReceiveNotifiesFromLinkedInstances = false;
    UPROPERTY()
    bool bPropagateNotifiesToLinkedInstances = false;

    // The root of the linked graph in the target instance.
    FAnimNode_Base* LinkedRoot = nullptr;
    int32 NodeIndex = INDEX_NONE;
    int32 CachedLinkedNodeIndex = INDEX_NONE;

    // Changes the class and remakes the instance.
    void SetAnimClass(TSubclassOf<UAnimInstance> InClass, const UAnimInstance* InOwningAnimInstance);
    virtual FName GetDynamicLinkFunctionName() const;
    virtual UAnimInstance* GetDynamicLinkTarget(UAnimInstance* InOwningAnimInstance) const;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

    void InitializeSubGraph_AnyThread(const FAnimationInitializeContext& Context);
    void CacheBonesSubGraph_AnyThread(const FAnimationCacheBonesContext& Context);

protected:
    // The pending inertial blends of a class change (they need an inertialization node above this one).
    float PendingBlendOutDuration = -1.0f;
    UPROPERTY()
    TObjectPtr<const UBlendProfile> PendingBlendOutProfile = nullptr;
    float PendingBlendInDuration = -1.0f;
    UPROPERTY()
    TObjectPtr<const UBlendProfile> PendingBlendInProfile = nullptr;

    void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
    bool NeedsOnInitializeAnimInstance() const override { return true; }
    void ReinitializeLinkedAnimInstance(const UAnimInstance* InOwningAnimInstance, UAnimInstance* InNewAnimInstance = nullptr);
    void TeardownInstance(const UAnimInstance* InOwningAnimInstance);
    virtual bool CanTeardownLinkedInstance(const UAnimInstance* LinkedInstance) const { return true; }
    UClass* GetTargetClass() const override { return InstanceClass.Get(); }
    void DynamicLink(UAnimInstance* InOwningAnimInstance);
    void DynamicUnlink(UAnimInstance* InOwningAnimInstance);
    static int32 FindFunctionInputIndex(const FAnimBlueprintFunction& AnimBlueprintFunction, const FName& InInputName);
    void RequestBlend(const UAnimBlueprintGeneratedClass* PriorAnimBPClass, const UAnimBlueprintGeneratedClass* NewAnimBPClass);
};

}

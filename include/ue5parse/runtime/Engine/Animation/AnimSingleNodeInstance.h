#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/Animation/BlendSpace.h"
#include "ue5parse/runtime/Engine/Animation/Skeleton.h"

#include "AnimSingleNodeInstance.generated.h"

namespace ue5parse::runtime {

class FAnimSingleNodeInstanceProxy;

// The one node of a single node instance: it ticks and poses the instance's asset (UE's FAnimNode_SingleNode in
// Animation/AnimSingleNodeInstanceProxy.h).
struct FAnimNode_SingleNode : public FAnimNode_Base {
    FName ActiveMontageSlot = FAnimSlotGroup::DefaultSlotName;

    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

private:
    friend class FAnimSingleNodeInstanceProxy;
    FAnimSingleNodeInstanceProxy* Proxy = nullptr;
};

// The proxy of a single node instance: the asset, its time, rate and play state (UE's FAnimSingleNodeInstanceProxy).
// A cooked game has no editor preview, so an additive asset plays onto the reference pose.
class FAnimSingleNodeInstanceProxy : public FAnimInstanceProxy {
public:
    FAnimSingleNodeInstanceProxy() { SingleNode.Proxy = this; }

    void Initialize(UAnimInstance* InAnimInstance) override;
    bool Evaluate(FPoseContext& Output) override;
    void PostUpdate(UAnimInstance* InAnimInstance) override;
    void InitializeObjects(UAnimInstance* InAnimInstance) override;

    void SetPlaying(bool bIsPlaying) { bPlaying = bIsPlaying; }
    bool IsPlaying() const { return bPlaying; }
    bool IsReverse() const { return bReverse; }
    void SetLooping(bool bIsLooping) { bLooping = bIsLooping; }
    bool IsLooping() const { return bLooping; }
    void ResetWeightInfo() { WeightInfo = FSlotNodeWeightInfo(); }
    void SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping, float InPlayRate);
    void UpdateBlendspaceSamples(const FVector& InBlendInput);
    void SetCurrentTime(float InCurrentTime) { CurrentTime = InCurrentTime; }
    float GetCurrentTime() const { return CurrentTime; }
    float GetPlayRate() const { return PlayRate; }
    void SetPlayRate(float InPlayRate) { PlayRate = InPlayRate; }
    FVector GetFilterLastOutput() const { return BlendFilter.GetFilterLastOutput(); }
    void SetReverse(bool bInReverse);
    void SetBlendSpacePosition(const FVector& InPosition) { BlendSpacePosition = InPosition; }
    void GetBlendSpaceState(FVector& OutPosition, FVector& OutFilteredPosition) const;
    float GetBlendSpaceLength() const;
    void UpdateMontageWeightForSlot(const FName CurrentSlotNodeName, float InGlobalNodeWeight);
    void SetMontagePreviewSlot(FName PreviewSlot) { SingleNode.ActiveMontageSlot = PreviewSlot; }

protected:
    void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;

private:
    friend struct FAnimNode_SingleNode;

    void InternalBlendSpaceEvaluatePose(const UBlendSpace* BlendSpace, FPoseContext& OutContext);
    // The last montage of the frame that is active (UE's GetActiveMontageEvaluationState).
    const FMontageEvaluationState* GetActiveMontageEvaluationState() const;

    UAnimationAsset* CurrentAsset = nullptr;
    FAnimNode_SingleNode SingleNode;
    FVector BlendSpacePosition = FVector(0, 0, 0);
    TArray<FBlendSampleData> BlendSampleData;
    FBlendFilter BlendFilter;
    int32 TriangulationIndex = INDEX_NONE;
    FSlotNodeWeightInfo WeightInfo;
    float CurrentTime = 0;
    FDeltaTimeRecord DeltaTimeRecord;
    float PlayRate = 1;
    bool bLooping = true;
    bool bPlaying = true;
    bool bReverse = false;
};

// An anim instance that plays one asset, without a graph: the skeletal mesh component's single node mode (UE's
// Animation/AnimSingleNodeInstance.h). A montage plays in its first slot.
UCLASS()
class UAnimSingleNodeInstance : public UAnimInstance {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UAnimationAsset> CurrentAsset = nullptr;

    explicit UAnimSingleNodeInstance(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable)
    void SetLooping(bool bIsLooping);
    UFUNCTION(BlueprintCallable)
    void SetPlayRate(float InPlayRate);
    UFUNCTION(BlueprintCallable)
    void SetReverse(bool bInReverse);
    UFUNCTION(BlueprintCallable)
    void SetPosition(float InPosition, bool bFireNotifies = true);
    UFUNCTION(BlueprintCallable)
    void SetPositionWithPreviousTime(float InPosition, float InPreviousTime, bool bFireNotifies = true);
    UFUNCTION(BlueprintCallable)
    void SetBlendSpacePosition(const FVector& InPosition);
    UFUNCTION(BlueprintCallable)
    void SetPlaying(bool bIsPlaying);
    UFUNCTION(BlueprintCallable)
    float GetLength();
    UFUNCTION(BlueprintCallable)
    void PlayAnim(bool bIsLooping = false, float InPlayRate = 1.f, float InStartPosition = 0.f);
    UFUNCTION(BlueprintCallable)
    void StopAnim();
    UFUNCTION(BlueprintCallable)
    void SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping = true, float InPlayRate = 1.f);
    UFUNCTION(BlueprintCallable)
    UAnimationAsset* GetAnimationAsset() const { return CurrentAsset; }

    bool IsPlaying() const;
    bool IsReverse() const;
    bool IsLooping() const;
    float GetPlayRate() const;
    float GetCurrentTime() const;
    UAnimationAsset* GetCurrentAsset() const { return CurrentAsset; }
    void GetBlendSpaceState(FVector& OutPosition, FVector& OutFilteredPosition) const;
    FVector GetFilterLastOutput();
    void UpdateBlendspaceSamples(FVector InBlendInput);
    // Plays the montage again from the section; its sections loop when the instance loops.
    void RestartMontage(UAnimMontage* Montage, FName FromSection = FName());
    void SetMontageLoop(UAnimMontage* Montage, bool bIsLooping, FName StartingSection = FName());
    void SetMontagePreviewSlot(FName PreviewSlot);
    void UpdateMontageWeightForTimeSkip(float TimeDifference);

    void NativeInitializeAnimation() override;
    void OnMontageInstanceStopped(FAnimMontageInstance& StoppedMontageInstance) override;
    void Montage_Advance(float DeltaTime) override;

private:
    FAnimSingleNodeInstanceProxy& GetSingleNodeProxy() const { return static_cast<FAnimSingleNodeInstanceProxy&>(const_cast<UAnimSingleNodeInstance*>(this)->GetProxyOnGameThread()); }
};

}

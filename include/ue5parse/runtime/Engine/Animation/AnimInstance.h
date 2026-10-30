#pragma once

#include <memory>

#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimMontage.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifyQueue.h"

#include "AnimInstance.generated.h"

namespace ue5parse::runtime {

class FAnimInstanceProxy;
class AActor;
struct FAnimNode_LinkedInputPose;
class USkeletalMesh;
class USkeletalMeshComponent;
class USkeleton;
struct FBranchingPointNotifyPayload;
struct FPoseContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllMontageInstancesEndedMCDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMontageStartedMCDelegate, UAnimMontage*, Montage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMontageEndedMCDelegate, UAnimMontage*, Montage, bool, bInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMontageBlendingOutStartedMCDelegate, UAnimMontage*, Montage, bool, bInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayMontageAnimNotifyDelegate, FName, NotifyName,
                                             const FBranchingPointNotifyPayload&, BranchingPointPayload);

// FMontageActiveSlotTracker: the weights of a slot node in one frame.
struct FMontageActiveSlotTracker {
    // The montages' weight in the slot.
    float MontageLocalWeight = 0;
    float NodeGlobalWeight = 0;
    bool bIsRelevantThisTick = false;
    bool bWasRelevantOnPreviousTick = false;
};

// The montage events that the advance queues until the frame's notifies are dispatched.
struct FQueuedMontageBlendingOutEvent {
    UAnimMontage* Montage = nullptr;
    bool bInterrupted = false;
    FOnMontageBlendingOutStarted Delegate;
};

struct FQueuedMontageEndedEvent {
    UAnimMontage* Montage = nullptr;
    int32 MontageInstanceID = INDEX_NONE;
    bool bInterrupted = false;
    FOnMontageEnded Delegate;
};

namespace ERootMotionMode {
enum Type : int32 { NoRootMotionExtraction, IgnoreRootMotion, RootMotionFromEverything, RootMotionFromMontagesOnly };
}

// UE's animation instance (Animation/AnimInstance.h): the object that a skeletal mesh component's anim class makes. A
// cooked animation Blueprint derives from this class and holds its graph's nodes in its properties; the proxy runs
// them. The outer of an instance is its component.
UCLASS()
class UAnimInstance : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USkeleton> CurrentSkeleton = nullptr;
    UPROPERTY()
    TEnumAsByte<ERootMotionMode::Type> RootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
    UPROPERTY()
    bool bQueueMontageEvents = false;
    // The graph updates with the evaluation on a worker thread, unless something needs it at once.
    UPROPERTY()
    bool bUseMultiThreadedAnimationUpdate = true;
    // A linked instance that a linked graph node made, and whether it sends its notifies to the main instance or
    // takes the main instance's.
    bool bCreatedByLinkedAnimGraph = false;
    bool bPropagateNotifiesToLinkedInstances = false;
    bool bReceiveNotifiesFromLinkedInstances = false;
    // (UAnimMontage* Montage, bool bInterrupted)
    UPROPERTY()
    FMulticastScriptDelegate OnMontageBlendingOut;
    // (UAnimMontage* Montage)
    UPROPERTY()
    FMulticastScriptDelegate OnMontageStarted;
    // (UAnimMontage* Montage, bool bInterrupted)
    UPROPERTY()
    FMulticastScriptDelegate OnMontageEnded;
    UPROPERTY()
    FMulticastScriptDelegate OnAllMontageInstancesEnded;
    // The PlayMontageNotify branching points of the montages (FName NotifyName, const FBranchingPointNotifyPayload&).
    UPROPERTY()
    FMulticastScriptDelegate OnPlayMontageNotifyBegin;
    UPROPERTY()
    FMulticastScriptDelegate OnPlayMontageNotifyEnd;

    // The montages that play or blend out, oldest first.
    TArray<std::unique_ptr<FAnimMontageInstance>> MontageInstances;
    // The notifies of this frame: the montages add theirs on the game thread, the graph's players after the update.
    FAnimNotifyQueue NotifyQueue;
    // The notify states that are active, and the references they began with.
    UPROPERTY(transient)
    TArray<FAnimNotifyEvent> ActiveAnimNotifyState;
    UPROPERTY(transient)
    TArray<FAnimNotifyEventReference> ActiveAnimNotifyEventReference;

    explicit UAnimInstance(const FObjectInitializer& ObjectInitializer);
    ~UAnimInstance() override;

    USkeletalMeshComponent* GetSkelMeshComponent() const;
    // The proxy after the component's evaluation task completes, which the game thread needs.
    FAnimInstanceProxy& GetProxyOnGameThread();
    const FAnimInstanceProxy& GetProxyOnGameThread() const;
    // The proxy as it is, for the code that the evaluation task runs.
    FAnimInstanceProxy& GetProxyOnAnyThread() { return *AnimInstanceProxy; }
    const FAnimInstanceProxy& GetProxyOnAnyThread() const { return *AnimInstanceProxy; }
    // UE's ShouldExtractRootMotion: the players lock the root of root motion animations in these modes.
    bool ShouldExtractRootMotion() const {
        return RootMotionMode == ERootMotionMode::RootMotionFromEverything || RootMotionMode == ERootMotionMode::IgnoreRootMotion;
    }
    // The input pose node of the main graph (UE's GetLinkedInputPoseNode with no names).
    FAnimNode_LinkedInputPose* GetLinkedInputPoseNode() const;
    // Whether the proxy's bones are those of the mesh, so the graph can evaluate on it.
    bool ParallelCanEvaluate(const USkeletalMesh* InSkeletalMesh) const;

    // Links the graph to the component's mesh and runs the initialize events, then initializes the graph.
    void InitializeAnimation(bool bInDeferRootNodeInitialization = false);
    // Stops the montages and ends the active notify states.
    void UninitializeAnimation();
    virtual void NativeUninitializeAnimation() {}
    enum class EUpdateAnimationFlag : uint8 { Default, ForceParallelUpdate };
    // The game thread's part of a frame: the event graph between the property copies around it. The graph update
    // follows here when NeedsImmediateUpdate, otherwise in the component's evaluation task.
    void UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion = false, EUpdateAnimationFlag UpdateFlag = EUpdateAnimationFlag::Default);
    // The graph update, which can run on a worker thread.
    void ParallelUpdateAnimation();
    // After the update, on the game thread. The main instance also post updates its linked instances.
    void PostUpdateAnimation();
    bool NeedsImmediateUpdate(float DeltaSeconds, bool bNeedsValidRootMotion) const;
    // Whether UpdateAnimation ran and its PostUpdateAnimation did not.
    bool NeedsUpdate() const { return bNeedsUpdate; }
    virtual bool CanRunParallelWork() const { return true; }
    void ParallelEvaluateAnimation(FPoseContext& Output);
    // Around the evaluation, on the game thread: after it the native and Blueprint post evaluate events run.
    void PreEvaluateAnimation() {}
    void PostEvaluateAnimation();
    virtual void NativePostEvaluateAnimation() {}
    // The curves of the final pose, which the component gives the instances after the evaluation (the post process
    // instance's output when there is one).
    void SetAnimationCurves(const FBlendedCurve& InCurves) { AnimationCurves.CopyFrom(InCurves); }
    void CopyCurveValues(const UAnimInstance& InSourceInstance) { AnimationCurves.CopyFrom(InSourceInstance.AnimationCurves); }
    // After the evaluation: the notifies, the queued montage events, then the deletion of the ended montage instances.
    void DispatchQueuedAnimEvents();
    // The frame's notifies: the states that end, then those that begin, then the ticks of the active states; a notify
    // fires its object or calls the AnimNotify_<Name> function.
    void TriggerAnimNotifies(float DeltaSeconds);
    void TriggerSingleAnimNotify(const FAnimNotifyEvent* AnimNotifyEvent);
    void TriggerSingleAnimNotify(FAnimNotifyEventReference& EventReference);
    // Ends the active notify states.
    void EndNotifyStates();
    // A native class can take a notify before its object or function fires: true when it did.
    virtual bool HandleNotify(const FAnimNotifyEvent& AnimNotifyEvent) { return false; }
    bool ShouldTriggerAnimNotifyState(const UAnimNotifyState* AnimNotifyState) const { return AnimNotifyState != nullptr; }

    // Montages (UE's Montage_* functions). A null montage means every active montage.
    UFUNCTION(BlueprintCallable)
    float Montage_Play(UAnimMontage* MontageToPlay, float InPlayRate = 1.0f, EMontagePlayReturnType ReturnValueType = EMontagePlayReturnType::MontageLength,
                       float InTimeToStartMontageAt = 0.0f, bool bStopAllMontages = true);
    UFUNCTION(BlueprintCallable)
    float Montage_PlayWithBlendIn(UAnimMontage* MontageToPlay, const FAlphaBlendArgs& BlendIn, float InPlayRate = 1.0f,
                                  EMontagePlayReturnType ReturnValueType = EMontagePlayReturnType::MontageLength, float InTimeToStartMontageAt = 0.0f,
                                  bool bStopAllMontages = true);
    UFUNCTION(BlueprintCallable)
    float Montage_PlayWithBlendSettings(UAnimMontage* MontageToPlay, const FMontageBlendSettings& BlendInSettings, float InPlayRate = 1.0f,
                                        EMontagePlayReturnType ReturnValueType = EMontagePlayReturnType::MontageLength,
                                        float InTimeToStartMontageAt = 0.0f, bool bStopAllMontages = true);
    UFUNCTION(BlueprintCallable)
    void Montage_Stop(float InBlendOutTime, const UAnimMontage* Montage = nullptr);
    // VALORANT's addition (13.08 0x1441c25d0): the graph's root node initializes again.
    UFUNCTION(BlueprintCallable)
    void ResetAnimGraph();
    UFUNCTION(BlueprintCallable)
    void Montage_StopWithBlendOut(const FAlphaBlendArgs& BlendOut, const UAnimMontage* Montage = nullptr);
    UFUNCTION(BlueprintCallable)
    void Montage_StopWithBlendSettings(const FMontageBlendSettings& BlendOutSettings, const UAnimMontage* Montage = nullptr);
    UFUNCTION(BlueprintCallable)
    void Montage_StopGroupByName(float InBlendOutTime, FName GroupName);
    UFUNCTION(BlueprintCallable)
    void Montage_Pause(const UAnimMontage* Montage = nullptr);
    UFUNCTION(BlueprintCallable)
    void Montage_Resume(const UAnimMontage* Montage);
    UFUNCTION(BlueprintCallable)
    void Montage_JumpToSection(FName SectionName, const UAnimMontage* Montage = nullptr);
    UFUNCTION(BlueprintCallable)
    void Montage_JumpToSectionsEnd(FName SectionName, const UAnimMontage* Montage = nullptr);
    UFUNCTION(BlueprintCallable)
    void Montage_SetNextSection(FName SectionNameToChange, FName NextSection, const UAnimMontage* Montage = nullptr);
    UFUNCTION(BlueprintCallable)
    void Montage_SetPlayRate(const UAnimMontage* Montage, float NewPlayRate = 1.0f);
    UFUNCTION(BlueprintCallable)
    void Montage_SetPosition(const UAnimMontage* Montage, float NewPosition);
    // The native delegates of one montage instance (UE's Montage_SetEndDelegate and Montage_SetBlendingOutDelegate).
    void Montage_SetEndDelegate(FOnMontageEnded& InOnMontageEnded, UAnimMontage* Montage);
    void Montage_SetBlendingOutDelegate(FOnMontageBlendingOutStarted& InOnMontageBlendingOut, UAnimMontage* Montage);
    UFUNCTION(BlueprintPure)
    bool Montage_IsActive(const UAnimMontage* Montage) const;
    UFUNCTION(BlueprintPure)
    bool Montage_IsPlaying(const UAnimMontage* Montage) const;
    UFUNCTION(BlueprintPure)
    FName Montage_GetCurrentSection(const UAnimMontage* Montage = nullptr) const;
    UFUNCTION(BlueprintPure)
    float Montage_GetPosition(const UAnimMontage* Montage) const;
    UFUNCTION(BlueprintPure)
    bool Montage_GetIsStopped(const UAnimMontage* Montage) const;
    UFUNCTION(BlueprintPure)
    float Montage_GetBlendTime(const UAnimMontage* Montage) const;
    UFUNCTION(BlueprintPure)
    float Montage_GetPlayRate(const UAnimMontage* Montage) const;
    UFUNCTION(BlueprintPure)
    float Montage_GetEffectivePlayRate(const UAnimMontage* Montage) const;
    UFUNCTION(BlueprintPure)
    bool IsAnyMontagePlaying() const { return MontageInstances.Num() > 0; }
    UFUNCTION(BlueprintPure)
    UAnimMontage* GetCurrentActiveMontage() const;
    void StopAllMontages(float BlendOut);
    void StopAllMontagesByGroupName(FName InGroupName, const FMontageBlendSettings& BlendOutSettings);
    // The newest instance that is not blending out.
    FAnimMontageInstance* GetActiveMontageInstance() const;
    FAnimMontageInstance* GetActiveInstanceForMontage(const UAnimMontage* Montage) const;
    FAnimMontageInstance* GetRootMotionMontageInstance() const { return RootMotionMontageInstance; }

    // A transient montage of one animation in one slot (UE's PlaySlotAnimationAsDynamicMontage).
    UFUNCTION(BlueprintCallable)
    UAnimMontage* PlaySlotAnimationAsDynamicMontage(UAnimSequenceBase* Asset, FName SlotNodeName, float BlendInTime = 0.25f,
                                                    float BlendOutTime = 0.25f, float InPlayRate = 1.0f, int32 LoopCount = 1,
                                                    float BlendOutTriggerTime = -1.0f, float InTimeToStartMontageAt = 0.0f);
    UFUNCTION(BlueprintCallable)
    UAnimMontage* PlaySlotAnimationAsDynamicMontage_WithBlendArgs(UAnimSequenceBase* Asset, FName SlotNodeName, const FAlphaBlendArgs& BlendIn,
                                                                  const FAlphaBlendArgs& BlendOut, float InPlayRate = 1.0f, int32 LoopCount = 1,
                                                                  float BlendOutTriggerTime = -1.0f, float InTimeToStartMontageAt = 0.0f);
    UFUNCTION(BlueprintCallable)
    UAnimMontage* PlaySlotAnimationAsDynamicMontage_WithBlendSettings(UAnimSequenceBase* Asset, FName SlotNodeName,
                                                                      const FMontageBlendSettings& BlendInSettings,
                                                                      const FMontageBlendSettings& BlendOutSettings, float InPlayRate = 1.0f,
                                                                      int32 LoopCount = 1, float BlendOutTriggerTime = -1.0f,
                                                                      float InTimeToStartMontageAt = 0.0f);
    UFUNCTION(BlueprintCallable)
    void StopSlotAnimation(float InBlendOutTime = 0.25f, FName SlotNodeName = FName());
    UFUNCTION(BlueprintPure)
    bool IsPlayingSlotAnimation(const UAnimSequenceBase* Asset, FName SlotNodeName) const;
    UFUNCTION(BlueprintPure)
    bool IsSlotActive(FName SlotNodeName) const;
    UFUNCTION(BlueprintPure)
    float Blueprint_GetSlotMontageLocalWeight(FName SlotNodeName) const;
    float GetSlotNodeGlobalWeight(const FName& SlotNodeName) const;
    float GetSlotMontageGlobalWeight(const FName& SlotNodeName) const;

    // Called by the montage instances.
    virtual void OnMontageInstanceStopped(FAnimMontageInstance& StoppedMontageInstance) { ClearMontageInstanceReferences(StoppedMontageInstance); }
    // The montages' advance of a frame, with their branching points and events (UE's Montage_Advance).
    virtual void Montage_Advance(float DeltaSeconds);
    // The montages' blend weights of a frame (UE's Montage_UpdateWeight).
    void Montage_UpdateWeight(float DeltaSeconds);
    void ClearMontageInstanceReferences(FAnimMontageInstance& InMontageInstance);
    void QueueMontageBlendingOutEvent(const FQueuedMontageBlendingOutEvent& MontageBlendingOutEvent);
    void QueueMontageEndedEvent(const FQueuedMontageEndedEvent& MontageEndedEvent);

    // A curve of the last evaluated pose; 0 for a curve it does not have.
    UFUNCTION(BlueprintPure)
    UFUNCTION(BlueprintPure)
    float GetCurveValue(FName CurveName) const { return AnimationCurves.Get(CurveName); }
    UFUNCTION(BlueprintPure)
    bool GetCurveValueWithDefault(FName CurveName, float DefaultValue, float& OutValue) const;
    UFUNCTION(BlueprintPure)
    float GetInstanceAssetPlayerTimeFromEndFraction(int32 AssetPlayerIndex) const;
    UFUNCTION(BlueprintPure)
    AActor* GetOwningActor() const;
    UFUNCTION(BlueprintPure)
    USkeletalMeshComponent* GetOwningComponent() const;
    const FBlendedCurve& GetAnimationCurves() const { return AnimationCurves; }

    // The state machine queries of the anim graph's transition rules. A machine index counts from the end of the class's
    // node properties.
    UFUNCTION(BlueprintPure)
    float GetRelevantAnimTimeRemaining(int32 MachineIndex, int32 StateIndex);
    UFUNCTION(BlueprintPure)
    float GetRelevantAnimTimeRemainingFraction(int32 MachineIndex, int32 StateIndex);
    UFUNCTION(BlueprintPure)
    float GetRelevantAnimLength(int32 MachineIndex, int32 StateIndex);
    UFUNCTION(BlueprintPure)
    float GetRelevantAnimTime(int32 MachineIndex, int32 StateIndex);
    UFUNCTION(BlueprintPure)
    float GetRelevantAnimTimeFraction(int32 MachineIndex, int32 StateIndex);
    UFUNCTION(BlueprintPure)
    float GetInstanceMachineWeight(int32 MachineIndex);
    UFUNCTION(BlueprintPure)
    float GetInstanceStateWeight(int32 MachineIndex, int32 StateIndex);
    UFUNCTION(BlueprintPure)
    float GetInstanceCurrentStateElapsedTime(int32 MachineIndex);

    virtual void NativeInitializeAnimation() {}
    virtual void NativeUpdateAnimation(float DeltaSeconds) {}
    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) {}
    virtual void NativeBeginPlay() {}

    UFUNCTION(BlueprintImplementableEvent)
    void BlueprintInitializeAnimation();
    UFUNCTION(BlueprintImplementableEvent)
    void BlueprintUpdateAnimation(float DeltaTimeX);
    UFUNCTION(BlueprintImplementableEvent)
    void BlueprintThreadSafeUpdateAnimation(float DeltaTime);
    UFUNCTION(BlueprintImplementableEvent)
    void BlueprintPostEvaluateAnimation();
    UFUNCTION(BlueprintImplementableEvent)
    void BlueprintBeginPlay();

private:
    void ProcessCopies(int32 CallSite);
    float Montage_PlayInternal(UAnimMontage* MontageToPlay, const FMontageBlendSettings& BlendInSettings, float InPlayRate,
                               EMontagePlayReturnType ReturnValueType, float InTimeToStartMontageAt, bool bStopAllMontages);
    // Montage_Stop and its variants: the blend out of each instance.
    template <class BlendSelector>
    void Montage_StopInternal(BlendSelector&& AlphaBlendSelectorFunction, const UAnimMontage* Montage);
    // Calls the function on the instance of the montage, or on every active instance for a null montage.
    template <class Function>
    void ForEachActiveInstance(const UAnimMontage* Montage, Function&& Visit);
    // The instance of the montage, or for a null montage the first active one.
    const FAnimMontageInstance* FindQueriedInstance(const UAnimMontage* Montage) const;
    // Before the graph update: the weights and positions of the montages, then their state for the graph.
    void UpdateMontage(float DeltaSeconds);

protected:
    // The montages' state for the graph's slots: those with weight, in play order.
    void UpdateMontageEvaluationData();
    // Replaces the proxy (a subclass with its own proxy type, as UE's CreateAnimInstanceProxy).
    void SetAnimInstanceProxy(std::unique_ptr<FAnimInstanceProxy> InProxy);

private:
    void TriggerMontageBlendingOutEvent(const FQueuedMontageBlendingOutEvent& MontageBlendingOutEvent);
    void TriggerMontageEndedEvent(const FQueuedMontageEndedEvent& MontageEndedEvent);
    void TriggerQueuedMontageEvents();

    TMap<const UAnimMontage*, FAnimMontageInstance*> ActiveMontagesMap;
    FAnimMontageInstance* RootMotionMontageInstance = nullptr;
    TArray<FQueuedMontageBlendingOutEvent> QueuedMontageBlendingOutEvents;
    TArray<FQueuedMontageEndedEvent> QueuedMontageEndedEvents;

    std::unique_ptr<FAnimInstanceProxy> AnimInstanceProxy;
    FBlendedCurve AnimationCurves;
    bool bNeedsUpdate = false;
};

}

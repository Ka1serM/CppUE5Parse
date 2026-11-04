#pragma once

#include <memory>
#include <vector>

#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimCurveTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimMontage.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNotifyQueue.h"

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

struct FMontageActiveSlotTracker {
    float MontageLocalWeight = 0;
    float NodeGlobalWeight = 0;
    bool bIsRelevantThisTick = false;
    bool bWasRelevantOnPreviousTick = false;
};

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
    UPROPERTY()
    bool bUseMultiThreadedAnimationUpdate = true;
    bool bCreatedByLinkedAnimGraph = false;
    bool bPropagateNotifiesToLinkedInstances = false;
    bool bReceiveNotifiesFromLinkedInstances = false;
    UPROPERTY()
    FMulticastScriptDelegate OnMontageBlendingOut;
    UPROPERTY()
    FMulticastScriptDelegate OnMontageStarted;
    UPROPERTY()
    FMulticastScriptDelegate OnMontageEnded;
    UPROPERTY()
    FMulticastScriptDelegate OnAllMontageInstancesEnded;
    UPROPERTY()
    FMulticastScriptDelegate OnPlayMontageNotifyBegin;
    UPROPERTY()
    FMulticastScriptDelegate OnPlayMontageNotifyEnd;

    TArray<std::unique_ptr<FAnimMontageInstance>> MontageInstances;
    FAnimNotifyQueue NotifyQueue;
    UPROPERTY(transient)
    TArray<FAnimNotifyEvent> ActiveAnimNotifyState;
    UPROPERTY(transient)
    TArray<FAnimNotifyEventReference> ActiveAnimNotifyEventReference;

    explicit UAnimInstance(const FObjectInitializer& ObjectInitializer);
    ~UAnimInstance() override;

    USkeletalMeshComponent* GetSkelMeshComponent() const;
    FAnimInstanceProxy& GetProxyOnGameThread();
    const FAnimInstanceProxy& GetProxyOnGameThread() const;
    FAnimInstanceProxy& GetProxyOnAnyThread() { return *AnimInstanceProxy; }
    const FAnimInstanceProxy& GetProxyOnAnyThread() const { return *AnimInstanceProxy; }
    bool ShouldExtractRootMotion() const {
        return RootMotionMode == ERootMotionMode::RootMotionFromEverything || RootMotionMode == ERootMotionMode::IgnoreRootMotion;
    }
    FAnimNode_LinkedInputPose* GetLinkedInputPoseNode() const;
    bool ParallelCanEvaluate(const USkeletalMesh* InSkeletalMesh) const;

    void InitializeAnimation(bool bInDeferRootNodeInitialization = false);
    void UninitializeAnimation();
    virtual void NativeUninitializeAnimation() {}
    enum class EUpdateAnimationFlag : uint8 { Default, ForceParallelUpdate };
    void UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion = false, EUpdateAnimationFlag UpdateFlag = EUpdateAnimationFlag::Default);
    void ParallelUpdateAnimation();
    void PostUpdateAnimation();
    bool NeedsImmediateUpdate(float DeltaSeconds, bool bNeedsValidRootMotion) const;
    bool NeedsUpdate() const { return bNeedsUpdate; }
    virtual bool CanRunParallelWork() const { return true; }
    void ParallelEvaluateAnimation(FPoseContext& Output);
    void PreEvaluateAnimation() {}
    void PostEvaluateAnimation();
    virtual void NativePostEvaluateAnimation() {}
    void UpdateCurvesToEvaluationContext(const FBlendedCurve& InCurves);
    void UpdateCurvesPostEvaluation();
    void CopyCurveValues(const UAnimInstance& InSourceInstance) { AnimationCurves.CopyFrom(InSourceInstance.AnimationCurves); }
    void DispatchQueuedAnimEvents();
    void TriggerAnimNotifies(float DeltaSeconds);
    void TriggerSingleAnimNotify(const FAnimNotifyEvent* AnimNotifyEvent);
    void TriggerSingleAnimNotify(FAnimNotifyEventReference& EventReference);
    void EndNotifyStates();
    virtual bool HandleNotify(const FAnimNotifyEvent& AnimNotifyEvent) { return false; }
    bool ShouldTriggerAnimNotifyState(const UAnimNotifyState* AnimNotifyState) const { return AnimNotifyState != nullptr; }

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
    FAnimMontageInstance* GetActiveMontageInstance() const;
    FAnimMontageInstance* GetActiveInstanceForMontage(const UAnimMontage* Montage) const;

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

    virtual void OnMontageInstanceStopped(FAnimMontageInstance& StoppedMontageInstance) { ClearMontageInstanceReferences(StoppedMontageInstance); }
    virtual void Montage_Advance(float DeltaSeconds);
    void Montage_UpdateWeight(float DeltaSeconds);
    void ClearMontageInstanceReferences(FAnimMontageInstance& InMontageInstance);
    void QueueMontageBlendingOutEvent(const FQueuedMontageBlendingOutEvent& MontageBlendingOutEvent);
    void QueueMontageEndedEvent(const FQueuedMontageEndedEvent& MontageEndedEvent);

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
    template <class BlendSelector>
    void Montage_StopInternal(BlendSelector&& AlphaBlendSelectorFunction, const UAnimMontage* Montage);
    template <class Function>
    void ForEachActiveInstance(const UAnimMontage* Montage, Function&& Visit);
    const FAnimMontageInstance* FindQueriedInstance(const UAnimMontage* Montage) const;
    void UpdateMontage(float DeltaSeconds);

protected:
    void UpdateMontageEvaluationData();
    void SetAnimInstanceProxy(std::unique_ptr<FAnimInstanceProxy> InProxy);

private:
    void TriggerMontageBlendingOutEvent(const FQueuedMontageBlendingOutEvent& MontageBlendingOutEvent);
    void TriggerMontageEndedEvent(const FQueuedMontageEndedEvent& MontageEndedEvent);
    void TriggerQueuedMontageEvents();

    TMap<const UAnimMontage*, FAnimMontageInstance*> ActiveMontagesMap;
    FAnimMontageInstance* RootMotionMontageInstance = nullptr;
    std::vector<FQueuedMontageBlendingOutEvent> QueuedMontageBlendingOutEvents;
    std::vector<FQueuedMontageEndedEvent> QueuedMontageEndedEvents;

    std::unique_ptr<FAnimInstanceProxy> AnimInstanceProxy;
    FBlendedCurve AnimationCurves;
    TMap<FName, float> MaterialCurves;
    TArray<FName> MaterialParametersToClear;
    bool bNeedsUpdate = false;
};

}

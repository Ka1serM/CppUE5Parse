#pragma once

#include <functional>

#include "Engine/Source/Runtime/Engine/AlphaBlend.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimCompositeBase.h"

#include "AnimMontage.generated.h"

namespace ue5parse::runtime {

class UAnimInstance;
class UAnimMontage;
class UBlendProfile;
struct FAnimMontageInstance;

USTRUCT()
struct FCompositeSection : public FAnimLinkableElement {
    GENERATED_BODY()
    UPROPERTY()
    FName SectionName;
    UPROPERTY()
    FName NextSectionName;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> MetaData;
};

USTRUCT()
struct FSlotAnimationTrack {
    GENERATED_BODY()
    UPROPERTY()
    FName SlotName;
    UPROPERTY()
    FAnimTrack AnimTrack;
};

namespace EMontageBlendMode {

enum Type : int32 { Standard, Inertialization };

}

namespace EAnimNotifyEventType {

enum Type : int32 { Begin, End };

}

USTRUCT()
struct FBranchingPointMarker {
    GENERATED_BODY()
    UPROPERTY()
    int32 NotifyIndex = INDEX_NONE;
    UPROPERTY()
    float TriggerTime = 0;
    UPROPERTY()
    TEnumAsByte<EAnimNotifyEventType::Type> NotifyEventType = EAnimNotifyEventType::Begin;
};

USTRUCT()
struct FTimeStretchCurveMarker {
    GENERATED_BODY()
    UPROPERTY()
    float Time[3] = {0, 0, 0};
    UPROPERTY()
    float Alpha = 0;
};

USTRUCT()
struct FTimeStretchCurve {
    GENERATED_BODY()
    UPROPERTY()
    float SamplingRate = 15;
    UPROPERTY()
    float CurveValueMinPrecision = 0.001f;
    UPROPERTY()
    TArray<FTimeStretchCurveMarker> Markers;
    UPROPERTY()
    float Sum_dT_i_by_C_i[3] = {0, 0, 0};

    bool IsValid() const { return Markers.Num() > 0; }
};

enum class EMontagePlayReturnType : uint8 { MontageLength, Duration };

USTRUCT()
struct FMontageBlendSettings {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UBlendProfile> BlendProfile = nullptr;
    UPROPERTY()
    FAlphaBlendArgs Blend;
    UPROPERTY()
    TEnumAsByte<EMontageBlendMode::Type> BlendMode = EMontageBlendMode::Standard;

    FMontageBlendSettings() = default;
    explicit FMontageBlendSettings(float BlendTime) : Blend(BlendTime) {}
    explicit FMontageBlendSettings(const FAlphaBlendArgs& BlendArgs) : Blend(BlendArgs) {}
};

using FOnMontageEnded = std::function<void(UAnimMontage*, bool)>;
using FOnMontageBlendingOutStarted = std::function<void(UAnimMontage*, bool)>;

enum class EMontageSubStepResult : uint8 { Moved, NotMoved, InvalidSection, InvalidMontage };

struct FMontageSubStepper {
    void Initialize(const FAnimMontageInstance& InAnimInstance);
    void AddEvaluationTime(float InDeltaTime) { TimeRemaining += InDeltaTime; }
    bool HasTimeRemaining() const { return TimeRemaining > UE_SMALL_NUMBER; }
    float GetRemainingTime() const { return TimeRemaining; }
    EMontageSubStepResult Advance(float& InOut_P_Original, const FBranchingPointMarker** OutBranchingPointMarkerPtr);
    float GetRemainingPlayTimeToSectionEnd(float In_P_Original) const;
    bool HasReachedEndOfSection() const { return bReachedEndOfSection; }
    bool GetbPlayingForward() const { return bPlayingForward; }
    float GetDeltaMove() const { return DeltaMove; }
    int32 GetCurrentSectionIndex() const { return CurrentSectionIndex; }

private:
    const FAnimMontageInstance* MontageInstance = nullptr;
    const UAnimMontage* Montage = nullptr;
    float TimeRemaining = 0;
    float PlayRate = 1;
    float DeltaMove = 0;
    bool bPlayingForward = true;
    bool bReachedEndOfSection = false;
    int32 CurrentSectionIndex = INDEX_NONE;
    float CurrentSectionStartTime = 0;
    float CurrentSectionLength = 0;
};

UCLASS()
class UAnimMontage : public UAnimCompositeBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<EMontageBlendMode::Type> BlendModeIn = EMontageBlendMode::Standard;
    UPROPERTY()
    TEnumAsByte<EMontageBlendMode::Type> BlendModeOut = EMontageBlendMode::Standard;
    UPROPERTY()
    FAlphaBlend BlendIn{0.25f};
    UPROPERTY()
    FAlphaBlend BlendOut{0.25f};
    UPROPERTY()
    float BlendOutTriggerTime = -1;
    UPROPERTY()
    FName SyncGroup;
    UPROPERTY()
    int32 SyncSlotIndex = 0;
    UPROPERTY()
    TArray<FCompositeSection> CompositeSections;
    UPROPERTY()
    TArray<FSlotAnimationTrack> SlotAnimTracks;
    UPROPERTY()
    bool bEnableRootMotionTranslation = false;
    UPROPERTY()
    bool bEnableRootMotionRotation = false;
    UPROPERTY()
    bool bEnableAutoBlendOut = true;
    UPROPERTY()
    TObjectPtr<UBlendProfile> BlendProfileIn = nullptr;
    UPROPERTY()
    TObjectPtr<UBlendProfile> BlendProfileOut = nullptr;
    UPROPERTY()
    TArray<FBranchingPointMarker> BranchingPointMarkers;
    UPROPERTY()
    TArray<int32> BranchingPointStateNotifyIndices;
    UPROPERTY()
    FTimeStretchCurve TimeStretchCurve;

    explicit UAnimMontage(const FObjectInitializer& ObjectInitializer);

    static UAnimMontage* CreateSlotAnimationAsDynamicMontage_WithBlendSettings(UAnimSequenceBase* Asset, FName SlotNodeName,
                                                                              const FMontageBlendSettings& BlendInSettings,
                                                                              const FMontageBlendSettings& BlendOutSettings, float InPlayRate,
                                                                              int32 LoopCount, float InBlendOutTriggerTime);

    int32 GetSectionIndex(FName InSectionName) const;
    bool IsValidSectionIndex(int32 SectionIndex) const { return CompositeSections.IsValidIndex(SectionIndex); }
    const FCompositeSection& GetAnimCompositeSection(int32 SectionIndex) const { return CompositeSections[SectionIndex]; }
    int32 GetAnimCompositeSectionIndexFromPos(float CurrentTime, float& PosWithinCompositeSection) const;
    float CalculatePos(const FCompositeSection& Section, float PosWithinCompositeSection) const { return Section.GetTime() + PosWithinCompositeSection; }
    void GetSectionStartAndEndTime(int32 SectionIndex, float& OutStartTime, float& OutEndTime) const;
    float GetSectionLength(int32 SectionIndex) const;
    bool IsValidSlot(FName InSlotName) const;
    bool IsValidAdditiveSlot(FName SlotNodeName) const;
    const FAnimTrack* GetAnimationData(FName InSlotName) const;
    bool HasValidSlotSetup() const;
    bool HasRootMotion() const override;
    const FBranchingPointMarker* FindFirstBranchingPointMarker(float StartTrackPos, float EndTrackPos) const;
    static void FilterOutNotifyBranchingPoints(TArray<struct FAnimNotifyEventReference>& InAnimNotifies);
    FName GetGroupName() const;

private:
    bool IsWithinPos(int32 FirstIndex, int32 SecondIndex, float CurrentTime) const;
};

struct FAnimMontageInstance {
    UAnimMontage* Montage = nullptr;
    FOnMontageEnded OnMontageEnded;
    FOnMontageBlendingOutStarted OnMontageBlendingOutStarted;
    bool bPlaying = false;
    float DefaultBlendTimeMultiplier = 1;
    TArray<int32> NextSections;
    TArray<int32> PrevSections;
    FDeltaTimeRecord DeltaTimeRecord;

    explicit FAnimMontageInstance(UAnimInstance* InAnimInstance) : AnimInstance(InAnimInstance) {}
    FAnimMontageInstance(const FAnimMontageInstance&) = delete;
    FAnimMontageInstance& operator=(const FAnimMontageInstance&) = delete;

    void Initialize(UAnimMontage* InMontage);
    void Play(float InPlayRate);
    void Play(float InPlayRate, const FMontageBlendSettings& BlendInSettings);
    void Stop(const FMontageBlendSettings& InBlendOutSettings, bool bInterrupt = true);
    void Stop(const FAlphaBlend& InBlendOut, bool bInterrupt = true);
    void Pause() { bPlaying = false; }
    void SetPlaying(bool bInPlaying) { bPlaying = bInPlaying; }
    void Terminate();
    bool JumpToSectionName(FName SectionName, bool bEndOfSection = false);
    bool SetNextSectionName(FName SectionName, FName NewNextSectionName);
    bool SetNextSectionID(int32 SectionID, int32 NewNextSectionID);
    FName GetCurrentSection() const;
    FName GetSectionNameFromID(int32 SectionID) const;

    void UpdateWeight(float DeltaTime);
    void Advance(float DeltaTime);

    bool IsValid() const { return Montage != nullptr; }
    bool IsPlaying() const { return IsValid() && bPlaying; }
    bool IsStopped() const { return Blend.GetDesiredValue() == 0.0f; }
    bool IsActive() const { return IsValid() && !IsStopped(); }
    int32 GetInstanceID() const { return InstanceID; }
    float GetPosition() const { return Position; }
    void SetPosition(float const& InPosition) { Position = InPosition; }
    float GetPlayRate() const { return PlayRate; }
    void SetPlayRate(float InPlayRate) { PlayRate = InPlayRate; }
    float GetWeight() const { return Blend.GetBlendedValue(); }
    float GetBlendTime() const { return Blend.GetBlendTime(); }
    const FAlphaBlend& GetBlend() const { return Blend; }
    float GetBlendStartAlpha() const { return BlendStartAlpha; }
    const UBlendProfile* GetActiveBlendProfile() const { return ActiveBlendProfile; }

private:
    friend struct FMontageSubStepper;

    void InitializeBlend(const FAlphaBlend& InAlphaBlend);
    void HandleEvents(float PreviousTrackPos, float CurrentTrackPos, const FBranchingPointMarker* BranchingPointMarker);
    void BranchingPointEventHandler(const FBranchingPointMarker* BranchingPointMarker);
    bool UpdateActiveStateBranchingPoints(float CurrentTrackPosition);
    bool ValidateInstanceAfterNotifyState(UAnimInstance* InAnimInstance) const;
    void RefreshNextPrevSections();
    void OnMontagePositionChanged(FName ToSectionName);

    UAnimInstance* AnimInstance;
    FMontageSubStepper MontageSubStepper;
    FAlphaBlend Blend;
    const UBlendProfile* ActiveBlendProfile = nullptr;
    float BlendStartAlpha = 0;
    float Position = 0;
    float PlayRate = 1;
    float PreviousWeight = 0;
    float NotifyWeight = 0;
    int32 InstanceID = INDEX_NONE;
    TArray<FAnimNotifyEvent> ActiveStateBranchingPoints;
    bool bInterrupted = false;
    bool bEnableAutoBlendOut = true;
    bool bDisableRootMotion = false;
};

}

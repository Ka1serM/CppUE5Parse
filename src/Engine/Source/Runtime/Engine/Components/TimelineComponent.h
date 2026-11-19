#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "TimelineComponent.generated.h"

namespace ue5parse::runtime {

class AActor;
class UCurveFloat;
class UCurveLinearColor;
class UCurveVector;
class UTimelineTemplate;

DECLARE_DYNAMIC_DELEGATE(FOnTimelineEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTimelineFloat, float, Output);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTimelineVector, FVector, Output);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTimelineLinearColor, FLinearColor, Output);

UENUM()
enum ETimelineLengthMode : int32 { TL_TimelineLength, TL_LastKeyFrame };

namespace ETimelineDirection {

UENUM()
enum Type : int32 { Forward, Backward };

}

USTRUCT()
struct FTimelineEventEntry {
    GENERATED_BODY()
    UPROPERTY()
    float Time = 0;
    UPROPERTY()
    FOnTimelineEvent EventFunc;
};

USTRUCT()
struct FTimelineVectorTrack {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UCurveVector> VectorCurve = nullptr;
    UPROPERTY()
    FOnTimelineVector InterpFunc;
    UPROPERTY()
    FName TrackName;
    UPROPERTY()
    FName VectorPropertyName;
};

USTRUCT()
struct FTimelineFloatTrack {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UCurveFloat> FloatCurve = nullptr;
    UPROPERTY()
    FOnTimelineFloat InterpFunc;
    UPROPERTY()
    FName TrackName;
    UPROPERTY()
    FName FloatPropertyName;
};

USTRUCT()
struct FTimelineLinearColorTrack {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UCurveLinearColor> LinearColorCurve = nullptr;
    UPROPERTY()
    FOnTimelineLinearColor InterpFunc;
    UPROPERTY()
    FName TrackName;
    UPROPERTY()
    FName LinearColorPropertyName;
};

USTRUCT()
struct FTimeline {
    GENERATED_BODY()
    UPROPERTY(Replicated)
    TEnumAsByte<ETimelineLengthMode> LengthMode = TL_TimelineLength;
    UPROPERTY()
    bool bLooping = false;
    UPROPERTY()
    bool bReversePlayback = false;
    UPROPERTY()
    bool bPlaying = false;
    UPROPERTY(Replicated)
    float Length = 5.0f;
    UPROPERTY()
    float PlayRate = 1.0f;
    UPROPERTY()
    float TickInterval = 0;
    UPROPERTY()
    float Position = 0;
    UPROPERTY(Replicated)
    TArray<FTimelineEventEntry> Events;
    UPROPERTY(Replicated)
    TArray<FTimelineVectorTrack> InterpVectors;
    UPROPERTY(Replicated)
    TArray<FTimelineFloatTrack> InterpFloats;
    UPROPERTY(Replicated)
    TArray<FTimelineLinearColorTrack> InterpLinearColors;
    UPROPERTY(Replicated)
    FOnTimelineEvent TimelinePostUpdateFunc;
    UPROPERTY(Replicated)
    FOnTimelineEvent TimelineFinishedFunc;
    UPROPERTY(Replicated)
    TWeakObjectPtr<UObject> PropertySetObject;
    UPROPERTY(Replicated)
    FName DirectionPropertyName;

    void Play() {
        bReversePlayback = false;
        bPlaying = true;
    }
    void PlayFromStart();
    void Reverse() {
        bReversePlayback = true;
        bPlaying = true;
    }
    void ReverseFromEnd();
    void Stop() { bPlaying = false; }
    void TickTimeline(float DeltaTime);
    void SetPlaybackPosition(float NewPosition, bool bFireEvents, bool bFireUpdate = true);
    void SetNewTime(float NewTime);
    float GetTimelineLength() const;
    float GetLastKeyframeTime() const;
    void SetTimelineLength(float NewLength);
};

UCLASS()
class UTimelineComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UTimelineComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(ReplicatedUsing = OnRep_Timeline)
    FTimeline TheTimeline;
    UPROPERTY()
    bool bIgnoreTimeDilation = false;

    static UTimelineComponent* CreateForActor(AActor* Actor, const UTimelineTemplate* TimelineTemplate);

    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    void Activate(bool bReset = false) override;
    void Deactivate() override;

    UFUNCTION(BlueprintCallable)
    void Play();
    UFUNCTION(BlueprintCallable)
    void PlayFromStart();
    UFUNCTION(BlueprintCallable)
    void Reverse();
    UFUNCTION(BlueprintCallable)
    void ReverseFromEnd();
    UFUNCTION(BlueprintCallable)
    void Stop() { TheTimeline.Stop(); }
    UFUNCTION(BlueprintCallable)
    bool IsPlaying() const { return TheTimeline.bPlaying; }
    UFUNCTION(BlueprintCallable)
    bool IsReversing() const { return TheTimeline.bPlaying && TheTimeline.bReversePlayback; }
    UFUNCTION(BlueprintCallable)
    void SetPlaybackPosition(float NewPosition, bool bFireEvents, bool bFireUpdate) { TheTimeline.SetPlaybackPosition(NewPosition, bFireEvents, bFireUpdate); }
    UFUNCTION(BlueprintCallable)
    float GetPlaybackPosition() const { return TheTimeline.Position; }
    UFUNCTION(BlueprintCallable)
    void SetLooping(bool bNewLooping) { TheTimeline.bLooping = bNewLooping; }
    UFUNCTION(BlueprintCallable)
    bool IsLooping() const { return TheTimeline.bLooping; }
    UFUNCTION(BlueprintCallable)
    void SetPlayRate(float NewRate) { TheTimeline.PlayRate = NewRate; }
    UFUNCTION(BlueprintCallable)
    float GetPlayRate() const { return TheTimeline.PlayRate; }
    UFUNCTION(BlueprintCallable)
    void SetNewTime(float NewTime) { TheTimeline.SetNewTime(NewTime); }
    UFUNCTION(BlueprintCallable)
    float GetTimelineLength() const { return TheTimeline.GetTimelineLength(); }
    UFUNCTION(BlueprintCallable)
    float GetScaledTimelineLength() const;
    UFUNCTION(BlueprintCallable)
    void SetTimelineLength(float NewLength) { TheTimeline.SetTimelineLength(NewLength); }
    UFUNCTION(BlueprintCallable)
    void SetTimelineLengthMode(TEnumAsByte<ETimelineLengthMode> NewLengthMode) { TheTimeline.LengthMode = NewLengthMode; }
    UFUNCTION(BlueprintCallable)
    void SetIgnoreTimeDilation(bool bNewIgnoreTimeDilation) { bIgnoreTimeDilation = bNewIgnoreTimeDilation; }
    UFUNCTION(BlueprintCallable)
    bool GetIgnoreTimeDilation() const { return bIgnoreTimeDilation; }
    UFUNCTION(BlueprintCallable)
    void SetTimelineTickInterval(float NewTickInterval) { TheTimeline.TickInterval = NewTickInterval; }
    UFUNCTION(BlueprintCallable)
    float GetTimelineTickInterval() const { return TheTimeline.TickInterval; }
    UFUNCTION(BlueprintCallable)
    void SetFloatCurve(UCurveFloat* NewFloatCurve, FName FloatTrackName);
    UFUNCTION(BlueprintCallable)
    void SetVectorCurve(UCurveVector* NewVectorCurve, FName VectorTrackName);
    UFUNCTION(BlueprintCallable)
    void SetLinearColorCurve(UCurveLinearColor* NewLinearColorCurve, FName LinearColorTrackName);
    UFUNCTION(BlueprintCallable)
    void SetTimelinePostUpdateFunc(FOnTimelineEvent NewTimelinePostUpdateFunc) { TheTimeline.TimelinePostUpdateFunc = NewTimelinePostUpdateFunc; }
    UFUNCTION(BlueprintCallable)
    void SetTimelineFinishedFunc(FOnTimelineEvent NewTimelineFinishedFunc) { TheTimeline.TimelineFinishedFunc = NewTimelineFinishedFunc; }
    UFUNCTION(BlueprintCallable)
    void AddEvent(float Time, FOnTimelineEvent EventFunc);
    UFUNCTION(BlueprintCallable)
    void AddInterpFloat(UCurveFloat* FloatCurve, FOnTimelineFloat InterpFunc, FName PropertyName, FName TrackName);
    UFUNCTION(BlueprintCallable)
    void AddInterpVector(UCurveVector* VectorCurve, FOnTimelineVector InterpFunc, FName PropertyName, FName TrackName);
    UFUNCTION(BlueprintCallable)
    void AddInterpLinearColor(UCurveLinearColor* LinearColorCurve, FOnTimelineLinearColor InterpFunc, FName PropertyName, FName TrackName);
};

}

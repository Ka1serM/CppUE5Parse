#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "ue5parse/runtime/Engine/Components/ActorComponent.h"

#include "TimelineComponent.generated.h"

namespace ue5parse::runtime {

class AActor;
class UCurveFloat;
class UCurveLinearColor;
class UCurveVector;
class UTimelineTemplate;

enum ETimelineLengthMode : int32 { TL_TimelineLength, TL_LastKeyFrame };

namespace ETimelineDirection {
enum Type : int32 { Forward, Backward };
}

USTRUCT()
struct FTimelineEventEntry {
    GENERATED_BODY()
    UPROPERTY()
    float Time = 0;
    UPROPERTY()
    FScriptDelegate EventFunc;
};

USTRUCT()
struct FTimelineVectorTrack {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UCurveVector> VectorCurve = nullptr;
    UPROPERTY()
    FScriptDelegate InterpFunc;
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
    FScriptDelegate InterpFunc;
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
    FScriptDelegate InterpFunc;
    UPROPERTY()
    FName TrackName;
    UPROPERTY()
    FName LinearColorPropertyName;
};

// FTimeline: a playback position over tracks of curves; moving it calls the tracks' functions, sets the properties they
// name on the property set object, and fires the events passed (UE's Components/TimelineComponent.h). VALORANT adds
// TickInterval.
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
    FScriptDelegate TimelinePostUpdateFunc;
    UPROPERTY(Replicated)
    FScriptDelegate TimelineFinishedFunc;
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

// UTimelineComponent: a component that ticks an FTimeline (UE's Components/TimelineComponent.h).
UCLASS()
class UTimelineComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UTimelineComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(ReplicatedUsing = OnRep_Timeline)
    FTimeline TheTimeline;
    UPROPERTY()
    bool bIgnoreTimeDilation = false;

    // The timeline component of a Blueprint class's timeline (UE's UBlueprintGeneratedClass::CreateTimelineComponent).
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
    void SetPlaybackPosition(float NewPosition, bool bFireEvents, bool bFireUpdate) {
        TheTimeline.SetPlaybackPosition(NewPosition, bFireEvents, bFireUpdate);
    }
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
    void SetTimelinePostUpdateFunc(FScriptDelegate NewTimelinePostUpdateFunc) { TheTimeline.TimelinePostUpdateFunc = NewTimelinePostUpdateFunc; }
    UFUNCTION(BlueprintCallable)
    void SetTimelineFinishedFunc(FScriptDelegate NewTimelineFinishedFunc) { TheTimeline.TimelineFinishedFunc = NewTimelineFinishedFunc; }
    UFUNCTION(BlueprintCallable)
    void AddEvent(float Time, FScriptDelegate EventFunc);
    UFUNCTION(BlueprintCallable)
    void AddInterpFloat(UCurveFloat* FloatCurve, FScriptDelegate InterpFunc, FName PropertyName, FName TrackName);
    UFUNCTION(BlueprintCallable)
    void AddInterpVector(UCurveVector* VectorCurve, FScriptDelegate InterpFunc, FName PropertyName, FName TrackName);
    UFUNCTION(BlueprintCallable)
    void AddInterpLinearColor(UCurveLinearColor* LinearColorCurve, FScriptDelegate InterpFunc, FName PropertyName, FName TrackName);
};

}

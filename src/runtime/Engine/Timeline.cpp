#include "ue5parse/provider/FileProvider.h"
#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Engine/Components/TimelineComponent.h"
#include "ue5parse/runtime/Engine/Curves/CurveFloat.h"
#include "ue5parse/runtime/Engine/Curves/CurveLinearColor.h"
#include "ue5parse/runtime/Engine/Curves/CurveVector.h"
#include "ue5parse/runtime/Engine/Engine/TimelineTemplate.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

// UE's Timeline.cpp and UBlueprintGeneratedClass::CreateTimelineComponent (UE 5.3).

namespace ue5parse::runtime {

namespace {

template <class T>
void ExecuteIfBound(const FScriptDelegate& Delegate, T Value) {
    if (Delegate.IsBound()) Delegate.ProcessDelegate(&Value);
}

void ExecuteIfBound(const FScriptDelegate& Delegate) {
    if (Delegate.IsBound()) Delegate.ProcessDelegate(nullptr);
}

}

void FTimeline::PlayFromStart() {
    SetPlaybackPosition(0.0f, false);
    Play();
}

void FTimeline::ReverseFromEnd() {
    SetPlaybackPosition(GetTimelineLength(), false);
    Reverse();
}

void FTimeline::SetPlaybackPosition(float NewPosition, bool bFireEvents, bool bFireUpdate) {
    const float OldPosition = Position;
    Position = NewPosition;
    UObject* PropSetObject = PropertySetObject.Get();
    for (FTimelineVectorTrack& Track : InterpVectors) {
        if (Track.VectorCurve == nullptr || (!Track.InterpFunc.IsBound() && Track.VectorPropertyName.isNone())) continue;
        const FVector Value = Track.VectorCurve->GetVectorValue(Position);
        ExecuteIfBound(Track.InterpFunc, Value);
        if (PropSetObject == nullptr) continue;
        if (FStructProperty* Property = FindFProperty<FStructProperty>(PropSetObject->GetClass(), Track.VectorPropertyName))
            *Property->ContainerPtrToValuePtr<FVector>(PropSetObject) = Value;
    }
    for (FTimelineFloatTrack& Track : InterpFloats) {
        if (Track.FloatCurve == nullptr || (!Track.InterpFunc.IsBound() && Track.FloatPropertyName.isNone())) continue;
        const float Value = Track.FloatCurve->GetFloatValue(Position);
        ExecuteIfBound(Track.InterpFunc, Value);
        if (PropSetObject == nullptr) continue;
        if (FFloatProperty* Property = FindFProperty<FFloatProperty>(PropSetObject->GetClass(), Track.FloatPropertyName))
            Property->SetPropertyValue_InContainer(PropSetObject, Value);
    }
    for (FTimelineLinearColorTrack& Track : InterpLinearColors) {
        if (Track.LinearColorCurve == nullptr || (!Track.InterpFunc.IsBound() && Track.LinearColorPropertyName.isNone())) continue;
        const FLinearColor Value = Track.LinearColorCurve->GetLinearColorValue(Position);
        ExecuteIfBound(Track.InterpFunc, Value);
        if (PropSetObject == nullptr) continue;
        if (FStructProperty* Property = FindFProperty<FStructProperty>(PropSetObject->GetClass(), Track.LinearColorPropertyName))
            *Property->ContainerPtrToValuePtr<FLinearColor>(PropSetObject) = Value;
    }
    if (!DirectionPropertyName.isNone() && PropSetObject != nullptr) {
        const int64 Direction = bReversePlayback ? ETimelineDirection::Backward : ETimelineDirection::Forward;
        if (FByteProperty* Byte = FindFProperty<FByteProperty>(PropSetObject->GetClass(), DirectionPropertyName)) {
            Byte->SetPropertyValue_InContainer(PropSetObject, static_cast<uint8>(Direction));
        } else if (FEnumProperty* Enum = FindFProperty<FEnumProperty>(PropSetObject->GetClass(), DirectionPropertyName)) {
            Enum->GetUnderlyingProperty()->SetIntPropertyValue(Enum->ContainerPtrToValuePtr<void>(PropSetObject), Direction);
        }
    }
    if (bFireEvents) {
        float MinTime = bReversePlayback ? Position : OldPosition;
        float MaxTime = bReversePlayback ? OldPosition : Position;
        // An event at the very end (or start, reversed) fires.
        if (!bReversePlayback && MaxTime == GetTimelineLength()) MaxTime += UE_KINDA_SMALL_NUMBER;
        if (bReversePlayback && MinTime == 0.0f) MinTime -= UE_KINDA_SMALL_NUMBER;
        // A copy: an event may add events.
        const TArray<FTimelineEventEntry> EventsCopy = Events;
        for (const FTimelineEventEntry& Event : EventsCopy) {
            const bool bFire = bReversePlayback ? (Event.Time > MinTime && Event.Time <= MaxTime)
                                                : (Event.Time >= MinTime && Event.Time < MaxTime);
            if (bFire) ExecuteIfBound(Event.EventFunc);
        }
    }
    if (bFireUpdate) ExecuteIfBound(TimelinePostUpdateFunc);
}

void FTimeline::TickTimeline(float DeltaTime) {
    bool bIsFinished = false;
    if (bPlaying) {
        const float TimelineLength = GetTimelineLength();
        const float EffectiveDeltaTime = DeltaTime * (bReversePlayback ? -PlayRate : PlayRate);
        float NewPosition = Position + EffectiveDeltaTime;
        if (EffectiveDeltaTime > 0.0f) {
            if (NewPosition > TimelineLength) {
                if (bLooping) {
                    SetPlaybackPosition(TimelineLength, true, false);
                    SetPlaybackPosition(0.0f, false, false);
                    if (TimelineLength > 0.0f) {
                        while (NewPosition > TimelineLength) NewPosition -= TimelineLength;
                    } else {
                        NewPosition = 0.0f;
                    }
                } else {
                    NewPosition = TimelineLength;
                    Stop();
                    bIsFinished = true;
                }
            }
        } else if (NewPosition < 0.0f) {
            if (bLooping) {
                SetPlaybackPosition(0.0f, true, false);
                SetPlaybackPosition(TimelineLength, false, false);
                if (TimelineLength > 0.0f) {
                    while (NewPosition < 0.0f) NewPosition += TimelineLength;
                } else {
                    NewPosition = 0.0f;
                }
            } else {
                NewPosition = 0.0f;
                Stop();
                bIsFinished = true;
            }
        }
        SetPlaybackPosition(NewPosition, true);
    }
    if (bIsFinished) ExecuteIfBound(TimelineFinishedFunc);
}

void FTimeline::SetNewTime(float NewTime) { SetPlaybackPosition(FMath::Clamp(NewTime, 0.0f, Length), false); }

float FTimeline::GetTimelineLength() const {
    switch (LengthMode.GetValue()) {
    case TL_TimelineLength: return Length;
    case TL_LastKeyFrame: return GetLastKeyframeTime();
    }
    return 0.0f;
}

float FTimeline::GetLastKeyframeTime() const {
    float MaxTime = 0.0f;
    for (const FTimelineEventEntry& Event : Events) MaxTime = FMath::Max(Event.Time, MaxTime);
    float MinValue = 0, MaxValue = 0;
    for (const FTimelineVectorTrack& Track : InterpVectors) {
        Track.VectorCurve->GetTimeRange(MinValue, MaxValue);
        MaxTime = FMath::Max(MaxValue, MaxTime);
    }
    for (const FTimelineFloatTrack& Track : InterpFloats) {
        Track.FloatCurve->GetTimeRange(MinValue, MaxValue);
        MaxTime = FMath::Max(MaxValue, MaxTime);
    }
    for (const FTimelineLinearColorTrack& Track : InterpLinearColors) {
        Track.LinearColorCurve->GetTimeRange(MinValue, MaxValue);
        MaxTime = FMath::Max(MaxValue, MaxTime);
    }
    return MaxTime;
}

void FTimeline::SetTimelineLength(float NewLength) {
    Length = NewLength;
    if (Position > NewLength) SetNewTime(NewLength - UE_KINDA_SMALL_NUMBER);
}

UTimelineComponent::UTimelineComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTimelineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bIgnoreTimeDilation) {
        if (UWorld* World = GetWorld()) DeltaTime = World->GetDeltaSeconds();
    }
    TheTimeline.TickTimeline(DeltaTime);
    if (!IsPlaying()) Deactivate();
}

// VALORANT's UTimelineComponent::Activate also gives the tick function the timeline's TickInterval.
void UTimelineComponent::Activate(bool bReset) {
    if (GetRuntimeProvider().getGame() == EGame::GAME_Valorant) PrimaryComponentTick.TickInterval = TheTimeline.TickInterval;
    Super::Activate(bReset);
    PrimaryComponentTick.SetTickFunctionEnable(true);
}

void UTimelineComponent::Deactivate() {
    Super::Deactivate();
    PrimaryComponentTick.SetTickFunctionEnable(false);
}

void UTimelineComponent::Play() {
    Activate();
    TheTimeline.Play();
}

void UTimelineComponent::PlayFromStart() {
    Activate();
    TheTimeline.PlayFromStart();
}

void UTimelineComponent::Reverse() {
    Activate();
    TheTimeline.Reverse();
}

void UTimelineComponent::ReverseFromEnd() {
    Activate();
    TheTimeline.ReverseFromEnd();
}

float UTimelineComponent::GetScaledTimelineLength() const {
    return TheTimeline.PlayRate != 0.0f ? TheTimeline.GetTimelineLength() / TheTimeline.PlayRate : 0.0f;
}

void UTimelineComponent::SetFloatCurve(UCurveFloat* NewFloatCurve, FName FloatTrackName) {
    for (FTimelineFloatTrack& Track : TheTimeline.InterpFloats)
        if (Track.TrackName == FloatTrackName) Track.FloatCurve = NewFloatCurve;
}

void UTimelineComponent::SetVectorCurve(UCurveVector* NewVectorCurve, FName VectorTrackName) {
    for (FTimelineVectorTrack& Track : TheTimeline.InterpVectors)
        if (Track.TrackName == VectorTrackName) Track.VectorCurve = NewVectorCurve;
}

void UTimelineComponent::SetLinearColorCurve(UCurveLinearColor* NewLinearColorCurve, FName LinearColorTrackName) {
    for (FTimelineLinearColorTrack& Track : TheTimeline.InterpLinearColors)
        if (Track.TrackName == LinearColorTrackName) Track.LinearColorCurve = NewLinearColorCurve;
}

void UTimelineComponent::AddEvent(float Time, FScriptDelegate EventFunc) {
    FTimelineEventEntry Entry;
    Entry.Time = Time;
    Entry.EventFunc = EventFunc;
    TheTimeline.Events.Add(Entry);
}

void UTimelineComponent::AddInterpFloat(UCurveFloat* FloatCurve, FScriptDelegate InterpFunc, FName PropertyName, FName TrackName) {
    if (FloatCurve == nullptr) return;
    FTimelineFloatTrack Track;
    Track.FloatCurve = FloatCurve;
    Track.InterpFunc = InterpFunc;
    Track.TrackName = TrackName;
    Track.FloatPropertyName = PropertyName;
    TheTimeline.InterpFloats.Add(Track);
}

void UTimelineComponent::AddInterpVector(UCurveVector* VectorCurve, FScriptDelegate InterpFunc, FName PropertyName, FName TrackName) {
    if (VectorCurve == nullptr) return;
    FTimelineVectorTrack Track;
    Track.VectorCurve = VectorCurve;
    Track.InterpFunc = InterpFunc;
    Track.TrackName = TrackName;
    Track.VectorPropertyName = PropertyName;
    TheTimeline.InterpVectors.Add(Track);
}

void UTimelineComponent::AddInterpLinearColor(UCurveLinearColor* LinearColorCurve, FScriptDelegate InterpFunc, FName PropertyName,
                                              FName TrackName) {
    if (LinearColorCurve == nullptr) return;
    FTimelineLinearColorTrack Track;
    Track.LinearColorCurve = LinearColorCurve;
    Track.InterpFunc = InterpFunc;
    Track.TrackName = TrackName;
    Track.LinearColorPropertyName = PropertyName;
    TheTimeline.InterpLinearColors.Add(Track);
}

UTimelineComponent* UTimelineComponent::CreateForActor(AActor* Actor, const UTimelineTemplate* TimelineTemplate) {
    if (!IsValid(Actor) || TimelineTemplate == nullptr || Actor->IsTemplate()) return nullptr;
    UTimelineComponent* NewTimeline = NewObject<UTimelineComponent>(Actor, StaticClass(), TimelineTemplate->GetVariableName());
    NewTimeline->CreationMethod = EComponentCreationMethod::UserConstructionScript;
    Actor->BlueprintCreatedComponents.Add(NewTimeline);
    Actor->AddOwnedComponent(NewTimeline);
    NewTimeline->TheTimeline.PropertySetObject = Actor;
    NewTimeline->TheTimeline.DirectionPropertyName = TimelineTemplate->GetDirectionPropertyName();
    NewTimeline->SetTimelineLength(TimelineTemplate->TimelineLength);
    NewTimeline->SetTimelineLengthMode(TimelineTemplate->LengthMode);
    NewTimeline->TheTimeline.TickInterval = TimelineTemplate->TimelineTickInterval;
    NewTimeline->PrimaryComponentTick.TickGroup = TimelineTemplate->TimelineTickGroup;
    if (FObjectPropertyBase* Prop = FindFProperty<FObjectPropertyBase>(Actor->GetClass(), TimelineTemplate->GetVariableName()))
        Prop->SetObjectPropertyValue_InContainer(Actor, NewTimeline);
    for (const FTTEventTrack& Track : TimelineTemplate->EventTracks) {
        if (Track.CurveKeys == nullptr) continue;
        FScriptDelegate EventDelegate;
        EventDelegate.BindUFunction(Actor, Track.GetFunctionName());
        for (const FRichCurveKey& Key : Track.CurveKeys->FloatCurve.GetConstRefOfKeys()) NewTimeline->AddEvent(Key.Time, EventDelegate);
    }
    for (const FTTFloatTrack& Track : TimelineTemplate->FloatTracks)
        if (Track.CurveFloat != nullptr) NewTimeline->AddInterpFloat(Track.CurveFloat, FScriptDelegate(), Track.GetPropertyName(), Track.GetTrackName());
    for (const FTTVectorTrack& Track : TimelineTemplate->VectorTracks)
        if (Track.CurveVector != nullptr) NewTimeline->AddInterpVector(Track.CurveVector, FScriptDelegate(), Track.GetPropertyName(), Track.GetTrackName());
    for (const FTTLinearColorTrack& Track : TimelineTemplate->LinearColorTracks)
        if (Track.CurveLinearColor != nullptr)
            NewTimeline->AddInterpLinearColor(Track.CurveLinearColor, FScriptDelegate(), Track.GetPropertyName(), Track.GetTrackName());
    FScriptDelegate UpdateDelegate;
    UpdateDelegate.BindUFunction(Actor, TimelineTemplate->GetUpdateFunctionName());
    NewTimeline->SetTimelinePostUpdateFunc(UpdateDelegate);
    FScriptDelegate FinishedDelegate;
    FinishedDelegate.BindUFunction(Actor, TimelineTemplate->GetFinishedFunctionName());
    NewTimeline->SetTimelineFinishedFunc(FinishedDelegate);
    NewTimeline->RegisterComponent();
    if (TimelineTemplate->bAutoPlay) {
        NewTimeline->bAutoActivate = true;
        NewTimeline->Play();
    }
    if (TimelineTemplate->bLoop) NewTimeline->SetLooping(true);
    if (TimelineTemplate->bReplicated) NewTimeline->SetIsReplicated(true);
    if (TimelineTemplate->bIgnoreTimeDilation) NewTimeline->SetIgnoreTimeDilation(true);
    return NewTimeline;
}

}

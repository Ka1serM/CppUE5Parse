#pragma once

#include "Engine/Source/Runtime/MovieScene/MovieSceneBinding.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneFrameMigration.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneFwd.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneMarkedFrame.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneObjectBindingID.h"
#include "Engine/Source/Runtime/MovieScene/MovieScenePossessable.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSignedObject.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSpawnable.h"

#include "MovieScene.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneObjectBindingIDs {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMovieSceneObjectBindingID> IDs;
};

UCLASS(DefaultToInstanced)
class UMovieScene : public UMovieSceneSignedObject {
    GENERATED_BODY()
public:
    explicit UMovieScene(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TArray<FMovieSceneSpawnable> Spawnables;
    UPROPERTY()
    TArray<FMovieScenePossessable> Possessables;
    UPROPERTY()
    TArray<FMovieSceneBinding> ObjectBindings;
    UPROPERTY()
    TMap<FName, FMovieSceneObjectBindingIDs> BindingGroups;
    UPROPERTY()
    TArray<TObjectPtr<UMovieSceneTrack>> Tracks;
    UPROPERTY()
    TArray<TObjectPtr<UMovieSceneTrack>> MasterTracks;
    UPROPERTY()
    TObjectPtr<UMovieSceneTrack> ShotTrack = nullptr;
    UPROPERTY()
    TObjectPtr<UMovieSceneTrack> CameraCutTrack = nullptr;
    UPROPERTY()
    float InTime = 0;
    UPROPERTY()
    float OutTime = 0;
    UPROPERTY()
    float StartTime = 0;
    UPROPERTY()
    float EndTime = 0;
    UPROPERTY()
    bool bForceFixedFrameIntervalPlayback = false;
    UPROPERTY()
    float FixedFrameInterval = 0;
    UPROPERTY()
    FMovieSceneFrameRange SelectionRange;
    UPROPERTY()
    FMovieSceneFrameRange PlaybackRange;
    UPROPERTY()
    FFrameRate TickResolution = {.Numerator = 60000, .Denominator = 1};
    UPROPERTY()
    FFrameRate DisplayRate = {.Numerator = 30, .Denominator = 1};
    UPROPERTY()
    EMovieSceneEvaluationType EvaluationType = EMovieSceneEvaluationType::WithSubFrames;
    UPROPERTY()
    EUpdateClockSource ClockSource = EUpdateClockSource::Tick;
    UPROPERTY()
    FSoftObjectPath CustomClockSourcePath;
    UPROPERTY()
    TObjectPtr<UObject> CustomClock = nullptr;
    UPROPERTY()
    TArray<FMovieSceneMarkedFrame> MarkedFrames;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> GeneratedConditions;
};

}

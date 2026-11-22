#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneBlendType.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneFrameMigration.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneFwd.h"
#include "Engine/Source/Runtime/MovieScene/Decorations/MovieSceneDecorationContainer.h"

#include "MovieSceneSection.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneSectionEvalOptions {
    GENERATED_BODY()

    UPROPERTY()
    bool bCanEditCompletionMode = false;
    UPROPERTY()
    EMovieSceneCompletionMode CompletionMode = EMovieSceneCompletionMode::ProjectDefault;
};

USTRUCT()
struct FMovieSceneEasingSettings {
    GENERATED_BODY()

    UPROPERTY()
    int32 AutoEaseInDuration = 0;
    UPROPERTY()
    int32 AutoEaseOutDuration = 0;
    UPROPERTY()
    FScriptInterface EaseIn;
    UPROPERTY()
    bool bManualEaseIn = false;
    UPROPERTY()
    int32 ManualEaseInDuration = 0;
    UPROPERTY()
    FScriptInterface EaseOut;
    UPROPERTY()
    bool bManualEaseOut = false;
    UPROPERTY()
    int32 ManualEaseOutDuration = 0;
    UPROPERTY()
    float AutoEaseInTime = 0;
    UPROPERTY()
    float AutoEaseOutTime = 0;
    UPROPERTY()
    float ManualEaseInTime = 0;
    UPROPERTY()
    float ManualEaseOutTime = 0;
};

USTRUCT()
struct FMovieSceneTimecodeSource {
    GENERATED_BODY()

    UPROPERTY()
    FTimecode Timecode;
    UPROPERTY()
    FFrameNumber DeltaFrame;
};

UCLASS(DefaultToInstanced)
class UMovieSceneSection : public UMovieSceneDecorationContainerObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneSection(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FMovieSceneSectionEvalOptions EvalOptions;
    UPROPERTY()
    FMovieSceneEasingSettings Easing;
    UPROPERTY()
    FMovieSceneFrameRange SectionRange;
    UPROPERTY()
    FFrameNumber PreRollFrames;
    UPROPERTY()
    FFrameNumber PostRollFrames;
    UPROPERTY()
    int32 RowIndex = 0;
    UPROPERTY()
    int32 OverlapPriority = 0;
    UPROPERTY()
    uint32 bIsActive : 1 = true;
    UPROPERTY()
    uint32 bIsLocked : 1 = false;
    UPROPERTY()
    uint32 bIsInfinite : 1 = false;
    UPROPERTY()
    float StartTime = 0;
    UPROPERTY()
    float EndTime = 0;
    UPROPERTY()
    float PreRollTime = 0;
    UPROPERTY()
    float PostRollTime = 0;
    UPROPERTY()
    bool bSupportsInfiniteRange = false;
    UPROPERTY()
    FOptionalMovieSceneBlendType BlendType;
    UPROPERTY()
    FMovieSceneTimecodeSource TimecodeSource;
};

}

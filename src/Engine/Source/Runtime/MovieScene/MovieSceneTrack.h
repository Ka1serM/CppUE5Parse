#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneSegment.h"
#include "Engine/Source/Runtime/MovieScene/Decorations/MovieSceneDecorationContainer.h"

#include "MovieSceneTrack.generated.h"

namespace ue5parse::runtime {

class UMovieSceneSection;

USTRUCT()
struct FMovieSceneTrackEvalOptions {
    GENERATED_BODY()

    UPROPERTY()
    uint32 bCanEvaluateNearestSection : 1 = false;
    UPROPERTY()
    uint32 bEvaluateNearestSection : 1 = false;
    UPROPERTY()
    uint32 bEvalNearestSection : 1 = false;
    UPROPERTY()
    uint32 bEvaluateInPreroll : 1 = false;
    UPROPERTY()
    uint32 bEvaluateInPostroll : 1 = false;
};

USTRUCT()
struct FMovieSceneTrackEvaluationFieldEntry {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UMovieSceneSection> Section = nullptr;
    UPROPERTY()
    FFrameNumberRange Range;
    UPROPERTY()
    FFrameNumber ForcedTime;
    UPROPERTY()
    ESectionEvaluationFlags Flags = ESectionEvaluationFlags::None;
    UPROPERTY()
    int16 LegacySortOrder = 0;
};

USTRUCT()
struct FMovieSceneTrackEvaluationField {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMovieSceneTrackEvaluationFieldEntry> Entries;
};

UCLASS(DefaultToInstanced)
class UMovieSceneTrack : public UMovieSceneDecorationContainerObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneTrack(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FMovieSceneTrackEvalOptions EvalOptions;
    UPROPERTY()
    bool bIsEvalDisabled = false;
    UPROPERTY()
    TArray<int32> RowsDisabled;
    UPROPERTY()
    FGuid EvaluationFieldGuid;
    UPROPERTY()
    FMovieSceneTrackEvaluationField EvaluationField;
    UPROPERTY()
    TObjectPtr<UMovieSceneTrack> ParentTrack = nullptr;
    UPROPERTY()
    bool bAutoExpandToKeyframes = true;
};

UCLASS()
class UMovieSceneNameableTrack : public UMovieSceneTrack {
    GENERATED_BODY()
public:
    explicit UMovieSceneNameableTrack(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

#pragma once

#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvalTemplate.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneSegment.h"

#include "MovieSceneEvaluationTrack.generated.h"

namespace ue5parse::runtime {

class UMovieSceneTrack;

UENUM()
enum class EEvaluationMethod : uint8 { Static, Swept };

USTRUCT()
struct FSectionEvaluationDataTree {
    GENERATED_BODY()

    TMovieSceneEvaluationTree<FSectionEvaluationData> Tree;

    bool Serialize(FArchive& Ar) {
        Ar << Tree;
        return true;
    }
    bool operator==(const FSectionEvaluationDataTree&) const = default;
};

template <>
struct TStructOpsTypeTraits<FSectionEvaluationDataTree> : TStructOpsTypeTraitsBase2<FSectionEvaluationDataTree> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

USTRUCT()
struct FMovieSceneEvaluationTrackSegments {
    GENERATED_BODY()

    UPROPERTY()
    TArray<int32> SegmentIdentifierToIndex;
    UPROPERTY()
    TArray<FMovieSceneSegment> SortedSegments;

    //! 4.15 to 4.18 store the segments as a plain sorted array.
    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMovieSceneEvaluationTrackSegments> : TStructOpsTypeTraitsBase2<FMovieSceneEvaluationTrackSegments> {
    enum { WithSerializeFromMismatchedTag = true };
};

USTRUCT()
struct FMovieSceneEvaluationTrack {
    GENERATED_BODY()

    UPROPERTY()
    FGuid ObjectBindingID;
    UPROPERTY()
    uint16 EvaluationPriority = 1000;
    UPROPERTY()
    EEvaluationMethod EvaluationMethod = EEvaluationMethod::Static;
    UPROPERTY()
    FMovieSceneEvaluationTrackSegments Segments;
    UPROPERTY()
    TWeakObjectPtr<UMovieSceneTrack> SourceTrack;
    UPROPERTY()
    FSectionEvaluationDataTree EvaluationTree;
    UPROPERTY()
    TArray<FMovieSceneEvalTemplatePtr> ChildTemplates;
    UPROPERTY()
    FMovieSceneTrackImplementationPtr TrackTemplate;
    UPROPERTY()
    FName EvaluationGroup;
    UPROPERTY()
    bool bEvaluateInPreroll = true;
    UPROPERTY()
    bool bEvaluateInPostroll = true;
    UPROPERTY()
    bool bTearDownPriority = false;
};

}

#pragma once

#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneTrackIdentifier.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSequenceID.h"

#include "MovieSceneEvaluationKey.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneEvaluationKey {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneSequenceID SequenceID;
    UPROPERTY()
    FMovieSceneTrackIdentifier TrackIdentifier;
    UPROPERTY()
    uint32 SectionIndex = static_cast<uint32>(-1);

    //! 4.17 and 4.18 call the last field SectionIdentifier.
    bool Serialize(FArchive& Ar) {
        Ar << SequenceID << TrackIdentifier << SectionIndex;
        return true;
    }
    bool operator==(const FMovieSceneEvaluationKey&) const = default;
};

template <>
struct TStructOpsTypeTraits<FMovieSceneEvaluationKey> : TStructOpsTypeTraitsBase2<FMovieSceneEvaluationKey> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

USTRUCT()
struct FMovieSceneOrderedEvaluationKey {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneEvaluationKey Key;
    UPROPERTY()
    uint32 EvaluationIndex = 0;
    UPROPERTY()
    uint16 SetupIndex = 0;
    UPROPERTY()
    uint16 TearDownIndex = 0;
};

}

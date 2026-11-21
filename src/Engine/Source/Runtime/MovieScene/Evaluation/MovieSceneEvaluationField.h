#pragma once

#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvaluationTree.h"

namespace ue5parse::runtime {

struct FMovieSceneEvaluationFieldEntityTree {
    struct FEntityAndMetaDataIndex {
        int32 EntityIndex = INDEX_NONE;
        int32 MetaDataIndex = INDEX_NONE;
        bool operator==(const FEntityAndMetaDataIndex&) const = default;

        friend FArchive& operator<<(FArchive& Ar, FEntityAndMetaDataIndex& Pair) { return Ar << Pair.EntityIndex << Pair.MetaDataIndex; }
    };

    TMovieSceneEvaluationTree<FEntityAndMetaDataIndex> SerializedData;
    bool operator==(const FMovieSceneEvaluationFieldEntityTree&) const = default;

    bool Serialize(FArchive& Ar) {
        Ar << SerializedData;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMovieSceneEvaluationFieldEntityTree> : TStructOpsTypeTraitsBase2<FMovieSceneEvaluationFieldEntityTree> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}

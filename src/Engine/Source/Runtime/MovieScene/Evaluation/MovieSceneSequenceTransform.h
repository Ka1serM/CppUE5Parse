#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "MovieSceneSequenceTransform.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneTimeTransform {
    GENERATED_BODY()

    UPROPERTY()
    float TimeScale = 1.0f;
    UPROPERTY()
    FFrameTime Offset;
};

USTRUCT()
struct FMovieSceneTimeWarping {
    GENERATED_BODY()

    UPROPERTY()
    FFrameNumber Start;
    UPROPERTY()
    FFrameNumber End;
};

USTRUCT()
struct FMovieSceneNestedSequenceTransform {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneTimeTransform LinearTransform;
    UPROPERTY()
    FMovieSceneTimeWarping Warping;
};

USTRUCT()
struct FMovieSceneSequenceTransform {
    GENERATED_BODY()

    UPROPERTY()
    float TimeScale = 1.0f;
    UPROPERTY()
    FFrameTime Offset;
    UPROPERTY()
    FMovieSceneTimeTransform LinearTransform;
    UPROPERTY()
    TArray<FMovieSceneNestedSequenceTransform> NestedTransforms;
};

USTRUCT()
struct FMovieSceneWarpCounter {
    GENERATED_BODY()

    UPROPERTY()
    TArray<uint32> WarpCounts;

    bool operator==(const FMovieSceneWarpCounter&) const = default;
};

}

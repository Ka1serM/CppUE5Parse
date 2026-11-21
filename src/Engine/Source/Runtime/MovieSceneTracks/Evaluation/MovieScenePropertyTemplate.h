#pragma once

#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvalTemplate.h"

#include "MovieScenePropertyTemplate.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieScenePropertySectionData {
    GENERATED_BODY()

    UPROPERTY()
    FName PropertyName;
    UPROPERTY()
    FString PropertyPath;
    UPROPERTY()
    FName FunctionName;
    UPROPERTY()
    FName NotifyFunctionName;
};

USTRUCT()
struct FMovieScenePropertySectionTemplate : public FMovieSceneEvalTemplate {
    GENERATED_BODY()

    UPROPERTY()
    FMovieScenePropertySectionData PropertyData;
};

}

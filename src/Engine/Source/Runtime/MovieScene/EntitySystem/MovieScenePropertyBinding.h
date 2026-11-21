#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "MovieScenePropertyBinding.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieScenePropertyBinding {
    GENERATED_BODY()

    FMovieScenePropertyBinding() = default;
    FMovieScenePropertyBinding(FName InPropertyName, const FString& InPropertyPath);

    UPROPERTY()
    FName PropertyName;
    UPROPERTY()
    FName PropertyPath;
    UPROPERTY()
    bool bCanUseClassLookup = false;
};

}

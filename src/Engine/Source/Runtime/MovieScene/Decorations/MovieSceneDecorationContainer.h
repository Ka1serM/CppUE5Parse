#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "MovieSceneDecorationContainer.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneDecorationContainer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<UObject>> Decorations;
};

}

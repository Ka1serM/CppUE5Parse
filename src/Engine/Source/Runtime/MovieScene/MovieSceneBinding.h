#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "Engine/Source/Runtime/MovieScene/Decorations/MovieSceneDecorationContainer.h"

#include "MovieSceneBinding.generated.h"

namespace ue5parse::runtime {

class UMovieSceneTrack;

USTRUCT()
struct FMovieSceneBinding : public FMovieSceneDecorationContainer {
    GENERATED_BODY()

    UPROPERTY()
    FGuid ObjectGuid;
    UPROPERTY()
    FString BindingName;
    UPROPERTY()
    TArray<TObjectPtr<UMovieSceneTrack>> Tracks;
};

}

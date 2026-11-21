#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "MovieSceneBinding.generated.h"

namespace ue5parse::runtime {

class UMovieSceneTrack;

USTRUCT()
struct FMovieSceneBinding {
    GENERATED_BODY()

    UPROPERTY()
    FGuid ObjectGuid;
    UPROPERTY()
    FString BindingName;
    UPROPERTY()
    TArray<TObjectPtr<UMovieSceneTrack>> Tracks;
};

}

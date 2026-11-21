#pragma once

#include "Engine/Source/Runtime/MovieScene/MovieSceneDynamicBinding.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneObjectBindingID.h"

#include "MovieScenePossessable.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieScenePossessable {
    GENERATED_BODY()

    UPROPERTY()
    FGuid Guid;
    UPROPERTY()
    FString Name;
    UPROPERTY()
    TObjectPtr<UClass> PossessedObjectClass = nullptr;
    UPROPERTY()
    FGuid ParentGuid;
    UPROPERTY()
    TArray<FName> Tags;
    UPROPERTY()
    FMovieSceneObjectBindingID SpawnableObjectBindingID;
    UPROPERTY()
    FMovieSceneDynamicBinding DynamicBinding;
};

}

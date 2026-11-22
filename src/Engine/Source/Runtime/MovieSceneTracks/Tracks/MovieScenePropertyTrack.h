#pragma once

#include "Engine/Source/Runtime/MovieScene/EntitySystem/MovieScenePropertyBinding.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSection.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneTrack.h"

#include "MovieScenePropertyTrack.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UMovieScenePropertyTrack : public UMovieSceneTrack {
    GENERATED_BODY()
public:
    explicit UMovieScenePropertyTrack(const FObjectInitializer& ObjectInitializer);

    void SetPropertyNameAndPath(FName InPropertyName, const FString& InPropertyPath);

    UPROPERTY()
    TObjectPtr<UMovieSceneSection> SectionToKey;
    UPROPERTY()
    FMovieScenePropertyBinding PropertyBinding;
    UPROPERTY()
    FName PropertyName;
    UPROPERTY()
    FString PropertyPath;
    UPROPERTY()
    bool bSectionsAreShowable = false;
    UPROPERTY()
    TArray<TObjectPtr<UMovieSceneSection>> Sections;
};

}

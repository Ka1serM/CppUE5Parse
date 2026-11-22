#include "Engine/Source/Runtime/MovieSceneTracks/Tracks/MovieScenePropertyTrack.h"

namespace ue5parse::runtime {

UMovieScenePropertyTrack::UMovieScenePropertyTrack(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    EvalOptions.bEvaluateNearestSection = true;
    EvalOptions.bCanEvaluateNearestSection = true;
}

void UMovieScenePropertyTrack::SetPropertyNameAndPath(FName InPropertyName, const FString& InPropertyPath) {
    PropertyBinding = FMovieScenePropertyBinding(InPropertyName, InPropertyPath);
    PropertyName = InPropertyName;
    PropertyPath = InPropertyPath;
}

}

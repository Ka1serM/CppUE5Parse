#include "Engine/Source/Runtime/MovieSceneTracks/Sections/MovieSceneFloatSection.h"

namespace ue5parse::runtime {

UMovieSceneFloatSection::UMovieSceneFloatSection(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    EvalOptions.bCanEditCompletionMode = true;
    EvalOptions.CompletionMode = EMovieSceneCompletionMode::ProjectDefault;
    BlendType = {EMovieSceneBlendType::Absolute, true};
    bSupportsInfiniteRange = true;
}

}

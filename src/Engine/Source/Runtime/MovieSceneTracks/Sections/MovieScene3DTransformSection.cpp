#include "Engine/Source/Runtime/MovieSceneTracks/Sections/MovieScene3DTransformSection.h"

namespace ue5parse::runtime {

UMovieScene3DTransformSection::UMovieScene3DTransformSection(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    EvalOptions.bCanEditCompletionMode = true;
    EvalOptions.CompletionMode = EMovieSceneCompletionMode::ProjectDefault;
    TransformMask.Mask = static_cast<uint32>(EMovieSceneTransformChannel::AllTransform);
    BlendType = {EMovieSceneBlendType::Absolute, true};
    bSupportsInfiniteRange = true;
    for (int32 Axis = 0; Axis < 3; Axis++) {
        Translation[Axis].DefaultValue = 0;
        Translation[Axis].bHasDefaultValue = true;
        Rotation[Axis].DefaultValue = 0;
        Rotation[Axis].bHasDefaultValue = true;
        Scale[Axis].DefaultValue = 1;
        Scale[Axis].bHasDefaultValue = true;
    }
}

}

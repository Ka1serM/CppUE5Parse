#pragma once

#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneFloatChannel.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneBlendType.h"
#include "Engine/Source/Runtime/MovieSceneTracks/Evaluation/MovieScenePropertyTemplate.h"

#include "MovieScenePropertyTemplates.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneFloatPropertySectionTemplate : public FMovieScenePropertySectionTemplate {
    GENERATED_BODY()

    //! FloatCurve is an FRichCurve before 4.20, read through the channel's rich curve conversion.
    UPROPERTY()
    FMovieSceneFloatChannel FloatCurve;
    UPROPERTY()
    FMovieSceneFloatChannel FloatFunction;
    UPROPERTY()
    EMovieSceneBlendType BlendType = EMovieSceneBlendType::Invalid;
};

}

#pragma once

#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneFloatChannel.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneBlendType.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvalTemplate.h"
#include "Engine/Source/Runtime/MovieSceneTracks/Sections/MovieScene3DTransformSection.h"

#include "MovieScene3DTransformTemplate.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieScene3DTransformTemplateData {
    GENERATED_BODY()

    //! The curves are FRichCurve before 4.20, read through the channel's rich curve conversion.
    UPROPERTY()
    FMovieSceneFloatChannel TranslationCurve[3];
    UPROPERTY()
    FMovieSceneFloatChannel RotationCurve[3];
    UPROPERTY()
    FMovieSceneFloatChannel ScaleCurve[3];
    UPROPERTY()
    FMovieSceneFloatChannel ManualWeight;
    UPROPERTY()
    EMovieSceneBlendType BlendType = EMovieSceneBlendType::Invalid;
    UPROPERTY()
    FMovieSceneTransformMask Mask;
    UPROPERTY()
    bool bUseQuaternionInterpolation = false;
};

USTRUCT()
struct FMovieSceneComponentTransformSectionTemplate : public FMovieSceneEvalTemplate {
    GENERATED_BODY()

    UPROPERTY()
    FMovieScene3DTransformTemplateData TemplateData;
};

USTRUCT()
struct FMovieScene3DTransformSectionTemplate : public FMovieSceneEvalTemplate {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneFloatChannel TranslationCurve[3];
    UPROPERTY()
    FMovieSceneFloatChannel RotationCurve[3];
    UPROPERTY()
    FMovieSceneFloatChannel ScaleCurve[3];
};

}

#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneCurveChannelCommon.h"

namespace ue5parse::runtime {

struct FMovieSceneDoubleValue {
    double Value = 0;
    FMovieSceneTangentData Tangent;
    TEnumAsByte<ERichCurveInterpMode> InterpMode = static_cast<ERichCurveInterpMode>(2);
    TEnumAsByte<ERichCurveTangentMode> TangentMode = static_cast<ERichCurveTangentMode>(0);
    uint8 PaddingByte = 0;
    bool operator==(const FMovieSceneDoubleValue&) const = default;
};

struct FMovieSceneDoubleChannel {
    using ChannelValueType = FMovieSceneDoubleValue;

    TEnumAsByte<ERichCurveExtrapolation> PreInfinityExtrap = static_cast<ERichCurveExtrapolation>(4);
    TEnumAsByte<ERichCurveExtrapolation> PostInfinityExtrap = static_cast<ERichCurveExtrapolation>(4);
    TArray<FFrameNumber> Times;
    TArray<FMovieSceneDoubleValue> Values;
    double DefaultValue = 0;
    bool bHasDefaultValue = false;
    FFrameRate TickResolution{60000, 1};
    bool operator==(const FMovieSceneDoubleChannel&) const = default;
};

DECLARE_BASE_STRUCTURE(FMovieSceneDoubleValue, "MovieSceneDoubleValue")
DECLARE_BASE_STRUCTURE(FMovieSceneDoubleChannel, "MovieSceneDoubleChannel")

}

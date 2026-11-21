#pragma once

#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneCurveChannelCommon.h"

#include "MovieSceneDoubleChannel.generated.h"

namespace ue5parse::runtime {

struct FPropertyTag;

USTRUCT()
struct FMovieSceneDoubleValue {
    GENERATED_BODY()

    UPROPERTY()
    double Value = 0;
    UPROPERTY()
    FMovieSceneTangentData Tangent;
    UPROPERTY()
    TEnumAsByte<ERichCurveInterpMode> InterpMode = RCIM_Cubic;
    UPROPERTY()
    TEnumAsByte<ERichCurveTangentMode> TangentMode = RCTM_Auto;
    UPROPERTY()
    uint8 PaddingByte = 0;
    uint8 UnserializedPaddingBytes[1] = {};

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMovieSceneDoubleValue> : TStructOpsTypeTraitsBase2<FMovieSceneDoubleValue> {
    enum { WithSerializer = true };
};

inline FArchive& operator<<(FArchive& Ar, FMovieSceneDoubleValue& Value) {
    Value.Serialize(Ar);
    return Ar;
}

USTRUCT()
struct FMovieSceneDoubleChannel : public FMovieSceneChannel {
    GENERATED_BODY()

    using ValueType = FMovieSceneDoubleValue;

    UPROPERTY()
    TEnumAsByte<ERichCurveExtrapolation> PreInfinityExtrap = RCCE_Constant;
    UPROPERTY()
    TEnumAsByte<ERichCurveExtrapolation> PostInfinityExtrap = RCCE_Constant;
    UPROPERTY()
    TArray<FFrameNumber> Times;
    UPROPERTY()
    TArray<FMovieSceneDoubleValue> Values;
    UPROPERTY()
    double DefaultValue = 0;
    UPROPERTY()
    bool bHasDefaultValue = false;
    UPROPERTY()
    FFrameRate TickResolution;
    UPROPERTY()
    bool bShowCurve = false;

    bool Serialize(FArchive& Ar);
    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMovieSceneDoubleChannel> : TStructOpsTypeTraitsBase2<FMovieSceneDoubleChannel> {
    enum { WithSerializer = true, WithSerializeFromMismatchedTag = true };
};

}

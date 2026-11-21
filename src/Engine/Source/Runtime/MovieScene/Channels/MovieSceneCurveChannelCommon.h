#pragma once

#include "Engine/Source/Runtime/Engine/Curves/RealCurve.h"
#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneChannel.h"

#include "MovieSceneCurveChannelCommon.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneTangentData {
    GENERATED_BODY()

    UPROPERTY()
    float ArriveTangent = 0;
    UPROPERTY()
    float LeaveTangent = 0;
    UPROPERTY()
    float ArriveTangentWeight = 0;
    UPROPERTY()
    float LeaveTangentWeight = 0;
    UPROPERTY()
    TEnumAsByte<ERichCurveTangentWeightMode> TangentWeightMode = RCTWM_WeightedNone;
    uint8 UnserializedPaddingBytes[3] = {};

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMovieSceneTangentData> : TStructOpsTypeTraitsBase2<FMovieSceneTangentData> {
    enum { WithSerializer = true };
};

inline FArchive& operator<<(FArchive& Ar, FMovieSceneTangentData& Tangent) {
    Tangent.Serialize(Ar);
    return Ar;
}

}

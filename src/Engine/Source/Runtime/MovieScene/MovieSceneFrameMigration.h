#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ue5parse::runtime {

inline FFrameRate GetLegacyConversionFrameRate() {
    return {60000, 1};
}

inline FFrameNumber UpgradeLegacyMovieSceneTime(FFrameRate Rate, double Time) {
    const double MaxSeconds = static_cast<double>(std::numeric_limits<int32>::max()) * Rate.Denominator / Rate.Numerator;
    const double Frame = std::clamp(Time, -MaxSeconds, MaxSeconds) * Rate.Numerator / Rate.Denominator;
    const double Rounded = std::floor(Frame + std::trunc(static_cast<float>(Frame - std::floor(Frame))));
    return {static_cast<int32>(std::clamp(Rounded, static_cast<double>(std::numeric_limits<int32>::min()), static_cast<double>(std::numeric_limits<int32>::max())))};
}

inline FFrameNumberRangeBound UpgradeLegacyRangeBound(const FFloatRangeBound& Bound) {
    if (Bound.Type == ERangeBoundTypes::Open)
        return {};
    return {ERangeBoundTypes::Inclusive, UpgradeLegacyMovieSceneTime(GetLegacyConversionFrameRate(), Bound.Value)};
}

struct FMovieSceneFrameRange {
    FFrameNumberRangeBound LowerBound;
    FFrameNumberRangeBound UpperBound;
    bool operator==(const FMovieSceneFrameRange&) const = default;
};

DECLARE_BASE_STRUCTURE(FMovieSceneFrameRange, "MovieSceneFrameRange")

}

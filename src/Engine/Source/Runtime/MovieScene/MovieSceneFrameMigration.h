#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

namespace ue5parse::runtime {

struct FFrameNumberRangeBound {
    TEnumAsByte<ERangeBoundTypes::Type> Type = ERangeBoundTypes::Open;
    FFrameNumber Value;
    bool operator==(const FFrameNumberRangeBound&) const = default;
};

inline FArchive& operator<<(FArchive& Ar, FFrameNumberRangeBound& Bound) {
    return Ar << Bound.Type << Bound.Value;
}

struct FMovieSceneFrameRange {
    FFrameNumberRangeBound LowerBound;
    FFrameNumberRangeBound UpperBound;
    bool operator==(const FMovieSceneFrameRange&) const = default;
};

DECLARE_BASE_STRUCTURE(FMovieSceneFrameRange, "MovieSceneFrameRange")

}

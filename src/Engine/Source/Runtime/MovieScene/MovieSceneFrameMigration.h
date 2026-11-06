#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

namespace ue5parse::runtime {

enum class ERangeBoundTypes : uint8 { Exclusive, Inclusive, Open };

struct FFrameNumberRangeBound {
    ERangeBoundTypes Type = ERangeBoundTypes::Open;
    FFrameNumber Value;
    bool operator==(const FFrameNumberRangeBound&) const = default;
};

inline FArchive& operator<<(FArchive& Ar, FFrameNumberRangeBound& Bound) { return Ar << reinterpret_cast<uint8&>(Bound.Type) << Bound.Value; }

struct FMovieSceneFrameRange {
    FFrameNumberRangeBound LowerBound;
    FFrameNumberRangeBound UpperBound;
    bool operator==(const FMovieSceneFrameRange&) const = default;
};

DECLARE_BASE_STRUCTURE(FMovieSceneFrameRange, "MovieSceneFrameRange")

}

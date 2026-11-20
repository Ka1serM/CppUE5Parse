#pragma once

#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

namespace ue5parse::runtime {

enum ERichCurveInterpMode : int32;
enum ERichCurveTangentMode : int32;
enum ERichCurveTangentWeightMode : int32;
enum ERichCurveExtrapolation : int32;

struct FMovieSceneTangentData {
    float ArriveTangent = 0.f;
    float LeaveTangent = 0.f;
    float ArriveTangentWeight = 0.f;
    float LeaveTangentWeight = 0.f;
    TEnumAsByte<ERichCurveTangentWeightMode> TangentWeightMode = static_cast<ERichCurveTangentWeightMode>(0);
    bool operator==(const FMovieSceneTangentData&) const = default;
};

DECLARE_BASE_STRUCTURE(FMovieSceneTangentData, "MovieSceneTangentData")

}

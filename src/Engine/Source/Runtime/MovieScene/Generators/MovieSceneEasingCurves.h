#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "MovieSceneEasingCurves.generated.h"

namespace ue5parse::runtime {

class UCurveFloat;

UENUM()
enum class EMovieSceneBuiltInEasing : uint8 {
    Linear,
    SinIn,
    SinOut,
    SinInOut,
    QuadIn,
    QuadOut,
    QuadInOut,
    Cubic,
    CubicIn,
    CubicOut,
    CubicInOut,
    HermiteCubicInOut,
    QuartIn,
    QuartOut,
    QuartInOut,
    QuintIn,
    QuintOut,
    QuintInOut,
    ExpoIn,
    ExpoOut,
    ExpoInOut,
    CircIn,
    CircOut,
    CircInOut,
    Custom,
};

UCLASS(DefaultToInstanced)
class UMovieSceneBuiltInEasingFunction : public UObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneBuiltInEasingFunction(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    EMovieSceneBuiltInEasing Type = EMovieSceneBuiltInEasing::CubicInOut;
};

UCLASS(DefaultToInstanced)
class UMovieSceneEasingExternalCurve : public UObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneEasingExternalCurve(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UCurveFloat> Curve = nullptr;
};

}

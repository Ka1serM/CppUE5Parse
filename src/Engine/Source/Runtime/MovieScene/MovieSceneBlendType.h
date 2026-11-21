#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "MovieSceneBlendType.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EMovieSceneBlendType : uint8 { Invalid, Absolute = 0x1, Additive = 0x2, Relative = 0x4, AdditiveFromBase = 0x8 };

USTRUCT()
struct FOptionalMovieSceneBlendType {
    GENERATED_BODY()

    UPROPERTY()
    EMovieSceneBlendType BlendType = EMovieSceneBlendType::Absolute;
    UPROPERTY()
    bool bIsValid = false;
};

}

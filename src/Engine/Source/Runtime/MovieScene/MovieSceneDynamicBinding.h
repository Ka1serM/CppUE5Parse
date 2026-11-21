#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/FieldPath.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "MovieSceneDynamicBinding.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneDynamicBinding {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UFunction> Function = nullptr;
    UPROPERTY()
    FFieldPath ResolveParamsProperty;
};

}

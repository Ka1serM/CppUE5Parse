#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "MovieSceneBindingOverrides.generated.h"

namespace ue5parse::runtime {

UCLASS(DefaultToInstanced)
class UMovieSceneBindingOverrides : public UObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneBindingOverrides(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

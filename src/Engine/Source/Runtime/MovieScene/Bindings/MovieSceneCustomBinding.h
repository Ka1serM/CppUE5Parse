#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "MovieSceneCustomBinding.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UMovieSceneCustomBinding : public UObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneCustomBinding(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "MovieSceneEntitySystem.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UMovieSceneEntitySystem : public UObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneEntitySystem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS(Abstract)
class UMovieSceneBlenderSystem : public UMovieSceneEntitySystem {
    GENERATED_BODY()
public:
    explicit UMovieSceneBlenderSystem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

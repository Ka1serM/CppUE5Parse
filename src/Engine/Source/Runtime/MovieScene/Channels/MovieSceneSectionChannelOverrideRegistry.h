#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "MovieSceneSectionChannelOverrideRegistry.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMovieSceneSectionChannelOverrideRegistry : public UObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneSectionChannelOverrideRegistry(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

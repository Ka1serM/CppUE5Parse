#pragma once

#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneFloatChannel.h"
#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneSectionChannelOverrideRegistry.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSection.h"

#include "MovieSceneFloatSection.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMovieSceneFloatSection : public UMovieSceneSection {
    GENERATED_BODY()
public:
    explicit UMovieSceneFloatSection(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FMovieSceneFloatChannel FloatCurve;
    UPROPERTY()
    TObjectPtr<UMovieSceneSectionChannelOverrideRegistry> OverrideRegistry;
};

}

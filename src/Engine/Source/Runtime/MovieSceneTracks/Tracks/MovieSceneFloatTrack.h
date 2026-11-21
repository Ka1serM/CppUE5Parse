#pragma once

#include "Engine/Source/Runtime/MovieSceneTracks/Tracks/MovieScenePropertyTrack.h"

#include "MovieSceneFloatTrack.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMovieSceneFloatTrack : public UMovieScenePropertyTrack {
    GENERATED_BODY()
public:
    explicit UMovieSceneFloatTrack(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

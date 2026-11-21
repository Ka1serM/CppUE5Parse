#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/MovieScene/EntitySystem/MovieSceneEntitySystem.h"
#include "Engine/Source/Runtime/MovieSceneTracks/Tracks/MovieScenePropertyTrack.h"

#include "MovieScene3DTransformTrack.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMovieScene3DTransformTrack : public UMovieScenePropertyTrack {
    GENERATED_BODY()
public:
    explicit UMovieScene3DTransformTrack(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TSubclassOf<UMovieSceneBlenderSystem> BlenderSystemClass;
};

}

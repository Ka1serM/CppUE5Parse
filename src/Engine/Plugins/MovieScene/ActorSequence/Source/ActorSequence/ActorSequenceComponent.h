#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSequencePlaybackSettings.h"

#include "ActorSequenceComponent.generated.h"

namespace ue5parse::runtime {

class UActorSequence;
class UActorSequencePlayer;

UCLASS()
class UActorSequenceComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UActorSequenceComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    bool bAutoPlay = false;
    UPROPERTY()
    FMovieSceneSequencePlaybackSettings PlaybackSettings;
    UPROPERTY()
    TObjectPtr<UActorSequence> Sequence = nullptr;
    UPROPERTY()
    TObjectPtr<UActorSequencePlayer> SequencePlayer = nullptr;
};

}

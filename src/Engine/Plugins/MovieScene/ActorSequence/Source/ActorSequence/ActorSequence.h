#pragma once

#include "Engine/Plugins/MovieScene/ActorSequence/Source/ActorSequence/ActorSequenceObjectReference.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSequence.h"

#include "ActorSequence.generated.h"

namespace ue5parse::runtime {

class UMovieScene;

UCLASS(DefaultToInstanced)
class UActorSequence : public UMovieSceneSequence {
    GENERATED_BODY()
public:
    explicit UActorSequence(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UMovieScene> MovieScene = nullptr;
    UPROPERTY()
    FActorSequenceObjectReferenceMap ObjectReferences;
    UPROPERTY()
    bool bHasBeenInitialized = false;
};

}

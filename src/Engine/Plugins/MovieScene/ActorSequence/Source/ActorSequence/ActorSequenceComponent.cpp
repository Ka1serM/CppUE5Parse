#include "Engine/Plugins/MovieScene/ActorSequence/Source/ActorSequence/ActorSequenceComponent.h"

#include "Engine/Plugins/MovieScene/ActorSequence/Source/ActorSequence/ActorSequence.h"

namespace ue5parse::runtime {

UActorSequenceComponent::UActorSequenceComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    Sequence = ObjectInitializer.CreateDefaultSubobject<UActorSequence>(FName("Sequence"));
}

}

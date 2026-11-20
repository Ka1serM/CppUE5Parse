#include "Engine/Source/Runtime/Engine/Sound/AmbientSound.h"

#include "Engine/Source/Runtime/Engine/Components/AudioComponent.h"

namespace ue5parse::runtime {

AAmbientSound::AAmbientSound(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    AudioComponent = ObjectInitializer.CreateDefaultSubobject<UAudioComponent>(FName("AudioComponent0"));
    AudioComponent->Mobility = EComponentMobility::Movable;
    RootComponent = AudioComponent;
    bHidden = true;
}

}

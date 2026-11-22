#include "GameTypes/Fortnite/FortPropAudioComponent.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/UnknownProperties.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundBase.h"

namespace ue5parse::runtime {

void UFortPropAudioComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Sound != nullptr)
        return;
    Sound = Cast<USoundBase>(GetLoadedObjectPropertyValue(Ar, this, FName("SoundLoop")));
}

}

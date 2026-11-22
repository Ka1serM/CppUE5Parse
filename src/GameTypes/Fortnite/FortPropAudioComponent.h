#pragma once

#include "Engine/Source/Runtime/Engine/Components/AudioComponent.h"

#include "FortPropAudioComponent.generated.h"

namespace ue5parse::runtime {

//! A Fortnite prop sound plays its SoundLoop when the component has no Sound.
UCLASS()
class UFortPropAudioComponent : public UAudioComponent {
    GENERATED_BODY()
public:
    explicit UFortPropAudioComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;
};

}

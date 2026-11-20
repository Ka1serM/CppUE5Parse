#pragma once

#include "Engine/Source/Runtime/AudioMixer/Components/SynthComponent.h"

#include "MediaSoundComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMediaSoundComponent : public USynthComponent {
    GENERATED_BODY()
public:
    explicit UMediaSoundComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

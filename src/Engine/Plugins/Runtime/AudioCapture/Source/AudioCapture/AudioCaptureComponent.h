#pragma once

#include "Engine/Source/Runtime/AudioMixer/Components/SynthComponent.h"

#include "AudioCaptureComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAudioCaptureComponent : public USynthComponent {
    GENERATED_BODY()
public:
    explicit UAudioCaptureComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

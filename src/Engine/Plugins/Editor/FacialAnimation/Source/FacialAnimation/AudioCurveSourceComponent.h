#pragma once

#include "Engine/Source/Runtime/Engine/Components/AudioComponent.h"

#include "AudioCurveSourceComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAudioCurveSourceComponent : public UAudioComponent {
    GENERATED_BODY()
public:
    explicit UAudioCurveSourceComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

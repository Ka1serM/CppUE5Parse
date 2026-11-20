#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "ReverbVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AReverbVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit AReverbVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

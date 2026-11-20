#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "LightmassCharacterIndirectDetailVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ALightmassCharacterIndirectDetailVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit ALightmassCharacterIndirectDetailVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

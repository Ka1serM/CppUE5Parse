#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "LightmassImportanceVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ALightmassImportanceVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit ALightmassImportanceVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

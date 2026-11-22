#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "NavModifierVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ANavModifierVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit ANavModifierVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "TriggerVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ATriggerVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit ATriggerVolume(const FObjectInitializer& ObjectInitializer);
};

}

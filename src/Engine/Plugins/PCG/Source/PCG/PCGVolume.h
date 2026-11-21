#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "PCGVolume.generated.h"

namespace ue5parse::runtime {

class UPCGComponent;

UCLASS()
class APCGVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit APCGVolume(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UPCGComponent> PCGComponent = nullptr;
};

}

#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "LightmassPortal.generated.h"

namespace ue5parse::runtime {

class ULightmassPortalComponent;

UCLASS()
class ALightmassPortal : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<ULightmassPortalComponent> PortalComponent = nullptr;

    explicit ALightmassPortal(const FObjectInitializer& ObjectInitializer);
};

}

#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"

#include "SkyAtmosphere.generated.h"

namespace ue5parse::runtime {

class USkyAtmosphereComponent;

UCLASS()
class ASkyAtmosphere : public AInfo {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USkyAtmosphereComponent> SkyAtmosphereComponent = nullptr;

    explicit ASkyAtmosphere(const FObjectInitializer& ObjectInitializer);
};

}

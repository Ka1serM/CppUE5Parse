#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"

#include "ExponentialHeightFog.generated.h"

namespace ue5parse::runtime {

class UExponentialHeightFogComponent;

UCLASS()
class AExponentialHeightFog : public AInfo {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UExponentialHeightFogComponent> Component = nullptr;

    explicit AExponentialHeightFog(const FObjectInitializer& ObjectInitializer);
};

}

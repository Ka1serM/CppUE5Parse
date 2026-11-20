#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"

#include "WorldDataLayers.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AWorldDataLayers : public AInfo {
    GENERATED_BODY()
public:
    explicit AWorldDataLayers(const FObjectInitializer& ObjectInitializer);
};

}

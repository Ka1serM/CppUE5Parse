#pragma once

#include "Engine/Source/Runtime/Landscape/LandscapeProxy.h"

#include "Landscape.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ALandscape : public ALandscapeProxy {
    GENERATED_BODY()
public:
    explicit ALandscape(const FObjectInitializer& ObjectInitializer);
};

}

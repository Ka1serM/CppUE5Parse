#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Brush.h"

#include "Volume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AVolume : public ABrush {
    GENERATED_BODY()
public:
    explicit AVolume(const FObjectInitializer& ObjectInitializer);
};

}

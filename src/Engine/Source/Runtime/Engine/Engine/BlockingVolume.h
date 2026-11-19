#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "BlockingVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ABlockingVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit ABlockingVolume(const FObjectInitializer& ObjectInitializer);
};

}

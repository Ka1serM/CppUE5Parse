#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "NavSystemConfigOverride.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ANavSystemConfigOverride : public AActor {
    GENERATED_BODY()
public:
    explicit ANavSystemConfigOverride(const FObjectInitializer& ObjectInitializer);
};

}

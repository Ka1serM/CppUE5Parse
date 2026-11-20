#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "NavMeshBoundsVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ANavMeshBoundsVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit ANavMeshBoundsVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

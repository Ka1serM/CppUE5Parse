#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "PCGWorldActor.generated.h"

namespace ue5parse::runtime {

class UPCGLandscapeCache;

UCLASS()
class APCGWorldActor : public AActor {
    GENERATED_BODY()
public:
    explicit APCGWorldActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    uint32 PartitionGridSize = 0;
    UPROPERTY()
    TObjectPtr<UPCGLandscapeCache> LandscapeCacheObject = nullptr;
    UPROPERTY()
    bool bUse2DGrid = false;
};

}

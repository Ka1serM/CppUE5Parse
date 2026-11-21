#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "CacheManagerActor.generated.h"

namespace ue5parse::runtime {

class UBillboardComponent;

UCLASS()
class AChaosCacheManager : public AActor {
    GENERATED_BODY()
public:
    explicit AChaosCacheManager(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UObject> CacheCollection = nullptr;
    UPROPERTY()
    float StartTime = 0.0f;
};

UCLASS()
class AChaosCachePlayer : public AChaosCacheManager {
    GENERATED_BODY()
public:
    explicit AChaosCachePlayer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

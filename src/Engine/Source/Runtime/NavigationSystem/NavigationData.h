#pragma once

#include "Engine/Source/Runtime/Engine/AI/Navigation/NavigationTypes.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "NavigationData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSupportedAreaData {
    GENERATED_BODY()
    UPROPERTY()
    FString AreaClassName;
    UPROPERTY()
    int32 AreaID = -1;
    UPROPERTY()
    TObjectPtr<UClass> AreaClass = nullptr;
};

UCLASS(abstract)
class ANavigationData : public AActor {
    GENERATED_BODY()
public:
    explicit ANavigationData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FNavDataConfig NavDataConfig;
    UPROPERTY()
    uint32 bEnableDrawing : 1 = 0;
    UPROPERTY()
    uint32 bRebuildAtRuntime : 1 = 0;
    UPROPERTY()
    float ObservedPathsTickInterval = 0.5f;
    UPROPERTY()
    TArray<FSupportedAreaData> SupportedAreas;
};

}

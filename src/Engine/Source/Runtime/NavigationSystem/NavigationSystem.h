#pragma once

#include "Engine/Source/Runtime/Engine/AI/NavigationSystemConfig.h"

#include "NavigationSystem.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNavigationSystemModuleConfig : public UNavigationSystemConfig {
    GENERATED_BODY()
public:
    explicit UNavigationSystemModuleConfig(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    uint32 bDirtyNavDataPackageOnNavigationGenerationFinished : 1 = false;
    UPROPERTY()
    uint32 bStrictlyStatic : 1 = false;
    UPROPERTY()
    uint32 bCreateOnClient : 1 = false;
    UPROPERTY()
    uint32 bAutoSpawnMissingNavData : 1 = false;
    UPROPERTY()
    uint32 bSpawnNavDataInNavBoundsLevel : 1 = false;
};

}

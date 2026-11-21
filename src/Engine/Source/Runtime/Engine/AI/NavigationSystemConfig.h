#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/Engine/AI/Navigation/NavLinkDefinition.h"

#include "NavigationSystemConfig.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ENavConfigOverride : uint8 { UseProjectSetting, ForceEnabled, ForceDisabled };

UCLASS()
class UNavigationSystemConfig : public UObject {
    GENERATED_BODY()
public:
    explicit UNavigationSystemConfig(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FSoftClassPath NavigationSystemClass;
    UPROPERTY()
    FNavAgentSelector SupportedAgentsMask;
    UPROPERTY()
    FName DefaultAgentName;
    UPROPERTY()
    ENavConfigOverride InvokerGenerationModeOverride = ENavConfigOverride::UseProjectSetting;
    UPROPERTY()
    ENavConfigOverride PreserveTilesOutsideInvokerRadiusOverride = ENavConfigOverride::UseProjectSetting;
    UPROPERTY()
    ENavConfigOverride ProgressiveBuildOverride = ENavConfigOverride::UseProjectSetting;
    UPROPERTY()
    uint32 bIsOverriden : 1 = false;
};

UCLASS()
class UNullNavSysConfig : public UNavigationSystemConfig {
    GENERATED_BODY()
public:
    explicit UNullNavSysConfig(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

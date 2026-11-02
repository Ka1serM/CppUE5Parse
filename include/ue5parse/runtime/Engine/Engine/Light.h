#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

#include "Light.generated.h"

namespace ue5parse::runtime {

class ULightComponent;
class UPointLightComponent;
class USpotLightComponent;
class URectLightComponent;

// ALight: an actor whose root is its light component (UE's Engine/Light.h).
UCLASS()
class ALight : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<ULightComponent> LightComponent = nullptr;
};

// ADirectionalLight: its light component looks 46 degrees down (UE's Engine/DirectionalLight.h).
UCLASS()
class ADirectionalLight : public ALight {
    GENERATED_BODY()
public:
    explicit ADirectionalLight(const FObjectInitializer& ObjectInitializer);
};

// APointLight (UE's Engine/PointLight.h).
UCLASS()
class APointLight : public ALight {
    GENERATED_BODY()
public:
    explicit APointLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UPointLightComponent> PointLightComponent = nullptr;
};

// ASpotLight: its light component looks straight down (UE's Engine/SpotLight.h).
UCLASS()
class ASpotLight : public ALight {
    GENERATED_BODY()
public:
    explicit ASpotLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USpotLightComponent> SpotLightComponent = nullptr;
};

// ARectLight: its light component looks straight down (UE's Engine/RectLight.h).
UCLASS()
class ARectLight : public ALight {
    GENERATED_BODY()
public:
    explicit ARectLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<URectLightComponent> RectLightComponent = nullptr;
};

}

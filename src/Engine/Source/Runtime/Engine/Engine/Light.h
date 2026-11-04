#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "Light.generated.h"

namespace ue5parse::runtime {

class ULightComponent;
class UPointLightComponent;
class USpotLightComponent;
class URectLightComponent;

UCLASS()
class ALight : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<ULightComponent> LightComponent = nullptr;
};

UCLASS()
class ADirectionalLight : public ALight {
    GENERATED_BODY()
public:
    explicit ADirectionalLight(const FObjectInitializer& ObjectInitializer);
};

UCLASS()
class APointLight : public ALight {
    GENERATED_BODY()
public:
    explicit APointLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UPointLightComponent> PointLightComponent = nullptr;
};

UCLASS()
class ASpotLight : public ALight {
    GENERATED_BODY()
public:
    explicit ASpotLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USpotLightComponent> SpotLightComponent = nullptr;
};

UCLASS()
class ARectLight : public ALight {
    GENERATED_BODY()
public:
    explicit ARectLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<URectLightComponent> RectLightComponent = nullptr;
};

}

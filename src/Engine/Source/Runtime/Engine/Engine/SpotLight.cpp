#include "Engine/Source/Runtime/Engine/Engine/SpotLight.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Engine/Components/SpotLightComponent.h"

namespace ue5parse::runtime {

ASpotLight::ASpotLight(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<USpotLightComponent>(FName("LightComponent0"))) {
    SpotLightComponent = CastChecked<USpotLightComponent>(LightComponent);
    SpotLightComponent->Mobility = EComponentMobility::Stationary;
    RootComponent = SpotLightComponent;
    const EGame Game = GetRuntimeProvider().getGame();
    //! UE 4.9 to 4.23 set the rotation without updating ComponentToWorld.
    if (Game >= EGame::GAME_UE4_9 && Game < EGame::GAME_UE4_24)
        SpotLightComponent->SetRelativeRotation_Direct(FRotator(-90.0f, 0.0f, 0.0f));
    else
        SpotLightComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
}

}

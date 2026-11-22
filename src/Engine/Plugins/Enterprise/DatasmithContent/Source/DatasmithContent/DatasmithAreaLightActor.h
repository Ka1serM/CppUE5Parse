#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "DatasmithAreaLightActor.generated.h"

namespace ue5parse::runtime {

class UTextureLightProfile;

UCLASS()
class ADatasmithAreaLightActor : public AActor {
    GENERATED_BODY()
public:
    explicit ADatasmithAreaLightActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UTextureLightProfile> IESTexture = nullptr;
};

}

#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "Text3DActor.generated.h"

namespace ue5parse::runtime {

class UText3DComponent;

UCLASS()
class AText3DActor : public AActor {
    GENERATED_BODY()
public:
    explicit AText3DActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UText3DComponent> Text3DComponent = nullptr;
};

}

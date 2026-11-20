#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "CameraCaptureComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCameraCaptureComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UCameraCaptureComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

#pragma once

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigControl.h"

#include "ControlRigStaticMeshControl.generated.h"

namespace ue5parse::runtime {

class UStaticMeshComponent;

UCLASS()
class AControlRigStaticMeshControl : public AControlRigControl {
    GENERATED_BODY()
public:
    explicit AControlRigStaticMeshControl(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<USceneComponent> Scene = nullptr;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
};

}

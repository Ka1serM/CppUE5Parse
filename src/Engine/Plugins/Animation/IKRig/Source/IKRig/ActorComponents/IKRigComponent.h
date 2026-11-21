#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "IKRigComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UIKRigComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UIKRigComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

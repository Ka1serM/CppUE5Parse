#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "SmartObjectUserComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USmartObjectUserComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit USmartObjectUserComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

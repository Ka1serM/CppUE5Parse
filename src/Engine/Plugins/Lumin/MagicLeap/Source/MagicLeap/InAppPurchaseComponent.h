#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "InAppPurchaseComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UInAppPurchaseComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UInAppPurchaseComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

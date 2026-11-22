#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "NavigationInvokerComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNavigationInvokerComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UNavigationInvokerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

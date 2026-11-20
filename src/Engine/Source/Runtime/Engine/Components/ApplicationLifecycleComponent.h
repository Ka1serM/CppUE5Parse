#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "ApplicationLifecycleComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UApplicationLifecycleComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UApplicationLifecycleComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

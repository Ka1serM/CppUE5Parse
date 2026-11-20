#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "MediaComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMediaComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UMediaComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "LiveLinkDrivenComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULiveLinkDrivenComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit ULiveLinkDrivenComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

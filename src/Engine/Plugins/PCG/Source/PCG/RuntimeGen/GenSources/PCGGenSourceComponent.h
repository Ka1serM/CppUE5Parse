#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "PCGGenSourceComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPCGGenSourceComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UPCGGenSourceComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

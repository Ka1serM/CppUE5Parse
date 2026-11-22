#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "RaycastComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class URaycastComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit URaycastComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UMagicLeapRaycastComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UMagicLeapRaycastComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

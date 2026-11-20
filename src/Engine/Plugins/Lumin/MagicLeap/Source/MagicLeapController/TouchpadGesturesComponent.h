#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "TouchpadGesturesComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UTouchpadGesturesComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UTouchpadGesturesComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UMagicLeapTouchpadGesturesComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UMagicLeapTouchpadGesturesComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

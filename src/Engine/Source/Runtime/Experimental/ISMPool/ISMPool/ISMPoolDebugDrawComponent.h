#pragma once

#include "Engine/Source/Runtime/Engine/Debug/DebugDrawComponent.h"

#include "ISMPoolDebugDrawComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UISMPoolDebugDrawComponent : public UDebugDrawComponent {
    GENERATED_BODY()
public:
    explicit UISMPoolDebugDrawComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

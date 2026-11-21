#pragma once

#include "Engine/Source/Runtime/Engine/Debug/DebugDrawComponent.h"

#include "SmartObjectDebugRenderingComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USmartObjectDebugRenderingComponent : public UDebugDrawComponent {
    GENERATED_BODY()
public:
    explicit USmartObjectDebugRenderingComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

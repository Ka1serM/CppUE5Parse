#pragma once

#include "Engine/Source/Runtime/Engine/Components/ApplicationLifecycleComponent.h"

#include "LuminApplicationLifecycleComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULuminApplicationLifecycleComponent : public UApplicationLifecycleComponent {
    GENERATED_BODY()
public:
    explicit ULuminApplicationLifecycleComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

#pragma once

#include "Engine/Source/Runtime/NavigationSystem/NavRelevantComponent.h"

#include "NavModifierComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNavModifierComponent : public UNavRelevantComponent {
    GENERATED_BODY()
public:
    explicit UNavModifierComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

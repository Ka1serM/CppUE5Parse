#pragma once

#include "Engine/Source/Runtime/Engine/AI/Navigation/NavLinkDefinition.h"
#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "NavLinkComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNavLinkComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UNavLinkComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) { Links.AddDefaulted(); }

    UPROPERTY()
    TArray<FNavigationLink> Links;
};

}

#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "TextRenderComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UTextRenderComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FText Text;
    UPROPERTY()
    FColor TextRenderColor = FColor::White;

    explicit UTextRenderComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "BrushComponent.generated.h"

namespace ue5parse::runtime {

class UModel;
class UBodySetup;

UCLASS()
class UBrushComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UModel> Brush = nullptr;
    UPROPERTY()
    TObjectPtr<UBodySetup> BrushBodySetup = nullptr;
    UPROPERTY()
    FVector PrePivot_DEPRECATED = FVector::ZeroVector;

    explicit UBrushComponent(const FObjectInitializer& ObjectInitializer);
};

}

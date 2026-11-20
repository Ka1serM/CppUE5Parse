#pragma once

#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Landscape/LandscapeEditTypes.h"

#include "LandscapeLayerInfoObject.generated.h"

namespace ue5parse::runtime {

class UPhysicalMaterial;

UCLASS()
class ULandscapeLayerInfoObject : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName LayerName;
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> PhysMaterial = nullptr;
    UPROPERTY()
    float Hardness = 0.0f;
    UPROPERTY()
    FLinearColor LayerUsageDebugColor;
    UPROPERTY()
    ELandscapeTargetLayerBlendMethod BlendMethod = ELandscapeTargetLayerBlendMethod::None;
    UPROPERTY()
    FName BlendGroup;
};

}

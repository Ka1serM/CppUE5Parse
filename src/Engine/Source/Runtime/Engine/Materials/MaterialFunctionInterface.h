#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "MaterialFunctionInterface.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EMaterialFunctionUsage : uint8 { Default, MaterialLayer, MaterialLayerBlend };

UCLASS(Abstract)
class UMaterialFunctionInterface : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FGuid StateId;
    UPROPERTY()
    EMaterialFunctionUsage MaterialFunctionUsage = EMaterialFunctionUsage::Default;
};

}

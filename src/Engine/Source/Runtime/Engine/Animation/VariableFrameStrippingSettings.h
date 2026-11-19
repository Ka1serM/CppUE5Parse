#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "VariableFrameStrippingSettings.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UVariableFrameStrippingSettings : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPerPlatformBool UseVariableFrameStripping;
    UPROPERTY()
    FPerPlatformInt FrameStrippingRate{3};
};

}

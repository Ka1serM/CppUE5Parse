#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NavCollisionBase.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UNavCollisionBase : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bIsDynamicObstacle = false;
    UPROPERTY()
    bool bUseSurfaceArea = false;
};

}

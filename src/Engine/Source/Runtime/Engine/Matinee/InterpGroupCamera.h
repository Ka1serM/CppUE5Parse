#pragma once

#include "Engine/Source/Runtime/Engine/Matinee/InterpGroup.h"

#include "InterpGroupCamera.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UInterpGroupCamera : public UInterpGroup {
    GENERATED_BODY()
public:
    UPROPERTY()
    float CompressTolerance = 5.0f;
};

}

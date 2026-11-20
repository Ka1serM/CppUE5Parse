#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "VectorField.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UVectorField : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBox Bounds;
    UPROPERTY()
    float Intensity = 1.0f;
};

}

#pragma once

#include "Engine/Source/Runtime/Engine/Engine/ReflectionCapture.h"

#include "PlaneReflectionCapture.generated.h"

namespace ue5parse::runtime {

UCLASS()
class APlaneReflectionCapture : public AReflectionCapture {
    GENERATED_BODY()
public:
    explicit APlaneReflectionCapture(const FObjectInitializer& ObjectInitializer);
};

}

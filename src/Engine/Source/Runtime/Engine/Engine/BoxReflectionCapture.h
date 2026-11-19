#pragma once

#include "Engine/Source/Runtime/Engine/Engine/ReflectionCapture.h"

#include "BoxReflectionCapture.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ABoxReflectionCapture : public AReflectionCapture {
    GENERATED_BODY()
public:
    explicit ABoxReflectionCapture(const FObjectInitializer& ObjectInitializer);
};

}

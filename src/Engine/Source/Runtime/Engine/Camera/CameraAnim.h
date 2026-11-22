#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Engine/Engine/Scene.h"
#include "Engine/Source/Runtime/Engine/Matinee/InterpGroup.h"

#include "CameraAnim.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCameraAnim : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UInterpGroup> CameraInterpGroup = nullptr;
    UPROPERTY()
    float AnimLength = 3.0f;
    UPROPERTY()
    FBox BoundingBox;
    UPROPERTY()
    bool bRelativeToInitialTransform = true;
    UPROPERTY()
    bool bRelativeToInitialFOV = true;
    UPROPERTY()
    float BaseFOV = 90.0f;
    UPROPERTY()
    FPostProcessSettings BasePostProcessSettings;
    UPROPERTY()
    float BasePostProcessBlendWeight = 0;
};

}

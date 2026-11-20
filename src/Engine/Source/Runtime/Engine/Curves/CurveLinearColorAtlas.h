#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"

#include "CurveLinearColorAtlas.generated.h"

namespace ue5parse::runtime {

class UCurveLinearColor;

UCLASS()
class UCurveLinearColorAtlas : public UTexture2D {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint32 TextureSize = 256;
    UPROPERTY()
    uint32 GradientPixelSize = 1;
    UPROPERTY()
    uint32 bSquareResolution : 1 = true;
    UPROPERTY()
    uint32 TextureHeight = 256;
    UPROPERTY()
    TArray<TObjectPtr<UCurveLinearColor>> GradientCurves;
};

}

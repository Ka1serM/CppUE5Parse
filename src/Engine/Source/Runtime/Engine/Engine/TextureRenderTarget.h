#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Texture.h"

#include "TextureRenderTarget.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UTextureRenderTarget : public UTexture {
    GENERATED_BODY()
public:
    explicit UTextureRenderTarget(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float TargetGamma = 0.0f;
};

}

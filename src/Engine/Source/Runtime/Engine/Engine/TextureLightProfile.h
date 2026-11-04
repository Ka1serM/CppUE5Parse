#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"

#include "TextureLightProfile.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UTextureLightProfile : public UTexture2D {
    GENERATED_BODY()
public:
    UPROPERTY()
    float Brightness = -1;
    UPROPERTY()
    float TextureMultiplier = 0;
};

}

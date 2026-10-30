#pragma once

#include "ue5parse/runtime/Engine/Engine/Texture2D.h"

#include "TextureLightProfile.generated.h"

namespace ue5parse::runtime {

// UTextureLightProfile: an IES light profile, with the brightness it was measured at (UE's
// Engine/TextureLightProfile.h).
UCLASS()
class UTextureLightProfile : public UTexture2D {
    GENERATED_BODY()
public:
    // In lumens; -1 when the profile has none.
    UPROPERTY()
    float Brightness = -1;
    UPROPERTY()
    // UE's constructor leaves it as the zeroed memory of a new object.
    float TextureMultiplier = 0;
};

}

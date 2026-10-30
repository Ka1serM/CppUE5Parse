#pragma once

#include "ue5parse/runtime/Engine/Engine/Texture2D.h"

#include "LightMapTexture2D.generated.h"

namespace ue5parse::runtime {

enum ELightMapFlags : uint32 { LMF_None = 0, LMF_Streamed = 0x00000001 };

// ULightMapTexture2D: a texture of baked lighting, with its flags after the texture data.
UCLASS()
class ULightMapTexture2D : public UTexture2D {
    GENERATED_BODY()
public:
    uint32 LightmapFlags = LMF_None;
    void Serialize(FArchive& Ar) override;
};

}

#pragma once

#include "ue5parse/runtime/Engine/Engine/Texture.h"

#include "TextureCube.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UTextureCube : public UTexture {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;
    int32 GetSizeX() const { return PlatformData.SizeX; }
    int32 GetSizeY() const { return PlatformData.SizeY; }
};

}

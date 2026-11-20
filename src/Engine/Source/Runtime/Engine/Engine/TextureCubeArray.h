#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Texture.h"

#include "TextureCubeArray.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UTextureCubeArray : public UTexture {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;
};

}

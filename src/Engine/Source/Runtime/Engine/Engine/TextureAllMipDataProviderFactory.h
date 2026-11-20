#pragma once

#include "Engine/Source/Runtime/Engine/Engine/TextureMipDataProviderFactory.h"

#include "TextureAllMipDataProviderFactory.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UTextureAllMipDataProviderFactory : public UTextureMipDataProviderFactory {
    GENERATED_BODY()
};

}

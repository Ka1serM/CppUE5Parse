#pragma once

#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"

#include "TextureMipDataProviderFactory.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UTextureMipDataProviderFactory : public UAssetUserData {
    GENERATED_BODY()
};

}

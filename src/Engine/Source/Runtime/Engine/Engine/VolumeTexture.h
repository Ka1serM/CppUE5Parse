#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Texture.h"

#include "VolumeTexture.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UVolumeTexture : public UTexture {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<TextureAddress> AddressMode = TA_Wrap;

    void Serialize(FArchive& Ar) override;
};

}

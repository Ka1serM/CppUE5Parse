#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Texture.h"

#include "Texture2DArray.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UTexture2DArray : public UTexture {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<TextureAddress> AddressX = TA_Wrap;
    UPROPERTY()
    TEnumAsByte<TextureAddress> AddressY = TA_Wrap;
    UPROPERTY()
    TEnumAsByte<TextureAddress> AddressZ = TA_Wrap;

    void Serialize(FArchive& Ar) override;
};

}

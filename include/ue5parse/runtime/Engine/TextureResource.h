#pragma once

#include "ue5parse/runtime/CoreUObject/Serialization/BulkData.h"

// The mips of a texture (UE's TextureResource.h).

namespace ue5parse::runtime {

// FTexture2DMipMap: one mip's payload and size.
struct FTexture2DMipMap {
    FByteBulkData BulkData;
    int32 SizeX = 0;
    int32 SizeY = 0;
    int32 SizeZ = 1;

    // Reads the mip; the payload only when bSerializeMipData (UE's FTexture2DMipMap::Serialize).
    void Serialize(FArchive& Ar, bool bSerializeMipData);
};

}

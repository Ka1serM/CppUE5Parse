#pragma once

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"

namespace ue5parse::runtime {

struct FTexture2DMipMap {
    FByteBulkData BulkData;
    int32 SizeX = 0;
    int32 SizeY = 0;
    int32 SizeZ = 1;

    void Serialize(FArchive& Ar, bool bSerializeMipData);
};

}

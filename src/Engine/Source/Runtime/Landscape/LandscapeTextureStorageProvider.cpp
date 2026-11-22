#include "Engine/Source/Runtime/Landscape/LandscapeTextureStorageProvider.h"

#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"

namespace ue5parse::runtime {

void FLandscapeTexture2DMipMap::Serialize(FArchive& Ar) {
    Ar << SizeX << SizeY << bCompressed;
    BulkData.Serialize(Ar);
}

void ULandscapeTextureStorageProviderFactory::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    Ar << NumNonOptionalMips << NumNonStreamingMips << LandscapeGridScale;
    const int32 NumMips = Ar.ReadCount(1);
    Mips.SetNum(NumMips);
    for (int32 Index = 0; Index < NumMips && !Ar.IsError(); Index++)
        Mips[Index].Serialize(Ar);
    Ar << Texture;
}

}

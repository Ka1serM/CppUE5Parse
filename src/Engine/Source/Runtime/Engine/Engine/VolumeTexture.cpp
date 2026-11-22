#include "Engine/Source/Runtime/Engine/Engine/VolumeTexture.h"

#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

void UVolumeTexture::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    FStripDataFlags StripFlags(Ar);
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked)
        SerializeCookedPlatformData(Ar, true);
}

}

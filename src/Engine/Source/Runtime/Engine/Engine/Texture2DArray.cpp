#include "Engine/Source/Runtime/Engine/Engine/Texture2DArray.h"

#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

void UTexture2DArray::Serialize(FArchive& Ar) {
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

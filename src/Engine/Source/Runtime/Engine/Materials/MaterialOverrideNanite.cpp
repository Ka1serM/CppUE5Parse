#include "Engine/Source/Runtime/Engine/Materials/MaterialOverrideNanite.h"

#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"

namespace ue5parse::runtime {

bool FMaterialOverrideNanite::Serialize(FArchive& Ar) {
    bool bSerializeAsCookedData = false;
    Ar << bSerializeAsCookedData;
    if (bSerializeAsCookedData) {
        UObject* Material = nullptr;
        Ar << Material;
        OverrideMaterial = Cast<UMaterialInterface>(Material);
    }
    return false;
}

}

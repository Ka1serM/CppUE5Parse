#include "ue5parse/runtime/Engine/Materials/MaterialOverrideNanite.h"

#include "ue5parse/runtime/Engine/Materials/MaterialInterface.h"

// UE's Materials/MaterialOverrideNanite.cpp (UE 5.3).

namespace ue5parse::runtime {

bool FMaterialOverrideNanite::Serialize(FArchive& Ar) {
    bool bSerializeAsCookedData = false;
    Ar << bSerializeAsCookedData;
    if (bSerializeAsCookedData) {
        UObject* Material = nullptr;
        Ar << Material;
        OverrideMaterial = Cast<UMaterialInterface>(Material);
    }
    // The properties follow.
    return false;
}

}

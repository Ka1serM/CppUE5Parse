#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"
#include "ue5parse/runtime/CoreUObject/UObject/SoftObjectPtrTemplates.h"

#include "MaterialOverrideNanite.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

// FMaterialOverrideNanite: the material Nanite draws a mesh with instead (UE's Materials/MaterialOverrideNanite.h).
// Cooked data writes the material natively, then the properties.
USTRUCT()
struct FMaterialOverrideNanite {
    GENERATED_BODY()
    UPROPERTY()
    TSoftObjectPtr<UMaterialInterface> OverrideMaterialRef;
    UPROPERTY()
    bool bEnableOverride = false;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> OverrideMaterial = nullptr;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMaterialOverrideNanite> : TStructOpsTypeTraitsBase2<FMaterialOverrideNanite> {
    enum { WithSerializer = true };
};

}

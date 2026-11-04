#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtrTemplates.h"

#include "MaterialOverrideNanite.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

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

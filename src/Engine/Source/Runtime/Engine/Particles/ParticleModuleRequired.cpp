#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"

namespace ue5parse::runtime {

UParticleModuleRequired::UParticleModuleRequired(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    LODDuplicate = true;
}

void UParticleModuleRequired::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || HasAnyFlags(RF_ClassDefaultObject))
        return;
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked)
        Ar << DerivedData.BoundingGeometry;
}

void UParticleModuleRequired::PostLoad() {
    Super::PostLoad();
    SubImages_Horizontal = FMath::Max(SubImages_Horizontal, 1);
    SubImages_Vertical = FMath::Max(SubImages_Vertical, 1);
}

}

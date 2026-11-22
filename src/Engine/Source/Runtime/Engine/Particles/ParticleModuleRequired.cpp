#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

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
    if (!Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::MovedParticleCutoutsToRequiredModule, EGame::GAME_UE4_14))
        return;
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked)
        DerivedData.Serialize(Ar);
}

void UParticleModuleRequired::PostLoad() {
    Super::PostLoad();
    SubImages_Horizontal = FMath::Max(SubImages_Horizontal, 1);
    SubImages_Vertical = FMath::Max(SubImages_Vertical, 1);
}

}

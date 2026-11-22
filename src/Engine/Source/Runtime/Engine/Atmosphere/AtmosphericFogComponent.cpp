#include "Engine/Source/Runtime/Engine/Atmosphere/AtmosphericFogComponent.h"

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"

namespace ue5parse::runtime {

void UAtmosphericFogComponent::Serialize(FArchive& Ar) {
    if (Ar.UEVer() >= EUnrealEngineObjectUE5Version::INITIAL_VERSION) {
        Super::Serialize(Ar);
        return;
    }
    //! UE 4 derives the component from USceneComponent and stores the precomputed atmosphere after its properties.
    USceneComponent::Serialize(Ar);
    FByteBulkData TransmittanceData;
    FByteBulkData IrradianceData;
    FByteBulkData InscatterData;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::ATMOSPHERIC_FOG_CACHE_DATA) {
        TransmittanceData.Serialize(Ar);
        IrradianceData.Serialize(Ar);
    }
    InscatterData.Serialize(Ar);
    int32 PrecomputeCounter = 0;
    Ar << PrecomputeCounter;
}

}

#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionObject.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

bool HasLegacyNaniteData(const FArchive& Ar) {
    const int32 Version = Ar.CustomVer(FUE5MainStreamObjectVersion::GUID);
    return Version == FUE5MainStreamObjectVersion::GeometryCollectionNaniteData
        || (Version >= FUE5MainStreamObjectVersion::GeometryCollectionNaniteCooked && Version < FUE5MainStreamObjectVersion::GeometryCollectionNaniteTransient);
}

}

void UGeometryCollection::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    if (!Ar.CustomVerAtLeast(FDestructionObjectVersion::GUID, FDestructionObjectVersion::DensityUnitsChanged, EGame::GAME_UE4_23) && bMassAsDensity)
        Mass *= 1000000.0f;
    bool bIsCookedOrCooking = false;
    if (Ar.CustomVerAtLeast(FDestructionObjectVersion::GUID, FDestructionObjectVersion::GeometryCollectionInDDC, EGame::GAME_UE4_23))
        Ar << bIsCookedOrCooking;
    Chaos::FChaosArchive ChaosAr(Ar);
    if (Ar.CustomVerAtLeast(FDestructionObjectVersion::GUID, FDestructionObjectVersion::GeometryCollectionInDDCAndAsset, EGame::GAME_UE4_23))
        GeometryCollection.Serialize(ChaosAr);
    if (ChaosAr.IsError()) {
        ChaosAr.ReportErrorTo(Ar);
        return;
    }
    if (HasLegacyNaniteData(Ar)) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "UGeometryCollection.SerializeOldNaniteData");
        return;
    }
    if (Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::GeometryCollectionNaniteTransient, EGame::GAME_UE5_0)) {
        bool bCooked = false;
        Ar << bCooked;
        if (bCooked) {
            RenderData = std::make_unique<FGeometryCollectionRenderData>();
            if (Ar.GetGame() >= EGame::GAME_UE5_3)
                RenderData->Serialize(Ar);
            else
                Ar.SetError(ErrorCode::UnsupportedVersion, "FGeometryCollectionNaniteData");
        }
    }
    if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::GeometryCollectionPerChildDamageThreshold, EGame::GAME_UE5_1))
        PerClusterOnlyDamageThreshold = true;
    if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::GeometryCollectionDamagePropagationData, EGame::GAME_UE5_1))
        DamagePropagationData.bEnabled = false;
    if (!Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::GeometryCollectionConvertVertexColorToSRGB, EGame::GAME_UE5_3))
        bConvertVertexColorsToSRGB = false;
}

}

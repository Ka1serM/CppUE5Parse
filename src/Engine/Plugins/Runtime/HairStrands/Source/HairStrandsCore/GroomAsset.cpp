#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomAsset.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

namespace {

constexpr uint8 CDSF_ImportedStrands = 1;

void SerializeCardsLOD(FArchive& Ar, FHairGroupPlatformData::FCardsLOD& LOD) {
    LOD.BulkData.Serialize(Ar);
    LOD.InterpolationBulkData.Serialize(Ar);
    LOD.GuideBulkData.Serialize(Ar);
    LOD.GuideInterpolationBulkData.Serialize(Ar);
}

void SerializeCardsAndMeshes(FArchive& Ar, FHairGroupPlatformData& GroupData) {
    if (!Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::SerializeGroomCardsAndMeshes, EGame::GAME_UE4_26))
        return;
    bool bIsCooked = false;
    Ar << bIsCooked;
    if (!bIsCooked)
        return;
    GroupData.CardsLODs.SetNum(static_cast<int32>(Ar.ReadCount(1)));
    for (FHairGroupPlatformData::FCardsLOD& LOD : GroupData.CardsLODs)
        SerializeCardsLOD(Ar, LOD);
    GroupData.MeshesLODs.SetNum(static_cast<int32>(Ar.ReadCount(1)));
    for (FHairGroupPlatformData::FMeshesLOD& LOD : GroupData.MeshesLODs)
        LOD.BulkData.Serialize(Ar);
}

void SerializeGroupData(FArchive& Ar, FHairGroupPlatformData& GroupData) {
    GroupData.StrandsBulkData.Serialize(Ar);
    GroupData.GuidesBulkData.Serialize(Ar);
    GroupData.StrandsInterpolationBulkData.Serialize(Ar);
    if (!Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::SerializeHairClusterCullingData, EGame::GAME_UE4_26))
        return;
    GroupData.StrandsClusterBulkData.Serialize(Ar);
    SerializeCardsAndMeshes(Ar, GroupData);
}

//! The render curve and Nanite groom data are only serialized when r.RenderCurve or r.HairStrands.Nanite are enabled, both read-only and off by default.
void SerializePlatformData(FArchive& Ar, FHairGroupPlatformData& GroupData) {
    GroupData.GuidesBulkData.Serialize(Ar);
    GroupData.StrandsBulkData.Serialize(Ar);
    GroupData.StrandsInterpolationBulkData.Serialize(Ar);
    if (Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::SerializeHairClusterCullingData, EGame::GAME_UE4_26))
        GroupData.StrandsClusterBulkData.Serialize(Ar);
    SerializeCardsAndMeshes(Ar, GroupData);
}

}

void UGroomAsset::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::GroomAssetWidthOverride, EGame::GAME_UE5_3))
        for (FHairGroupsRendering& Group : HairGroupsRendering)
            Group.GeometrySettings.HairWidth_Override = true;
    if (!Ar.CustomVerAtLeast(FPhysicsObjectVersion::GUID, FPhysicsObjectVersion::GroomWithDescription, EGame::GAME_UE4_25))
        return;
    FStripDataFlags StripFlags(Ar);
    bool bHasImportedStrands = true;
    if (Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::GroomLODStripping, EGame::GAME_UE4_26)) {
        uint8 ClassDataStripFlags = 0;
        Ar << ClassDataStripFlags;
        bHasImportedStrands = (ClassDataStripFlags & CDSF_ImportedStrands) != 0;
    }
    if (Ar.GetGame() < EGame::GAME_UE5_3) {
        if (!StripFlags.IsEditorDataStripped() && bHasImportedStrands)
            return;
        HairGroupsPlatformData.SetNum(static_cast<int32>(Ar.ReadCount(1)));
        for (FHairGroupPlatformData& GroupData : HairGroupsPlatformData)
            SerializeGroupData(Ar, GroupData);
        return;
    }
    if (!StripFlags.IsEditorDataStripped())
        return;
    HairGroupsPlatformData.SetNum(static_cast<int32>(Ar.ReadCount(1)));
    for (FHairGroupPlatformData& GroupData : HairGroupsPlatformData)
        SerializePlatformData(Ar, GroupData);
}

}

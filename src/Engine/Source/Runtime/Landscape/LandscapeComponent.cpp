#include "Engine/Source/Runtime/Landscape/LandscapeComponent.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/LightMap.h"
#include "Engine/Source/Runtime/Engine/ShadowMap.h"

namespace ue5parse::runtime {

//! Before UE 4.12 the grass data stores its material state id and WPO rotation also in cooked packages.
static void SerializeUnfilteredGrassStateIds(FArchive& Ar) {
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::SERIALIZE_LANDSCAPE_GRASS_DATA_MATERIAL_GUID) {
        FGuid MaterialStateId;
        Ar << MaterialStateId;
    }
    if (!Ar.CustomVerAtLeast(FLandscapeCustomVersion::GUID, FLandscapeCustomVersion::GrassMaterialWPO, EGame::GAME_UE4_11))
        return;
    FQuat RotationForWPO;
    Ar << RotationForWPO;
}

FArchive& operator<<(FArchive& Ar, FLandscapeComponentGrassData& Data) {
    if (Ar.GetGame() < EGame::GAME_UE4_12)
        SerializeUnfilteredGrassStateIds(Ar);
    if (!Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::LandscapeGrassSingleArray, EGame::GAME_UE5_0)) {
        BulkSerialize(Ar, Data.HeightData);
        return Ar << Data.WeightData;
    }
    Ar << Data.NumElements;
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::OverridableLandscapeGrassTypes, EGame::GAME_UE5_8))
        Ar << Data.WeightOffsets;
    else
        Ar << Data.GrassTypeWeightOffsets;
    return Ar << Data.HeightWeightData;
}

void FLandscapeComponentDerivedData::Serialize(FArchive& Ar) {
    Ar << CompressedLandscapeData;
    if (Ar.GetGame() < EGame::GAME_UE4_26)
        return;
    const int32 NumStreamingLODs = Ar.ReadCount(1);
    StreamingLODDataArray.SetNum(NumStreamingLODs);
    for (int32 Index = 0; Index < NumStreamingLODs && !Ar.IsError(); Index++)
        StreamingLODDataArray[Index].Serialize(Ar);
}

ULandscapeComponent::ULandscapeComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    BodyInstance.CollisionProfileName = FName("NoCollision");
    bGenerateOverlapEvents = false;
    bUseAsOccluder = true;
    bAllowCullDistanceVolume = false;
    MaterialInstances.AddDefaulted();
    LODIndexToMaterialIndex.AddDefaulted();
    Mobility = EComponentMobility::Static;
    AlwaysLoadOnServer = false;
    TranslucencySortPriority = -1;
    LODStreamingProxy = ObjectInitializer.CreateDefaultSubobject<ULandscapeLODStreamingProxy>(FName("LandscapeLODStreamingProxy"));
}

void ULandscapeComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    if (!Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::MapBuildDataSeparatePackage, EGame::GAME_UE4_14)) {
        FLightMapRef LightMap;
        FShadowMapRef ShadowMap;
        Ar << LightMap << ShadowMap;
    }
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::SERIALIZE_LANDSCAPE_GRASS_DATA)
        Ar << GrassData;
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::LANDSCAPE_PLATFORMDATA_COOKING || HasAnyFlags(RF_ClassDefaultObject))
        return;
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked)
        SerializeCookedPlatformData(Ar);
}

//! Before UE 4.13 only a cook for a platform without vertex texture fetch writes the mobile platform data, with no flag for it.
void ULandscapeComponent::SerializeCookedPlatformData(FArchive& Ar) {
    if (Ar.GetGame() >= EGame::GAME_UE5_1)
        return;
    if (Ar.GetGame() >= EGame::GAME_UE4_13) {
        bool bCookedMobileData = false;
        Ar << bCookedMobileData;
        if (bCookedMobileData)
            PlatformData.Serialize(Ar);
        return;
    }
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::LANDSCAPE_GRASS_COOKING || Ar.UEVer() >= EUnrealEngineObjectUE4Version::SERIALIZE_LANDSCAPE_GRASS_DATA)
        return;
    int32 NumChannels = 0;
    Ar << NumChannels;
    if (NumChannels == 0)
        return;
    TArray<uint8> OldGrassData;
    BulkSerialize(Ar, OldGrassData);
}

}

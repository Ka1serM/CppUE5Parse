#include "Engine/Source/Runtime/Landscape/LandscapeComponent.h"

#include <cstring>
#include <vector>

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/Engine/LightMap.h"
#include "Engine/Source/Runtime/Engine/ShadowMap.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

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

//! Honor of Kings: World stores its own grass layout.
static void SkipHonorofKingsWorldGrassData(FArchive& Ar, FLandscapeComponentGrassData& Data) {
    Ar << Data.NumElements;
    const int32 Count = Ar.ReadCount(8);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        UObject* GrassType = nullptr;
        Ar << GrassType;
        const int32 ElementCount = Ar.ReadCount(12);
        for (int32 j = 0; j < ElementCount && !Ar.IsError(); j++) {
            int32 Type = 0;
            int32 Unknown = 0;
            int32 Length = 0;
            Ar << Type << Unknown << Length;
            Ar.SkipBytes(static_cast<int64>(Length) * 27 + (Type == 1 ? 8 : 20));
        }
    }
}

//! PUBG stores the grass height data in a bulk data and only the grass types of the weight data.
static void SerializePlayerUnknownsBattlegroundsGrassData(FArchive& Ar, FLandscapeComponentGrassData& Data) {
    FByteBulkData HeightBulkData;
    HeightBulkData.Serialize(Ar);
    if (Ar.IsError())
        return;
    if (HeightBulkData.GetElementCount() > 0) {
        Result<std::vector<std::byte>> Bytes = HeightBulkData.Read();
        if (!Bytes) {
            Ar.SetError(ErrorCode::InvalidData, "FLandscapeComponentGrassData.HeightData");
            return;
        }
        Data.HeightData.SetNum(static_cast<int32>(Bytes->size() >> 1));
        std::memcpy(Data.HeightData.GetData(), Bytes->data(), static_cast<size_t>(Data.HeightData.Num()) * sizeof(uint16));
    }
    const int32 Count = Ar.ReadCount(4);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        TObjectPtr<ULandscapeGrassType> GrassType = nullptr;
        Ar << GrassType;
        Data.WeightData.Add(GrassType);
    }
}

FArchive& operator<<(FArchive& Ar, FLandscapeComponentGrassData& Data) {
    if (Ar.GetGame() < EGame::GAME_UE4_12)
        SerializeUnfilteredGrassStateIds(Ar);
    if (Ar.GetGame() == EGame::GAME_HonorofKingsWorld) {
        SkipHonorofKingsWorldGrassData(Ar, Data);
        return Ar;
    }
    if (Ar.GetGame() == EGame::GAME_PlayerUnknownsBattlegrounds) {
        SerializePlayerUnknownsBattlegroundsGrassData(Ar, Data);
        return Ar;
    }
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

//! Aion 2 stores two more bulk arrays instead of the mobile platform data.
static void SkipAion2Data(FArchive& Ar) {
    bool bCookedMobileData = false;
    Ar << bCookedMobileData;
    SkipBulkSerializedArray(Ar);
    SkipBulkSerializedArray(Ar);
}

void ULandscapeComponent::Serialize(FArchive& Ar) {
    if (Ar.GetGame() == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(20);
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
    if (Ar.GetGame() == EGame::GAME_Farlight84)
        Ar.SkipBytes(32);
    bool bCooked = false;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::LANDSCAPE_PLATFORMDATA_COOKING && !HasAnyFlags(RF_ClassDefaultObject))
        Ar << bCooked;
    if (Ar.GetGame() == EGame::GAME_Aion2) {
        SkipAion2Data(Ar);
        return;
    }
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

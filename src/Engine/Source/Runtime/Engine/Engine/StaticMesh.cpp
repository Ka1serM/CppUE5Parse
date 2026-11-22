#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"

#include <vector>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/Engine/AI/Navigation/NavCollisionBase.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"

namespace ue5parse::runtime {

namespace {

using V4 = EUnrealEngineObjectUE4Version;

constexpr uint8 CDSF_AdjacencyData = 1;
constexpr uint8 CDSF_MinLodData = 2;
constexpr uint8 CDSF_ReversedIndexBuffer = 4;
constexpr uint8 CDSF_RayTracingResources = 8;
constexpr uint8 CDSF_StripIndexBuffers = 128 | 64 | 32;
constexpr int32 RayTracingRawDataHeaderSize = 6 * 4;
constexpr int32 MaxStreamingTexCoords = 8;
constexpr int32 DistanceFieldMips = 3;

bool HasTessellation(const FArchive& Ar) {
    return !Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::RemovingTessellation, EGame::GAME_UE5_0);
}

void SkipArray(FArchive& Ar, int32 ElementSize) {
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(ElementSize)) * ElementSize);
}

void SkipBulkArrays(FArchive& Ar, int32 Count) {
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++) {
        int32 ElementSize = 0;
        Ar << ElementSize;
        if (ElementSize <= 0) {
            Ar.SetError(ErrorCode::InvalidData, "SkipBulkArrays.ElementSize");
            return;
        }
        SkipArray(Ar, ElementSize);
    }
}

void SkipCardRepresentation(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    if (Game == EGame::GAME_Highguard) {
        FBox3f Bounds;
        Ar << Bounds;
    } else {
        FBox Bounds;
        Ar << Bounds;
    }
    if (Game < EGame::GAME_UE5_1 || Game == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(4);
    if (Ar.GetGame() >= EGame::GAME_UE5_2) {
        bool bMostlyTwoSided = false;
        Ar << bMostlyTwoSided;
    }
    SkipArray(Ar, Ar.GetGame() < EGame::GAME_UE5_1 ? 62 : 61);
}

//! Before 5.4 the distance field volume stores its bounds and mip transforms as large world doubles.
void SkipDistanceField5(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    const bool bFloatBounds = Game >= EGame::GAME_UE5_4 || Game == EGame::GAME_Highguard;
    const bool bTwoMips = Game == EGame::GAME_TheFinals || Game == EGame::GAME_ArcRaiders;
    if (bFloatBounds) {
        FBox3f Bounds;
        Ar << Bounds;
    } else {
        FBox Bounds;
        Ar << Bounds;
    }
    bool bMostlyTwoSided = false;
    Ar << bMostlyTwoSided;
    const int64 MipSize = 12 + 4 + (bFloatBounds ? 12 + 12 + 8 : 24 + 24 + 16) + 4 + 4;
    Ar.SkipBytes((bTwoMips ? 2 : DistanceFieldMips) * MipSize);
    SkipArray(Ar, 1);
    if (bTwoMips)
        Ar.SkipBytes(6);
    FByteBulkData Streamable;
    Streamable.Serialize(Ar);
}

void SkipDistanceField4(FArchive& Ar) {
    const bool bCompressed = Ar.GetGame() >= EGame::GAME_UE4_16;
    SkipArray(Ar, bCompressed ? 1 : 2);
    FBox3f Bounds;
    Ar.SkipBytes(12);
    Ar << Bounds;
    if (bCompressed)
        Ar.SkipBytes(8);
    bool bFlag = false;
    Ar << bFlag;
    if (Ar.UEVer() >= V4::RENAME_CROUCHMOVESCHARACTERDOWN)
        Ar << bFlag;
    if (Ar.UEVer() >= V4::DEPRECATE_UMG_STYLE_ASSETS && !(bCompressed && Ar.GetGame() == EGame::GAME_FragPunk))
        Ar << bFlag;
    if (bCompressed && Ar.GetGame() == EGame::GAME_Psychonauts2)
        Ar.SkipBytes(22);
}

void SkipNaniteHierarchyNodes(FArchive& Ar) {
    constexpr int32 Fanout = 4;
    const EGame Game = Ar.GetGame();
    const int32 Misc2Size = Game >= EGame::GAME_UE5_8 ? 12 : Game >= EGame::GAME_UE5_7 ? 8 : 4;
    SkipArray(Ar, Fanout * (16 + 12 + 4 + 12 + 4 + Misc2Size));
}

void SkipStaticMeshRayTracingProxy(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    bool bUsingRenderingLODs = false;
    Ar << bUsingRenderingLODs;
    if (StripFlags.IsAudioVisualDataStripped())
        return;
    const int32 LODCount = Ar.ReadCount(8);
    for (int32 i = 0; i < LODCount && !Ar.IsError(); i++) {
        bool bOwnsBuffers = false;
        Ar << bOwnsBuffers;
        if (bOwnsBuffers) {
            TArray<FStaticMeshSection> Sections;
            Ar << Sections;
        }
        bool bOwnsRayTracingGeometry = false;
        Ar << bOwnsRayTracingGeometry;
        bool bBuffersInlined = false;
        if (Ar.GetGame() >= EGame::GAME_UE5_6)
            Ar << bBuffersInlined;
        if (bBuffersInlined) {
            Ar.SetError(ErrorCode::UnsupportedVersion, "FStaticMeshRayTracingProxyLOD.Inlined");
            return;
        }
        FByteBulkData Streamable;
        Streamable.Serialize(Ar);
        if (Ar.GetGame() < EGame::GAME_UE5_6)
            continue;
        if (bOwnsBuffers)
            Ar.SkipBytes(4 + 8 + 16 + 8 + 8);
        if (bOwnsRayTracingGeometry)
            Ar.SkipBytes(8 + RayTracingRawDataHeaderSize);
    }
}

}

void SkipWeightedRandomSampler(FArchive& Ar) {
    SkipArray(Ar, 4);
    SkipArray(Ar, 4);
    Ar.SkipBytes(4);
}

void SkipBulkSerializedArray(FArchive& Ar) {
    int32 ElementSize = 0, Count = 0;
    Ar << ElementSize << Count;
    if (ElementSize < 0 || Count < 0) {
        Ar.SetError(ErrorCode::InvalidData, "SkipBulkSerializedArray");
        return;
    }
    Ar.SkipBytes(static_cast<int64>(ElementSize) * Count);
}

void FNaniteResources::Serialize(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsAudioVisualDataStripped())
        return;
    const EGame Game = Ar.GetGame();
    Ar.SkipBytes(4);
    FByteBulkData StreamablePages;
    StreamablePages.Serialize(Ar);
    RootDataSize = Ar.ReadCount(1);
    Ar.SkipBytes(RootDataSize);
    NumPageStreamingStates = Ar.ReadCount(20);
    Ar.SkipBytes(static_cast<int64>(NumPageStreamingStates) * (Game >= EGame::GAME_UE5_3 ? 20 : 24));
    SkipNaniteHierarchyNodes(Ar);
    SkipArray(Ar, 4);
    SkipArray(Ar, Game >= EGame::GAME_UE5_7 ? 2 : 4);
    if (Game >= EGame::GAME_UE5_6) {
        SkipArray(Ar, 48);
        if (Game >= EGame::GAME_UE5_7) {
            SkipArray(Ar, 4);
            if (Game >= EGame::GAME_UE5_8)
                SkipArray(Ar, 4);
            SkipArray(Ar, 4);
        }
        Ar.SkipBytes(12 + 12 + 4);
    }
    if (Game < EGame::GAME_UE5_8)
        SkipArray(Ar, 2);
    Ar.SkipBytes(4 + 4);
    if (Game >= EGame::GAME_UE5_2)
        Ar.SkipBytes(4);
    Ar << NumInputTriangles << NumInputVertices;
    if (Game < EGame::GAME_UE5_6)
        Ar.SkipBytes(2 + 2);
    if (Game >= EGame::GAME_UE5_1)
        Ar.SkipBytes(4);
    if (Game >= EGame::GAME_UE5_7)
        Ar.SkipBytes(8);
    if (Game >= EGame::GAME_UE5_8)
        Ar.SkipBytes(4);
}

FArchive& operator<<(FArchive& Ar, FStaticMeshSection& Section) {
    const EGame Game = Ar.GetGame();
    Ar << Section.MaterialIndex << Section.FirstIndex << Section.NumTriangles << Section.MinVertexIndex << Section.MaxVertexIndex;
    Ar << Section.bEnableCollision << Section.bCastShadow;
    if (Game == EGame::GAME_PlayerUnknownsBattlegrounds)
        Ar.SkipBytes(5);
    if (Game == EGame::GAME_AssaultFireFuture)
        return Ar;
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::StaticMeshSectionForceOpaqueField, EGame::GAME_UE4_25))
        Ar << Section.bForceOpaque;
    if (Game == EGame::GAME_MortalKombat1 || Game == EGame::GAME_TheFinals || Game == EGame::GAME_ArcRaiders)
        Ar.SkipBytes(8);
    if (Game == EGame::GAME_BlueProtocol) {
        int16 Data = 0;
        Ar << Data;
        Section.CustomData = Data;
    }
    if (Game >= EGame::GAME_UE4_26)
        Ar << Section.bVisibleInRayTracing;
    if (Game == EGame::GAME_Grounded || Game == EGame::GAME_Dauntless)
        Ar.SkipBytes(8);
    if (Game >= EGame::GAME_UE5_1)
        Ar << Section.bAffectDistanceFieldLighting;
    switch (Game) {
    case EGame::GAME_RogueCompany:
    case EGame::GAME_Grounded:
    case EGame::GAME_Grounded2:
    case EGame::GAME_RacingMaster:
    case EGame::GAME_WutheringWaves:
    case EGame::GAME_MetroAwakening:
    case EGame::GAME_Avowed:
    case EGame::GAME_OutlastTrials:
    case EGame::GAME_OuterWorlds2:
    case EGame::GAME_LiesofP:
        Ar.SkipBytes(4);
        break;
    case EGame::GAME_InfinityNikki: {
        int32 Data = 0;
        Ar << Data;
        Section.CustomData = Data;
        Ar.SkipBytes(8);
        break;
    }
    default:
        break;
    }
    return Ar;
}

namespace {

//! Index buffers that games add after the inlined buffers of a LOD.
void SkipInlinedGameData(FArchive& Ar, TArray<FStaticMeshSection>& Sections) {
    FRawStaticIndexBuffer Unused;
    switch (Ar.GetGame()) {
    case EGame::GAME_RogueCompany:
        Ar.SkipBytes(10);
        return;
    case EGame::GAME_TheDivisionResurgence:
        Ar.SkipBytes(12);
        return;
    case EGame::GAME_PUBGBlackBudget: {
        Ar.SkipBytes(4);
        int32 Count = 0;
        Ar << Count;
        if (Count > 0)
            SkipBulkSerializedArray(Ar);
        return;
    }
    case EGame::GAME_HonorofKingsWorld: {
        Unused.Serialize(Ar);
        Unused.Serialize(Ar);
        Ar.SkipBytes(4);
        FRawStaticIndexBuffer First;
        First.Serialize(Ar);
        for (int32 i = 0; i < 3; i++)
            Unused.Serialize(Ar);
        if (First.GetNumIndices() > 0 && Sections.Num() > 0)
            Sections[0].CustomData = 1;
        return;
    }
    case EGame::GAME_InfinityNikki:
        for (int32 i = 0; i < Sections.Num(); i++) {
            if (Sections[i].CustomData != 1)
                continue;
            for (int32 j = 0; j < 4; j++)
                Unused.Serialize(Ar);
            return;
        }
        return;
    default:
        return;
    }
}

void SkipLegacySamplers(FArchive& Ar, int32 SectionCount) {
    if (Ar.GetGame() < EGame::GAME_UE4_16)
        return;
    for (int32 i = 0; i <= SectionCount; i++)
        SkipWeightedRandomSampler(Ar);
}

bool HasHonorofKingsWorldSectionData(const TArray<FStaticMeshLODResources>& LODs) {
    for (int32 i = 0; i < LODs.Num(); i++)
        if (LODs[i].Sections.Num() > 0 && LODs[i].Sections[0].CustomData == 1)
            return true;
    return false;
}

//! Bytes that games add after the sizes of a LOD that is not inlined.
int64 GetStreamedGameDataSize(EGame Game) {
    switch (Game) {
    case EGame::GAME_SuicideSquad:
        return 29;
    case EGame::GAME_ArenaBreakoutInfinite:
        return 16;
    case EGame::GAME_TheFinals:
    case EGame::GAME_ArcRaiders:
        return 12;
    case EGame::GAME_StarWarsJediSurvivor:
    case EGame::GAME_DeltaForce:
        return 4;
    case EGame::GAME_FateTrigger:
        return 5;
    default:
        return 0;
    }
}

}

void FStaticMeshLODResources::SerializeBuffers(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    VertexBuffers.PositionVertexBuffer.Serialize(Ar);
    VertexBuffers.StaticMeshVertexBuffer.Serialize(Ar);
    VertexBuffers.ColorVertexBuffer.Serialize(Ar);
    const EGame Game = Ar.GetGame();
    if (Game == EGame::GAME_RogueCompany) {
        FColorVertexBuffer UnusedColors;
        UnusedColors.Serialize(Ar);
    }
    if (Game == EGame::GAME_ThePathless)
        Ar.SkipBytes(20);
    IndexBuffer.Serialize(Ar);
    FRawStaticIndexBuffer Unused;
    if (!StripFlags.IsClassDataStripped(CDSF_ReversedIndexBuffer))
        Unused.Serialize(Ar);
    if (Game == EGame::GAME_OutlastTrials)
        Ar.SkipBytes(4);
    Unused.Serialize(Ar);
    if (!StripFlags.IsClassDataStripped(CDSF_ReversedIndexBuffer))
        Unused.Serialize(Ar);
    if (!StripFlags.IsEditorDataStripped())
        Unused.Serialize(Ar);
    if (HasTessellation(Ar) && !StripFlags.IsClassDataStripped(CDSF_AdjacencyData) && Game != EGame::GAME_GTATheTrilogyDefinitiveEdition
        && Game != EGame::GAME_FinalFantasy7Rebirth)
        Unused.Serialize(Ar);
    if (Game == EGame::GAME_OutlastTrials)
        Ar.SkipBytes(4);
    if (Game == EGame::GAME_ArenaBreakoutInfinite) {
        Unused.Serialize(Ar);
        Unused.Serialize(Ar);
    }
    if (Game == EGame::GAME_TheFinals || Game == EGame::GAME_ArcRaiders) {
        Unused.Serialize(Ar);
        Ar.SkipBytes(4);
    }
    //! Final Fantasy VII Rebirth stores its real index buffer in a versioned byte array.
    if (Game == EGame::GAME_FinalFantasy7Rebirth) {
        Ar.SkipBytes(4);
        SkipArray(Ar, 1);
    }
    if (Ar.GetGame() >= EGame::GAME_UE4_25 && !StripFlags.IsClassDataStripped(CDSF_RayTracingResources)) {
        if (Ar.GetGame() >= EGame::GAME_UE5_6)
            Ar.SkipBytes(RayTracingRawDataHeaderSize);
        SkipBulkSerializedArray(Ar);
    }
    for (int32 i = 0; i < Sections.Num(); i++)
        SkipWeightedRandomSampler(Ar);
    SkipWeightedRandomSampler(Ar);
    bHasBuffers = !Ar.IsError();
}

void FStaticMeshLODResources::SerializeLegacyBuffers(FArchive& Ar, const FStripDataFlags& StripFlags) {
    const EGame Game = Ar.GetGame();
    if (Game == EGame::GAME_Abzu)
        Ar.SkipBytes(4);
    VertexBuffers.PositionVertexBuffer.Serialize(Ar);
    VertexBuffers.StaticMeshVertexBuffer.Serialize(Ar);
    if (Game == EGame::GAME_Borderlands3) {
        const int32 NumColorStreams = Ar.ReadCount(1);
        if (NumColorStreams > 0)
            VertexBuffers.ColorVertexBuffer.Serialize(Ar);
        for (int32 i = 1; i < NumColorStreams && !Ar.IsError(); i++) {
            FColorVertexBuffer UnusedColors;
            UnusedColors.Serialize(Ar);
        }
    } else {
        VertexBuffers.ColorVertexBuffer.Serialize(Ar);
    }
    IndexBuffer.Serialize(Ar);
    FRawStaticIndexBuffer Unused;
    if (Game == EGame::GAME_NarutotoBorutoShinobiStriker) {
        if (!StripFlags.IsClassDataStripped(CDSF_AdjacencyData))
            Unused.Serialize(Ar);
        for (int32 i = 0; i <= Sections.Num(); i++)
            SkipWeightedRandomSampler(Ar);
        bHasBuffers = !Ar.IsError();
        return;
    }
    //! PUBG strips all extra index buffers with flag bits 5 to 7.
    if (Game == EGame::GAME_PlayerUnknownsBattlegrounds && StripFlags.IsClassDataStripped(CDSF_StripIndexBuffers)) {
        SkipLegacySamplers(Ar, Sections.Num());
        bHasBuffers = !Ar.IsError();
        return;
    }
    const bool bReversed = Ar.UEVer() >= V4::SOUND_CONCURRENCY_PACKAGE && !StripFlags.IsClassDataStripped(CDSF_ReversedIndexBuffer);
    if (bReversed)
        Unused.Serialize(Ar);
    Unused.Serialize(Ar);
    if (bReversed)
        Unused.Serialize(Ar);
    if (Game == EGame::GAME_Abzu) {
        Ar.SkipBytes(4);
        SkipArray(Ar, 8);
        SkipArray(Ar, 4);
        SkipArray(Ar, 24);
        SkipArray(Ar, 4);
    }
    if (Ar.UEVer() >= V4::FTEXT_HISTORY && Ar.UEVer() < V4::RENAME_CROUCHMOVESCHARACTERDOWN)
        SkipDistanceField4(Ar);
    if (!StripFlags.IsEditorDataStripped())
        Unused.Serialize(Ar);
    if (!StripFlags.IsClassDataStripped(CDSF_AdjacencyData))
        Unused.Serialize(Ar);
    SkipLegacySamplers(Ar, Sections.Num());
    if (Game == EGame::GAME_SeaOfThieves)
        Ar.SkipBytes(17);
    bHasBuffers = !Ar.IsError();
}

void FStaticMeshLODResources::Serialize(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    FStripDataFlags StripFlags(Ar);
    if (Game == EGame::GAME_TheDivisionResurgence)
        Ar.SkipBytes(4);
    Ar << Sections;
    if (Game >= EGame::GAME_UE5_6)
        Ar << SourceMeshBounds.emplace();
    Ar << MaxDeviation;
    if (Game == EGame::GAME_ThePathless)
        Ar.SkipBytes(4);
    if (Ar.IsError())
        return;
    if (Game < EGame::GAME_UE4_23) {
        if (!StripFlags.IsAudioVisualDataStripped() && !StripFlags.IsClassDataStripped(CDSF_MinLodData))
            SerializeLegacyBuffers(Ar, StripFlags);
        return;
    }
    if (Game != EGame::GAME_Splitgate)
        Ar << bIsLODCookedOut;
    Ar << bBuffersInlined;
    if (Game == EGame::GAME_RogueCompany)
        bBuffersInlined = true;
    if (Ar.IsError() || StripFlags.IsAudioVisualDataStripped() || bIsLODCookedOut)
        return;
    if (Game >= EGame::GAME_UE5_5 || Game == EGame::GAME_MetalGearSolidDelta) {
        bool bHasRayTracingGeometry = false;
        Ar << bHasRayTracingGeometry;
    }
    if (bBuffersInlined) {
        SerializeBuffers(Ar);
        SkipInlinedGameData(Ar, Sections);
    } else {
        FByteBulkData BulkData;
        BulkData.Serialize(Ar);
        if (BulkData.GetElementCount() > 0) {
            Result<std::vector<std::byte>> Bytes = BulkData.Read();
            if (Bytes) {
                FMemoryReaderView BufferAr(*Bytes);
                BufferAr.CopyStateFrom(Ar);
                SerializeBuffers(BufferAr);
                if (BufferAr.IsError())
                    bHasBuffers = false;
            }
        }
        if (Game == EGame::GAME_HonorofKingsWorld) {
            Ar.SkipBytes(8);
            int32 Size = 0;
            Ar << Size;
            Ar.SkipBytes(Size == 32 ? 64 : 40);
        }
        Ar.SkipBytes(8 + 4 * 4 + 2 * 4 + 2 * 4 + 5 * 2 * 4);
        if (HasTessellation(Ar))
            Ar.SkipBytes(2 * 4);
        Ar.SkipBytes(Game >= EGame::GAME_UE5_6 ? RayTracingRawDataHeaderSize : GetStreamedGameDataSize(Game));
    }
    Ar.SkipBytes(12);
    if (Game == EGame::GAME_StarWarsJediSurvivor || Game == EGame::GAME_TheFinals || Game == EGame::GAME_ArcRaiders)
        Ar.SkipBytes(4);
}

void FStaticMeshRenderData::SerializeDistanceFields(FArchive& Ar) {
    if (Ar.UEVer() < V4::RENAME_CROUCHMOVESCHARACTERDOWN)
        return;
    if (Ar.UEVer() >= V4::RENAME_WIDGET_VISIBILITY) {
        FStripDataFlags StripFlags(Ar);
        if (StripFlags.IsAudioVisualDataStripped())
            return;
        if (Ar.GetGame() >= EGame::GAME_UE4_21 && StripFlags.IsClassDataStripped(1))
            return;
    }
    for (int32 i = 0; i < LODResources.Num() && !Ar.IsError(); i++) {
        bool bValid = false;
        Ar << bValid;
        if (!bValid)
            continue;
        const EGame Game = Ar.GetGame();
        if (Game >= EGame::GAME_UE5_0 || Game == EGame::GAME_TerminullBrigade || Game == EGame::GAME_WutheringWaves)
            SkipDistanceField5(Ar);
        else
            SkipDistanceField4(Ar);
        if (Game != EGame::GAME_TheFinals && Game != EGame::GAME_ArcRaiders)
            continue;
        const int32 Count = Ar.ReadCount(1);
        for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
            SkipDistanceField5(Ar);
    }
}

namespace {

//! Bytes that games add before the LODs of the static mesh render data.
void SkipLeadingRenderData(FArchive& Ar) {
    switch (Ar.GetGame()) {
    case EGame::GAME_HYENAS:
        Ar.SkipBytes(1);
        return;
    case EGame::GAME_DuneAwakening:
    case EGame::GAME_Squad:
        Ar.SkipBytes(4);
        return;
    case EGame::GAME_DaysGone: {
        int32 Count = 0;
        Ar << Count;
        Ar.SkipBytes(static_cast<int64>(Count) * 4 + 4);
        return;
    }
    default:
        return;
    }
}

//! Arena Breakout Infinite adds a signed distance field block after the distance fields.
void SkipArenaBreakoutInfiniteData(FArchive& Ar, int32 LODCount) {
    FStripDataFlags StripFlags(Ar);
    bool bHasData = false;
    Ar << bHasData;
    if (!bHasData)
        return;
    FBox Bounds;
    Ar << Bounds;
    Ar.SkipBytes(4 + 3 * 56);
    SkipArray(Ar, 1);
    for (int32 Index = 0; Index < LODCount && !Ar.IsError(); Index++) {
        int32 Flags = 0;
        Ar << Flags;
        if (Flags == 0)
            continue;
        FByteBulkData BulkData;
        BulkData.Serialize(Ar);
    }
}

int32 GetScreenSizeCount(EGame Game) {
    if (Game == EGame::GAME_FragPunk || Game == EGame::GAME_RocoKingdomWorld)
        return 16;
    if (Game == EGame::GAME_Stalker2)
        return 14;
    return Game >= EGame::GAME_UE4_9 ? 8 : 4;
}

//! Borderlands 3 adds per-LOD flags after the screen sizes.
void SkipBorderlands3Data(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(1);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++) {
        uint8 FlagCount = 0;
        Ar << FlagCount;
        Ar.SkipBytes(static_cast<int64>(FlagCount) * 12);
    }
}

}

void FStaticMeshRenderData::Serialize(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    if (Game == EGame::GAME_TonyHawkProSkater34) {
        bool bHasRenderData = false;
        Ar << bHasRenderData;
        if (!bHasRenderData)
            return;
    }
    SkipLeadingRenderData(Ar);
    const int32 LODCount = Ar.ReadCount(16);
    LODResources.Empty(LODCount);
    for (int32 i = 0; i < LODCount && !Ar.IsError(); i++)
        LODResources[LODResources.AddDefaulted()].Serialize(Ar);
    if (Game >= EGame::GAME_UE4_23)
        Ar << NumInlinedLODs;
    if (Game >= EGame::GAME_UE5_0) {
        NaniteResources.Serialize(Ar);
        if (Game >= EGame::GAME_UE5_5) {
            bool bHasRayTracingProxy = false;
            Ar << bHasRayTracingProxy;
            if (bHasRayTracingProxy)
                SkipStaticMeshRayTracingProxy(Ar);
        }
        FStripDataFlags CardFlags(Ar);
        if (!CardFlags.IsAudioVisualDataStripped() && !CardFlags.IsClassDataStripped(2))
            for (int32 i = 0; i < LODResources.Num() && !Ar.IsError(); i++) {
                bool bValid = false;
                Ar << bValid;
                if (bValid)
                    SkipCardRepresentation(Ar);
            }
    }
    if (Game == EGame::GAME_HonorofKingsWorld && HasHonorofKingsWorldSectionData(LODResources)) {
        const int32 Count = Ar.ReadCount(4);
        for (int32 i = 0; i < Count && !Ar.IsError(); i++)
            SkipArray(Ar, 41);
    }
    SerializeDistanceFields(Ar);
    if (Game == EGame::GAME_ArenaBreakoutInfinite)
        SkipArenaBreakoutInfiniteData(Ar, LODResources.Num());
    Ar << Bounds;
    //! UE 4.15 does not write bLODsShareStaticLighting.
    if (Game < EGame::GAME_UE4_15 || Game >= EGame::GAME_UE4_16) {
        if (Game >= EGame::GAME_UE5_6 || Game == EGame::GAME_GrayZoneWarfare || Game == EGame::GAME_HighOnLife2 || Game == EGame::GAME_Gothic1Remake) {
            uint8 RenderDataFlags = 0;
            Ar << RenderDataFlags;
            bLODsShareStaticLighting = (RenderDataFlags & 1) != 0;
        } else {
            Ar << bLODsShareStaticLighting;
        }
    }
    if (Game < EGame::GAME_UE4_14) {
        bool bReducedBySimplygon = false;
        Ar << bReducedBySimplygon;
    }
    //! Abzu streams four UV sets instead of eight.
    if (!Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::TextureStreamingMeshUVChannelData, EGame::GAME_UE4_14))
        Ar.SkipBytes(4 * (Game == EGame::GAME_Abzu ? 4 : MaxStreamingTexCoords) + 4);
    if (Game == EGame::GAME_DeltaForce || Game == EGame::GAME_DeadzoneRogue)
        Ar.SkipBytes(4);
    if (Game == EGame::GAME_InfinityNikki)
        Ar.SkipBytes(8);
    const int32 ScreenSizeCount = GetScreenSizeCount(Game);
    for (int32 i = 0; i < ScreenSizeCount; i++) {
        if (Game >= EGame::GAME_UE4_20) {
            bool bCooked = false;
            Ar << bCooked;
        }
        float Size = 0;
        Ar << Size;
        if (i < static_cast<int32>(std::size(ScreenSize)))
            ScreenSize[i] = Size;
        if (Game == EGame::GAME_HogwartsLegacy)
            Ar.SkipBytes(8);
        if (Game == EGame::GAME_VisionsofMana)
            Ar.SkipBytes(4);
    }
    if (Game == EGame::GAME_Borderlands3)
        SkipBorderlands3Data(Ar);
    if (Game >= EGame::GAME_UE5_4)
        FStripDataFlags RayTracingFlags(Ar);
}

FArchive& operator<<(FArchive& Ar, FStaticMaterial& Material) {
    UObject* Interface = nullptr;
    Ar << Interface << Material.MaterialSlotName;
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::TextureStreamingMeshUVChannelData, EGame::GAME_UE4_14))
        Ar << Material.UVChannelData;
    Material.MaterialInterface = Cast<UMaterialInterface>(Interface);
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::MeshMaterialSlotOverlayMaterialAdded,
                            EGame::GAME_UE5_6)) {
        UObject* Overlay = nullptr;
        Ar << Overlay;
        Material.OverlayMaterialInterface = Cast<UMaterialInterface>(Overlay);
    }
    if (Ar.GetGame() == EGame::GAME_FragPunk || Ar.GetGame() == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(4);
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FMeshUVChannelInfo& Info) {
    Ar << Info.bInitialized << Info.bOverrideDensities;
    for (float& Density : Info.LocalUVDensities)
        Ar << Density;
    return Ar;
}

void UStaticMesh::SkipOccluderData(FArchive& Ar) {
    bool bHasOccluderData = false;
    Ar << bHasOccluderData;
    if (!bHasOccluderData)
        return;
    switch (Ar.GetGame()) {
    case EGame::GAME_FragPunk: {
        bool bHasData = false;
        Ar << bHasData;
        if (bHasData)
            SkipBulkArrays(Ar, 3);
        return;
    }
    case EGame::GAME_CrystalOfAtlan:
    case EGame::GAME_RocoKingdomWorld:
        SkipBulkArrays(Ar, 3);
        return;
    case EGame::GAME_Farlight84: {
        SkipBulkArrays(Ar, 2);
        const int32 Count = Ar.ReadCount(8);
        for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
            SkipBulkArrays(Ar, 2);
        return;
    }
    case EGame::GAME_HonorofKingsWorld:
        SkipBulkArrays(Ar, 1);
        return;
    default:
        SkipArray(Ar, 12);
        SkipArray(Ar, 2);
        return;
    }
}

namespace {

//! Bytes that games add after the materials of a static mesh.
void SkipTrailingGameData(FArchive& Ar) {
    switch (Ar.GetGame()) {
    case EGame::GAME_OutlastTrials:
        Ar.SkipBytes(1);
        return;
    case EGame::GAME_Farlight84:
    case EGame::GAME_DuneAwakening:
        Ar.SkipBytes(4);
        return;
    case EGame::GAME_DaysGone: {
        int32 Count = 0;
        Ar << Count;
        Ar.SkipBytes(static_cast<int64>(Count) * 4 + 4);
        return;
    }
    default:
        return;
    }
}

}

void UStaticMesh::Serialize(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    if (Game == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(12);
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    FStripDataFlags StripFlags(Ar);
    bool bCooked = false;
    Ar << bCooked;
    UObject* BodySetupObject = nullptr;
    Ar << BodySetupObject;
    BodySetup = reinterpret_cast<UBodySetup*>(BodySetupObject);
    if (Ar.UEVer() >= V4::STATIC_MESH_STORE_NAV_COLLISION && Game != EGame::GAME_GearsOfWar4 && Game != EGame::GAME_TEKKEN7) {
        UObject* NavCollisionObject = nullptr;
        Ar << NavCollisionObject;
        NavCollision = Cast<UNavCollisionBase>(NavCollisionObject);
    }
    if (!StripFlags.IsEditorDataStripped()) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "UStaticMesh.EditorData");
        return;
    }
    Ar << LightingGuid;
    TArray<UObject*> SocketObjects;
    Ar << SocketObjects;
    Sockets.Empty(SocketObjects.Num());
    for (UObject* Socket : SocketObjects)
        Sockets.Add(Cast<UStaticMeshSocket>(Socket));
    if (bCooked) {
        RenderData = std::make_unique<FStaticMeshRenderData>();
        RenderData->Serialize(Ar);
        if (Ar.IsError())
            return;
    }
    if (Game == EGame::GAME_RocoKingdomWorld)
        Ar.SkipBytes(4);
    //! Dream Star removed the occluder data.
    if (bCooked && Game >= EGame::GAME_UE4_20 && Game < EGame::GAME_UE5_0 && Game != EGame::GAME_DreamStar)
        SkipOccluderData(Ar);
    if (Game == EGame::GAME_FateTrigger || Game == EGame::GAME_GhostsofTabor || Game == EGame::GAME_Aion2)
        Ar.SkipBytes(4);
    if (Game == EGame::GAME_TheFinals || Game == EGame::GAME_ArcRaiders) {
        bool bHasData = false;
        Ar << bHasData;
        if (bHasData)
            SkipBulkArrays(Ar, 5);
    }
    if (Ar.UEVer() >= V4::SPEEDTREE_STATICMESH)
        Ar << bHasSpeedTreeWind;
    if (bHasSpeedTreeWind) {
        Ar.Seek(Ar.TotalSize());
        return;
    }
    if (Game < EGame::GAME_UE4_14) {
        ConvertLegacyMaterials();
        return;
    }
    if (Ar.CustomVerAtLeast(FEditorObjectVersion::GUID, FEditorObjectVersion::RefactorMeshEditorMaterials, EGame::GAME_UE4_14))
        Ar << StaticMaterials;
    SkipTrailingGameData(Ar);
}

void UStaticMesh::ConvertLegacyMaterials() {
    for (UMaterialInterface* Material : Materials) {
        FStaticMaterial& StaticMaterial = StaticMaterials.Emplace_GetRef();
        StaticMaterial.MaterialInterface = Material;
        StaticMaterial.MaterialSlotName = Material != nullptr ? Material->GetFName() : NAME_None;
    }
    Materials.Empty();
}

void UStaticMesh::PostLoad() {
    Super::PostLoad();
    //! Before UE 4.9 a static mesh has no ExtendedBounds property and returns the bounds of its render data.
    if (GetRuntimeProvider().getGame() < EGame::GAME_UE4_9 && RenderData != nullptr)
        ExtendedBounds = RenderData->Bounds;
}

int32 UStaticMesh::GetNumNaniteVertices() const {
    if (RenderData == nullptr || !RenderData->HasValidNaniteData() || RenderData->NaniteResources.RootDataSize == 0)
        return 0;
    return static_cast<int32>(RenderData->NaniteResources.NumInputVertices);
}

int32 UStaticMesh::GetNumNaniteTriangles() const {
    if (RenderData == nullptr || !RenderData->HasValidNaniteData() || RenderData->NaniteResources.RootDataSize == 0)
        return 0;
    return static_cast<int32>(RenderData->NaniteResources.NumInputTriangles);
}

UMaterialInterface* UStaticMesh::GetMaterial(int32 MaterialIndex) const {
    return StaticMaterials.IsValidIndex(MaterialIndex) ? StaticMaterials[MaterialIndex].MaterialInterface : nullptr;
}

UStaticMeshSocket* UStaticMesh::FindSocket(FName InSocketName) const {
    if (InSocketName.isNone())
        return nullptr;
    for (UStaticMeshSocket* Socket : Sockets)
        if (Socket != nullptr && Socket->SocketName == InSocketName)
            return Socket;
    return nullptr;
}

int32 UStaticMesh::GetMaterialIndex(FName MaterialSlotName) const {
    for (int32 Index = 0; Index < StaticMaterials.Num(); Index++)
        if (StaticMaterials[Index].MaterialSlotName == MaterialSlotName)
            return Index;
    return INDEX_NONE;
}

}

#include "Engine/Source/Runtime/Engine/Animation/MorphTarget.h"

#include <algorithm>
#include <vector>

#include "CppUE5Parse/GameTypes/RocoKingdomWorld/RocoKingdomWorldMorphTarget.h"
#include "CppUE5Parse/GameTypes/SupermassiveGames/DarkPicturesMorphTarget.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnknownProperties.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

namespace {

using V4 = EUnrealEngineObjectUE4Version;

FMorphTargetDelta ToMorphTargetDelta(const DecodedMorphTargetDelta& Decoded) {
    FMorphTargetDelta Delta;
    Delta.PositionDelta = FVector3f(Decoded.positionDelta[0], Decoded.positionDelta[1], Decoded.positionDelta[2]);
    Delta.TangentZDelta = FVector3f(Decoded.tangentZDelta[0], Decoded.tangentZDelta[1], Decoded.tangentZDelta[2]);
    Delta.SourceIdx = Decoded.sourceIndex;
    return Delta;
}

bool IsDarkPicturesAnthology(EGame Game) {
    return Game == EGame::GAME_DarkPicturesAnthologyHouseOfAshes || Game == EGame::GAME_DarkPicturesAnthologyTheDevilinMe ||
           Game == EGame::GAME_DarkPicturesAnthologyManofMedan || Game == EGame::GAME_DarkPicturesAnthologyLittleHope;
}

void SerializeDarkPicturesDeltas(FArchive& Ar, FMorphTargetLODModel& M) {
    const int64 Start = Ar.Tell();
    const std::span<const std::byte> Data = Ar.ReadView(Ar.GetRemaining());
    if (Ar.IsError())
        return;
    Result<DarkPicturesMorphTargetDeltas> Decoded =
        decodeDarkPicturesMorphTargetDeltas(Data, Ar.GetGame() != EGame::GAME_DarkPicturesAnthologyLittleHope);
    if (!Decoded) {
        Ar.SetError(Decoded.error().code, Decoded.error().where);
        return;
    }
    Ar.Seek(Start + static_cast<int64>(Decoded->bytesRead));
    M.Vertices.Empty(static_cast<int32>(Decoded->deltas.size()));
    for (const DecodedMorphTargetDelta& Delta : Decoded->deltas)
        M.Vertices.Add(ToMorphTargetDelta(Delta));
}

void SerializeRocoKingdomWorldDeltas(FArchive& Ar, FMorphTargetLODModel& M) {
    TArray<uint64> Packed;
    Ar << Packed;
    M.Vertices.Empty(Packed.Num());
    for (const uint64 Value : Packed)
        M.Vertices.Add(ToMorphTargetDelta(decodeRocoKingdomWorldMorphTargetDelta(Value)));
}

std::vector<float> GetLoadedFloatArray(FArchive& Ar, const UObject* Object, FName Name) {
    const FLoadedPropertyValue Found = FindLoadedPropertyValue(Ar, Object->GetClass(), Object, Name);
    const FArrayProperty* Array = CastField<FArrayProperty>(Found.Property);
    if (Array == nullptr || CastField<FFloatProperty>(Array->Inner) == nullptr)
        return {};
    FScriptArrayHelper Helper(Array, Found.Value);
    std::vector<float> Values(static_cast<std::size_t>(Helper.Num()));
    for (int32 Index = 0; Index < Helper.Num(); Index++)
        Values[static_cast<std::size_t>(Index)] = FFloatProperty::GetPropertyValue(Helper.GetRawPtr(Index));
    return Values;
}

}

FArchive& operator<<(FArchive& Ar, FMorphTargetDelta& V) {
    if (Ar.UEVer() < V4::MORPHTARGET_CPU_TANGENTZDELTA_FORMATCHANGE) {
        uint8 Packed[4] = {};
        Ar << V.PositionDelta;
        Ar.Serialize(Packed, sizeof(Packed));
        Ar << V.SourceIdx;
        V.TangentZDelta = FVector3f(Packed[0] / 127.5f - 1.0f, Packed[1] / 127.5f - 1.0f, Packed[2] / 127.5f - 1.0f);
    } else {
        Ar << V.PositionDelta << V.TangentZDelta << V.SourceIdx;
    }
    if (Ar.GetGame() == EGame::GAME_StarWarsHunters)
        Ar.SkipBytes(4);
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FMorphTargetLODModel& M) {
    const EGame Game = Ar.GetGame();
    if (!Ar.CustomVerAtLeast(FEditorObjectVersion::GUID, FEditorObjectVersion::AddedMorphTargetSectionIndices, EGame::GAME_UE4_19)) {
        Ar << M.Vertices << M.NumBaseMeshVerts;
        M.bGeneratedByEngine = false;
    } else if (!Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::SaveGeneratedMorphTargetByEngine,
                                    EGame::GAME_UE4_20)) {
        Ar << M.Vertices << M.NumBaseMeshVerts << M.SectionIndices;
        M.bGeneratedByEngine = false;
    } else if (Game == EGame::GAME_TheCastingofFrankStone || Game == EGame::GAME_TheQuarry || Game == EGame::GAME_Directive8020 ||
               Game == EGame::GAME_MortalKombat1) {
        Ar << M.NumVertices;
        M.NumBaseMeshVerts = 1;
        if (Game == EGame::GAME_MortalKombat1)
            Ar << M.NumBaseMeshVerts;
        Ar << M.SectionIndices << M.bGeneratedByEngine;
        return Ar;
    } else if (IsDarkPicturesAnthology(Game)) {
        SerializeDarkPicturesDeltas(Ar, M);
        M.NumVertices = M.Vertices.Num();
        Ar << M.SectionIndices << M.bGeneratedByEngine;
        return Ar;
    } else if (Game == EGame::GAME_RocoKingdomWorld) {
        SerializeRocoKingdomWorldDeltas(Ar, M);
        M.NumVertices = M.Vertices.Num();
        Ar << M.NumBaseMeshVerts << M.SectionIndices << M.bGeneratedByEngine;
        return Ar;
    } else {
        bool bVerticesAreStrippedForCookedBuilds = false;
        if (Ar.CustomVerAtLeast(FUE5SpecialProjectStreamObjectVersion::GUID, FUE5SpecialProjectStreamObjectVersion::StripMorphTargetSourceDataForCookedBuilds,
                                EGame::GAME_UE5_0))
            Ar << bVerticesAreStrippedForCookedBuilds;
        if (bVerticesAreStrippedForCookedBuilds) {
            Ar << M.NumVertices;
        } else {
            Ar << M.Vertices;
            M.NumVertices = M.Vertices.Num();
        }
        Ar << M.NumBaseMeshVerts << M.SectionIndices << M.bGeneratedByEngine;
    }
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::MorphTargetCustomImport, EGame::GAME_UE5_5))
        Ar << M.SourceFilename;
    //! UE 5.6 marks a morph target generated by the engine with a source file name prefix and clears the name on load.
    if (M.SourceFilename.StartsWith(FString("GeneratedByEngine_"))) {
        M.bGeneratedByEngine = true;
        M.SourceFilename.Empty();
    }
    return Ar;
}

void UMorphTarget::Serialize(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    if (Game == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(4);
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsAudioVisualDataStripped())
        return;
    bool bCooked = false;
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::MorphTargetCookedCPUDataCompressed, EGame::GAME_UE5_7))
        Ar << bCooked;
    if (Game == EGame::GAME_NevernessToEverness)
        Ar << bCooked;
    if (Game == EGame::GAME_MortalKombat1)
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
    Ar << MorphLODModels;
    if (Ar.IsError())
        return;
    if (Game == EGame::GAME_RocoKingdomWorld)
        ScaleRocoKingdomWorldDeltas(Ar);
    if (!bCooked)
        return;
    bool bStoreCompressedVertices = false;
    Ar << bStoreCompressedVertices;
    if (!bStoreCompressedVertices)
        return;
    const int32 NumLODs = Ar.ReadCount(4 + 4 + 8);
    CompressedLODModels.Empty(NumLODs);
    for (int32 LODIndex = 0; LODIndex < NumLODs && !Ar.IsError(); LODIndex++) {
        FMorphTargetCompressedLODModel& LODModel = CompressedLODModels[CompressedLODModels.AddDefaulted()];
        Ar << LODModel.PackedDeltaHeaders << LODModel.PackedDeltaData << LODModel.PositionPrecision << LODModel.TangentPrecision;
    }
}

//! Roco Kingdom: World scales the packed deltas of each LOD by the compress extents of the morph target.
void UMorphTarget::ScaleRocoKingdomWorldDeltas(FArchive& Ar) {
    const std::vector<float> PositionScales = GetLoadedFloatArray(Ar, this, FName("MorphPosDeltaCompressExtent"));
    const std::vector<float> TangentScales = GetLoadedFloatArray(Ar, this, FName("MorphTanDeltaCompressExtent"));
    const std::size_t Count = std::min({PositionScales.size(), TangentScales.size(), static_cast<std::size_t>(MorphLODModels.Num())});
    for (std::size_t LODIndex = 0; LODIndex < Count; LODIndex++) {
        for (FMorphTargetDelta& Delta : MorphLODModels[static_cast<int32>(LODIndex)].Vertices) {
            Delta.PositionDelta = Delta.PositionDelta * PositionScales[LODIndex];
            Delta.TangentZDelta = Delta.TangentZDelta * TangentScales[LODIndex];
        }
    }
}

}

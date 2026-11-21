#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/HairStrandsDatas.h"

namespace ue5parse::runtime {

namespace {

constexpr uint32 DataFlags_HasData = 1;

}

void FHairStrandsBulkData::Serialize(FArchive& Ar) {
    SerializeHeader(Ar);
    SerializeData(Ar);
}

void FHairStrandsBulkData::SerializeHeader(FArchive& Ar) {
    FHeader& H = Header;
    const EGame Game = Ar.GetGame();
    Ar << H.CurveCount << H.PointCount << H.MaxLength << H.MaxRadius << H.BoundingBox;
    if (Game < EGame::GAME_UE5_3) {
        Ar << H.Flags;
        if (Game >= EGame::GAME_UE5_2)
            for (uint32& Offset : H.AttributeOffsets)
                Ar << Offset;
        return;
    }
    Ar << H.MinPointPerCurve << H.MaxPointPerCurve << H.AvgPointPerCurve << H.Flags;
    if (Game >= EGame::GAME_UE5_8)
        Ar << H.PositionHash << H.CurveCountHash << H.CurveOffsetHash;
    for (uint32& Offset : H.CurveAttributeOffsets)
        Ar << Offset;
    for (uint32& Offset : H.PointAttributeOffsets)
        Ar << Offset;
    Ar << H.ImportedAttributes << H.ImportedAttributeFlags;
    if (Game >= EGame::GAME_UE5_6)
        Ar << H.SourceCurveCount << H.SourcePointCount;
    Ar << H.CurveToPointCount;
    if (Game < EGame::GAME_UE5_4) {
        Ar << H.PositionStride << H.CurveStride << H.PointToCurveStride << H.CurveAttributeChunkStride << H.PointAttributeChunkStride;
        Ar << H.CurveAttributeChunkElementCount << H.PointAttributeChunkElementCount;
        return;
    }
    Ar << H.CoverageScales;
    Ar << H.PositionStride << H.CurveStride << H.PointToCurveStride << H.CurveAttributeChunkStride << H.PointAttributeChunkStride;
    Ar << H.TranscodedPositionChunkStride;
    if (Game >= EGame::GAME_UE5_6)
        Ar << H.CurveMappingStride << H.PointMappingStride;
    Ar << H.PointToCurveChunkElementCount << H.CurveAttributeChunkElementCount << H.PointAttributeChunkElementCount << H.TranscodedPositionChunkElementCount;
    Ar << H.PositionOffset << H.PositionScale;
}

void FHairStrandsBulkData::SerializeData(FArchive& Ar) {
    if (!(Header.Flags & DataFlags_HasData))
        return;
    const EGame Game = Ar.GetGame();
    if (Game < EGame::GAME_UE5_2) {
        constexpr uint32 DataFlags_HasMaterialData = 2;
        Data.Positions.Serialize(Ar);
        Data.Attributes0.Serialize(Ar);
        Data.Attributes1.Serialize(Ar);
        if (Header.Flags & DataFlags_HasMaterialData)
            Data.Materials.Serialize(Ar);
        Data.CurveOffsets.Serialize(Ar);
        return;
    }
    if (Game < EGame::GAME_UE5_3) {
        Data.Positions.Serialize(Ar);
        Data.Attributes.Serialize(Ar);
        Data.VertexToCurve.Serialize(Ar);
        Data.Curves.Serialize(Ar);
        return;
    }
    //! The point attribute and transcoded position flags were renumbered in 5.4.
    const uint32 DataFlags_HasPointAttribute = Game >= EGame::GAME_UE5_4 ? 0x2u : 0x4u;
    constexpr uint32 DataFlags_HasTranscodedPosition = 0x4u;
    constexpr uint32 DataFlags_HasPointMapping = 0x20u;
    constexpr uint32 DataFlags_HasCurveMapping = 0x40u;
    if (Game >= EGame::GAME_UE5_4 && (Header.Flags & DataFlags_HasTranscodedPosition))
        Data.TranscodedPositions.Serialize(Ar);
    else
        Data.Positions.Serialize(Ar);
    Data.CurveAttributes.Serialize(Ar);
    if (Header.Flags & DataFlags_HasPointAttribute)
        Data.PointAttributes.Serialize(Ar);
    Data.PointToCurve.Serialize(Ar);
    Data.Curves.Serialize(Ar);
    if (Game < EGame::GAME_UE5_6)
        return;
    if (Header.Flags & DataFlags_HasPointMapping)
        Data.PointMapping.Serialize(Ar);
    if (Header.Flags & DataFlags_HasCurveMapping)
        Data.CurveMapping.Serialize(Ar);
}

void FHairStrandsInterpolationBulkData::Serialize(FArchive& Ar) {
    SerializeHeader(Ar);
    SerializeData(Ar);
}

void FHairStrandsInterpolationBulkData::SerializeHeader(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    Ar << Header.Flags << Header.PointCount;
    if (Game < EGame::GAME_UE5_3) {
        Ar << Header.SimPointCount;
        return;
    }
    if (Game < EGame::GAME_UE5_4) {
        Ar << Header.SimPointCount << Header.InterpolationStride << Header.SimRootPointIndexStride;
        return;
    }
    Ar << Header.CurveCount << Header.CurveInterpolationStride << Header.PointInterpolationStride;
}

void FHairStrandsInterpolationBulkData::SerializeData(FArchive& Ar) {
    if (!(Header.Flags & DataFlags_HasData))
        return;
    const EGame Game = Ar.GetGame();
    if (Game < EGame::GAME_UE5_3) {
        constexpr uint32 DataFlags_HasSingleGuideData = 2;
        if (Header.Flags & DataFlags_HasSingleGuideData) {
            Data.Interpolation.Serialize(Ar);
        } else {
            Data.Interpolation0.Serialize(Ar);
            Data.Interpolation1.Serialize(Ar);
        }
        Data.SimRootPointIndex.Serialize(Ar);
        return;
    }
    if (Game < EGame::GAME_UE5_4) {
        Data.Interpolation.Serialize(Ar);
        Data.SimRootPointIndex.Serialize(Ar);
        return;
    }
    Data.CurveInterpolation.Serialize(Ar);
    Data.PointInterpolation.Serialize(Ar);
}

void FHairStrandsClusterBulkData::Serialize(FArchive& Ar) {
    SerializeHeader(Ar);
    SerializeData(Ar);
}

void FHairStrandsClusterBulkData::SerializeHeader(FArchive& Ar) {
    FHeader& H = Header;
    if (Ar.GetGame() < EGame::GAME_UE5_3) {
        Ar << H.ClusterCount << H.ClusterLODCount << H.VertexCount << H.VertexLODCount << H.LODVisibility << H.CPULODScreenSize;
        return;
    }
    Ar << H.ClusterCount << H.ClusterScale << H.PointCount << H.CurveCount << H.ClusterInfoParameters;
    const int32 LODInfoCount = Ar.ReadCount(4 + 4 + 4 + 4 + 4);
    H.LODInfos.SetNum(LODInfoCount);
    for (FHairLODInfo& Info : H.LODInfos)
        Ar << Info.CurveCount << Info.PointCount << Info.RadiusScale << Info.ScreenSize << Info.bIsVisible;
    Ar << H.PackedClusterInfoStride << H.CurveToClusterIdStride << H.PointLODStride;
}

void FHairStrandsClusterBulkData::SerializeData(FArchive& Ar) {
    if (Ar.GetGame() < EGame::GAME_UE5_3) {
        if (Header.ClusterLODCount)
            Data.ClusterLODInfos.Serialize(Ar);
        if (Header.VertexCount)
            Data.VertexToClusterIds.Serialize(Ar);
        if (Header.VertexLODCount)
            Data.ClusterVertexIds.Serialize(Ar);
        if (Header.ClusterCount)
            Data.PackedClusterInfos.Serialize(Ar);
        return;
    }
    if (Header.ClusterCount == 0)
        return;
    Data.PackedClusterInfos.Serialize(Ar);
    Data.CurveToClusterIds.Serialize(Ar);
    Data.PointLODs.Serialize(Ar);
}

}

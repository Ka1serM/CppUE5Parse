#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"

namespace ue5parse::runtime {

struct FHairStrandsBulkData {
    struct FHeader {
        uint32 CurveCount = 0;
        uint32 PointCount = 0;
        float MaxLength = 0.0f;
        float MaxRadius = 0.0f;
        FBox BoundingBox;
        uint32 MinPointPerCurve = 0;
        uint32 MaxPointPerCurve = 0;
        uint32 AvgPointPerCurve = 0;
        uint32 Flags = 0;
        uint64 PositionHash = 0;
        uint64 CurveCountHash = 0;
        uint64 CurveOffsetHash = 0;
        uint32 AttributeOffsets[7] = {};
        uint32 CurveAttributeOffsets[5] = {};
        uint32 PointAttributeOffsets[3] = {};
        uint32 ImportedAttributes = 0;
        uint32 ImportedAttributeFlags = 0;
        uint32 SourceCurveCount = 0;
        uint32 SourcePointCount = 0;
        TArray<uint32> CurveToPointCount;
        TArray<float> CoverageScales;
        uint32 PositionStride = 0;
        uint32 CurveStride = 0;
        uint32 PointToCurveStride = 0;
        uint32 CurveAttributeChunkStride = 0;
        uint32 PointAttributeChunkStride = 0;
        uint32 TranscodedPositionChunkStride = 0;
        uint32 CurveMappingStride = 0;
        uint32 PointMappingStride = 0;
        uint32 PointToCurveChunkElementCount = 0;
        uint32 CurveAttributeChunkElementCount = 0;
        uint32 PointAttributeChunkElementCount = 0;
        uint32 TranscodedPositionChunkElementCount = 0;
        FVector3f PositionOffset = FVector3f(1.0f);
        FVector3f PositionScale = FVector3f(1.0f);
    };
    struct FData {
        FByteBulkData Positions;
        FByteBulkData TranscodedPositions;
        FByteBulkData Attributes0;
        FByteBulkData Attributes1;
        FByteBulkData Materials;
        FByteBulkData CurveOffsets;
        FByteBulkData Attributes;
        FByteBulkData VertexToCurve;
        FByteBulkData CurveAttributes;
        FByteBulkData PointAttributes;
        FByteBulkData PointToCurve;
        FByteBulkData Curves;
        FByteBulkData PointMapping;
        FByteBulkData CurveMapping;
    };

    FHeader Header;
    FData Data;

    void Serialize(FArchive& Ar);
    void SerializeHeader(FArchive& Ar);
    void SerializeData(FArchive& Ar);
};

struct FHairStrandsInterpolationBulkData {
    struct FHeader {
        uint32 Flags = 0;
        uint32 PointCount = 0;
        uint32 SimPointCount = 0;
        uint32 CurveCount = 0;
        uint32 InterpolationStride = 0;
        uint32 SimRootPointIndexStride = 0;
        uint32 CurveInterpolationStride = 0;
        uint32 PointInterpolationStride = 0;
    };
    struct FData {
        FByteBulkData Interpolation;
        FByteBulkData Interpolation0;
        FByteBulkData Interpolation1;
        FByteBulkData SimRootPointIndex;
        FByteBulkData CurveInterpolation;
        FByteBulkData PointInterpolation;
    };

    FHeader Header;
    FData Data;

    void Serialize(FArchive& Ar);
    void SerializeHeader(FArchive& Ar);
    void SerializeData(FArchive& Ar);
};

struct FHairLODInfo {
    uint32 CurveCount = 0;
    uint32 PointCount = 0;
    float RadiusScale = 1.0f;
    float ScreenSize = 1.0f;
    bool bIsVisible = true;
};

//! FHairStrandsClusterCullingBulkData before 5.3, FHairStrandsClusterBulkData after.
struct FHairStrandsClusterBulkData {
    struct FHeader {
        uint32 ClusterCount = 0;
        uint32 ClusterLODCount = 0;
        uint32 VertexCount = 0;
        uint32 VertexLODCount = 0;
        TArray<bool> LODVisibility;
        TArray<float> CPULODScreenSize;
        float ClusterScale = 0.0f;
        uint32 PointCount = 0;
        uint32 CurveCount = 0;
        FVector4f ClusterInfoParameters;
        TArray<FHairLODInfo> LODInfos;
        uint32 PackedClusterInfoStride = 0;
        uint32 CurveToClusterIdStride = 0;
        uint32 PointLODStride = 0;
    };
    struct FData {
        FByteBulkData ClusterLODInfos;
        FByteBulkData VertexToClusterIds;
        FByteBulkData ClusterVertexIds;
        FByteBulkData PackedClusterInfos;
        FByteBulkData CurveToClusterIds;
        FByteBulkData PointLODs;
    };

    FHeader Header;
    FData Data;

    void Serialize(FArchive& Ar);
    void SerializeHeader(FArchive& Ar);
    void SerializeData(FArchive& Ar);
};

}

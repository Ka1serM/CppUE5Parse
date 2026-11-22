#pragma once

#include <limits>

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "MorphTarget.generated.h"

namespace ue5parse::runtime {

class USkeletalMesh;

struct FMorphTargetDelta {
    FVector3f PositionDelta = FVector3f(0, 0, 0);
    FVector3f TangentZDelta = FVector3f(0, 0, 0);
    uint32 SourceIdx = std::numeric_limits<uint32>::max();

    friend FArchive& operator<<(FArchive& Ar, FMorphTargetDelta& V);
};

namespace MorphTargetVertexCodec {

struct FDeltaBatchHeader {
    uint32 DataOffset = 0;
    uint32 NumElements = 0;
    bool bTangents = false;
    int8 IndexBits = 0;
    TIntVector3<int8> PositionBits;
    TIntVector3<int8> TangentZBits;
    uint32 IndexMin = 0;
    FIntVector PositionMin;
    FIntVector TangentZMin;

    friend FArchive& operator<<(FArchive& Ar, FDeltaBatchHeader& InHeader);
};

}

struct FMorphTargetCompressedLODModel {
    TArray<MorphTargetVertexCodec::FDeltaBatchHeader> PackedDeltaHeaders;
    TArray<uint32> PackedDeltaData;
    float PositionPrecision = 0.0f;
    float TangentPrecision = 0.0f;
};

struct FMorphTargetLODModel {
    TArray<FMorphTargetDelta> Vertices;
    int32 NumVertices = 0;
    int32 NumBaseMeshVerts = 0;
    TArray<int32> SectionIndices;
    bool bGeneratedByEngine = false;
    FString SourceFilename;

    friend FArchive& operator<<(FArchive& Ar, FMorphTargetLODModel& M);
};

UCLASS()
class UMorphTarget : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USkeletalMesh> BaseSkelMesh = nullptr;

    void Serialize(FArchive& Ar) override;

    const TArray<FMorphTargetLODModel>& GetMorphLODModels() const { return MorphLODModels; }
    const TArray<FMorphTargetCompressedLODModel>& GetCompressedLODModels() const { return CompressedLODModels; }

protected:
    TArray<FMorphTargetLODModel> MorphLODModels;
    TArray<FMorphTargetCompressedLODModel> CompressedLODModels;

private:
    void ScaleRocoKingdomWorldDeltas(FArchive& Ar);
};

}

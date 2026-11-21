#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"
#include "Engine/Source/Runtime/Engine/Rendering/MorphTargetVertexCodec.h"

#include "MorphTarget.generated.h"

namespace ue5parse::runtime {

class USkeletalMesh;

struct FMorphTargetDelta {
    FVector3f PositionDelta{};
    FVector3f TangentZDelta{};
    uint32 SourceIdx = 0;

    friend FArchive& operator<<(FArchive& Ar, FMorphTargetDelta& D);
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

struct FMorphTargetCompressedLODModel {
    TArray<MorphTargetVertexCodec::FDeltaBatchHeader> PackedDeltaHeaders;
    TArray<uint32> PackedDeltaData;
    float PositionPrecision = 0;
    float TangentPrecision = 0;

    friend FArchive& operator<<(FArchive& Ar, FMorphTargetCompressedLODModel& M) {
        return Ar << M.PackedDeltaHeaders << M.PackedDeltaData << M.PositionPrecision << M.TangentPrecision;
    }
};

UCLASS()
class UMorphTarget : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USkeletalMesh> BaseSkelMesh = nullptr;
    TArray<FMorphTargetLODModel> MorphLODModels;
    TArray<FMorphTargetCompressedLODModel> CompressedLODModels;

    void Serialize(FArchive& Ar) override;
};

}

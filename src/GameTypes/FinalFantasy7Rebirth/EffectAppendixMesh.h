#pragma once

#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "EffectAppendixMesh.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FEffectAppendixSkinWeight {
    GENERATED_BODY()
    UPROPERTY()
    TArray<uint16> BoneIndex;
    UPROPERTY()
    TArray<uint16> BoneWeight;
};

UCLASS()
class UEffectAppendixMesh : public UObject {
    GENERATED_BODY()
public:
    explicit UEffectAppendixMesh(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    int32 Version = 0;
    UPROPERTY()
    int32 FullSize = 0;
    UPROPERTY()
    int32 SplinePathLength = 0;
    UPROPERTY()
    int32 TrianglesCount = 0;
    UPROPERTY()
    int32 VerticesCount = 0;
    UPROPERTY()
    float Offset = 0.0f;
    UPROPERTY()
    float TotalTime = 0.0f;
    UPROPERTY()
    TArray<uint16> SplinePath;
    UPROPERTY()
    TArray<float> Times;
    UPROPERTY()
    float Scale1 = 0.0f;
    UPROPERTY()
    float Scale2 = 0.0f;
    UPROPERTY()
    TArray<FEffectAppendixSkinWeight> SkinWeightVertexBuffer;
    UPROPERTY()
    TArray<uint16> IndexBuffer;
    UPROPERTY()
    TArray<FVector3f> PositionVertexBuffer;
};

}

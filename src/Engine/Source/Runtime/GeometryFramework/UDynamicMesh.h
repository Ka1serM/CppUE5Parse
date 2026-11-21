#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicMesh3.h"

#include "UDynamicMesh.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EDynamicMeshChangeType : uint8 { GeneralEdit = 0, MeshChange = 1, MeshReplacementChange = 2, MeshVertexChange = 3, DeformationEdit = 4, AttributeEdit = 5 };

UENUM()
enum class EDynamicMeshAttributeChangeFlags : uint8 {
    Unknown = 0,
    MeshTopology = 1 << 0,
    VertexPositions = 1 << 1,
    NormalsTangents = 1 << 2,
    VertexColors = 1 << 3,
    UVs = 1 << 4,
    TriangleGroups = 1 << 5
};

UCLASS(Abstract)
class UDynamicMeshGenerator : public UObject {
    GENERATED_BODY()
public:
    explicit UDynamicMeshGenerator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UDynamicMesh : public UObject {
    GENERATED_BODY()
public:
    explicit UDynamicMesh(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
    void Serialize(FArchive& Ar) override;

    const FDynamicMesh3& GetMeshRef() const { return Mesh; }

    UPROPERTY()
    TObjectPtr<UDynamicMeshGenerator> MeshGenerator = nullptr;
    UPROPERTY()
    bool bEnableMeshGenerator = false;

private:
    FDynamicMesh3 Mesh;
};

UCLASS(Transient)
class UDynamicMeshPool : public UObject {
    GENERATED_BODY()
public:
    explicit UDynamicMeshPool(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TArray<TObjectPtr<UDynamicMesh>> CachedMeshes;
    UPROPERTY()
    TArray<TObjectPtr<UDynamicMesh>> AllCreatedMeshes;
};

}

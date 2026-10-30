#pragma once

#include <memory>

#include "ue5parse/runtime/Engine/Components/MeshComponent.h"
#include "ue5parse/runtime/Engine/Rendering/ColorVertexBuffer.h"

#include "StaticMeshComponent.generated.h"

namespace ue5parse::runtime {

class UStaticMesh;

// FStaticMeshComponentLODInfo: the per-LOD data of one placed static mesh: its built lighting and its painted vertex
// colors. Serialized natively (UE's Components/StaticMeshComponent.h).
struct FStaticMeshComponentLODInfo {
    FGuid MapBuildDataId;
    // The painted vertex colors; null when the LOD has none. Shared, so that a component copied from its archetype
    // keeps the archetype's colors.
    std::shared_ptr<FColorVertexBuffer> OverrideVertexColors;

    friend FArchive& operator<<(FArchive& Ar, FStaticMeshComponentLODInfo& Info);
};

// UStaticMeshComponent: a placed static mesh (UE's Components/StaticMeshComponent.h).
UCLASS()
class UStaticMeshComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    UStaticMesh* StaticMesh = nullptr;

    void Serialize(FArchive& Ar) override;
    UStaticMesh* GetStaticMesh() const { return StaticMesh; }
    int32 GetNumMaterials() const override;
    UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;

    // The reflected LODData is transient: the cooked data is read natively into this member.
    TArray<FStaticMeshComponentLODInfo> LODData;
};

}

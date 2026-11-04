#pragma once

#include <memory>

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"
#include "Engine/Source/Runtime/Engine/Rendering/ColorVertexBuffer.h"

#include "StaticMeshComponent.generated.h"

namespace ue5parse::runtime {

class UStaticMesh;

struct FStaticMeshComponentLODInfo {
    FGuid MapBuildDataId;
    std::shared_ptr<FColorVertexBuffer> OverrideVertexColors;

    friend FArchive& operator<<(FArchive& Ar, FStaticMeshComponentLODInfo& Info);
};

UCLASS()
class UStaticMeshComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY(ReplicatedUsing = OnRep_StaticMesh)
    TObjectPtr<UStaticMesh> StaticMesh = nullptr;

    void Serialize(FArchive& Ar) override;
    UStaticMesh* GetStaticMesh() const { return StaticMesh; }
    UFUNCTION(BlueprintCallable)
    virtual bool SetStaticMesh(UStaticMesh* NewMesh);
    UFUNCTION()
    void OnRep_StaticMesh(UStaticMesh* OldStaticMesh);
    UFUNCTION(BlueprintCallable)
    void SetForcedLodModel(int32 NewForcedLodModel) {}
    int32 GetNumMaterials() const override;
    UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
    int32 GetMaterialIndex(FName MaterialSlotName) const override;

    TArray<FStaticMeshComponentLODInfo> LODData;
};

}

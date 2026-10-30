#pragma once

#include <memory>

#include "ue5parse/runtime/Engine/Engine/StaticMeshSocket.h"
#include "ue5parse/runtime/Engine/Engine/StreamableRenderAsset.h"
#include "ue5parse/runtime/Engine/MeshUVChannelInfo.h"
#include "ue5parse/runtime/Engine/StaticMeshResources.h"

#include "StaticMesh.generated.h"

namespace ue5parse::runtime {

class UBodySetup;
class UMaterialInterface;

// FStaticMaterial: a material slot of a static mesh.
USTRUCT()
struct FStaticMaterial {
    GENERATED_BODY()
    UPROPERTY()
    UMaterialInterface* MaterialInterface = nullptr;
    UPROPERTY()
    FName MaterialSlotName;
    // Editor-only in UE; cooked data has None.
    UPROPERTY()
    FName ImportedMaterialSlotName;
    UPROPERTY()
    FMeshUVChannelInfo UVChannelData;

    friend FArchive& operator<<(FArchive& Ar, FStaticMaterial& Material);
};

// UStaticMesh (UE's Engine/StaticMesh.h): materials, sockets, and the cooked render data of its LODs.
UCLASS()
class UStaticMesh : public UStreamableRenderAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPerPlatformInt MinLOD;
    UPROPERTY()
    TArray<FStaticMaterial> StaticMaterials;
    UPROPERTY()
    int32 LightMapResolution = 4;
    UPROPERTY()
    int32 LightMapCoordinateIndex = 0;
    UPROPERTY()
    UBodySetup* BodySetup = nullptr;
    UPROPERTY()
    int32 LODForCollision = 0;
    UPROPERTY()
    TArray<UStaticMeshSocket*> Sockets;
    UPROPERTY()
    FVector PositiveBoundsExtension = FVector::ZeroVector;
    UPROPERTY()
    FVector NegativeBoundsExtension = FVector::ZeroVector;
    UPROPERTY()
    FBoxSphereBounds ExtendedBounds;
    UPROPERTY()
    UObject* NavCollision = nullptr;

    void Serialize(FArchive& Ar) override;
    void PostLoad() override;
    // Null when the mesh is not cooked.
    FStaticMeshRenderData* GetRenderData() const { return RenderData.get(); }
    const TArray<FStaticMaterial>& GetStaticMaterials() const { return StaticMaterials; }
    UFUNCTION(BlueprintCallable)
    UMaterialInterface* GetMaterial(int32 MaterialIndex) const;
    UFUNCTION(BlueprintCallable)
    FBoxSphereBounds GetBounds() const { return ExtendedBounds; }
    UFUNCTION(BlueprintCallable)
    int32 GetNumLODs() const { return RenderData ? RenderData->LODResources.Num() : 0; }
    int32 GetNumSections(int32 InLOD) const;
    UStaticMeshSocket* FindSocket(FName InSocketName) const;
    int32 GetMaterialIndex(FName MaterialSlotName) const;
    const FGuid& GetLightingGuid() const { return LightingGuid; }
    bool HasSpeedTreeWind() const { return bHasSpeedTreeWind; }

private:
    void CalculateExtendedBounds();

    std::unique_ptr<FStaticMeshRenderData> RenderData;
    FGuid LightingGuid;
    bool bHasSpeedTreeWind = false;
};

}

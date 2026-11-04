#pragma once

#include <memory>

#include "Engine/Source/Runtime/Engine/Engine/StaticMeshSocket.h"
#include "Engine/Source/Runtime/Engine/Engine/StreamableRenderAsset.h"
#include "Engine/Source/Runtime/Engine/MeshUVChannelInfo.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

#include "StaticMesh.generated.h"

namespace ue5parse::runtime {

class UBodySetup;
class UMaterialInterface;

USTRUCT()
struct FStaticMaterial {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UMaterialInterface> MaterialInterface = nullptr;
    UPROPERTY()
    FName MaterialSlotName;
    UPROPERTY()
    FName ImportedMaterialSlotName;
    UPROPERTY()
    FMeshUVChannelInfo UVChannelData;

    friend FArchive& operator<<(FArchive& Ar, FStaticMaterial& Material);
};

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
    TObjectPtr<UBodySetup> BodySetup = nullptr;
    UPROPERTY()
    int32 LODForCollision = 0;
    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshSocket>> Sockets;
    UPROPERTY()
    FVector PositiveBoundsExtension = FVector::ZeroVector;
    UPROPERTY()
    FVector NegativeBoundsExtension = FVector::ZeroVector;
    UPROPERTY()
    FBoxSphereBounds ExtendedBounds;
    UPROPERTY()
    TObjectPtr<UObject> NavCollision = nullptr;

    void Serialize(FArchive& Ar) override;
    void PostLoad() override;
    FStaticMeshRenderData* GetRenderData() const { return RenderData.get(); }
    UFUNCTION(BlueprintPure)
    const TArray<FStaticMaterial>& GetStaticMaterials() const { return StaticMaterials; }
    UFUNCTION(BlueprintCallable)
    UMaterialInterface* GetMaterial(int32 MaterialIndex) const;
    UFUNCTION(BlueprintCallable)
    FBoxSphereBounds GetBounds() const { return ExtendedBounds; }
    UFUNCTION(BlueprintCallable)
    int32 GetNumLODs() const { return RenderData ? RenderData->LODResources.Num() : 0; }
    UStaticMeshSocket* FindSocket(FName InSocketName) const;
    int32 GetMaterialIndex(FName MaterialSlotName) const;

private:
    void CalculateExtendedBounds();

    std::unique_ptr<FStaticMeshRenderData> RenderData;
    FGuid LightingGuid;
    bool bHasSpeedTreeWind = false;
};

}

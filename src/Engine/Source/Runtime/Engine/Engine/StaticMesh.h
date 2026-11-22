#pragma once

#include <memory>

#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/Engine/Engine/StaticMeshSocket.h"
#include "Engine/Source/Runtime/Engine/Engine/StreamableRenderAsset.h"
#include "Engine/Source/Runtime/Engine/MeshUVChannelInfo.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

#include "StaticMesh.generated.h"

namespace ue5parse::runtime {

class UBodySetup;
class UMaterialInterface;
class UNavCollisionBase;

UENUM()
enum class EStaticMeshPaintSupport : uint8 {
    Default,
    Enabled,
    Disabled,
};

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
    UPROPERTY()
    TObjectPtr<UMaterialInterface> OverlayMaterialInterface = nullptr;

    friend FArchive& operator<<(FArchive& Ar, FStaticMaterial& Material);
};

UCLASS()
class UStaticMesh : public UStreamableRenderAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> Materials;
    UPROPERTY()
    int32 LightMapResolution = 4;
    UPROPERTY()
    int32 LightMapCoordinateIndex = 0;
    UPROPERTY()
    TObjectPtr<UBodySetup> BodySetup = nullptr;
    UPROPERTY()
    bool UseFullPrecisionUVs_DEPRECATED = false;
    UPROPERTY()
    bool bUseMaximumStreamingTexelRatio = false;
    UPROPERTY()
    bool bStripComplexCollisionForConsole_DEPRECATED = false;
    UPROPERTY()
    bool bHasNavigationData = true;
    UPROPERTY()
    float StreamingDistanceMultiplier = 1.0f;
    UPROPERTY()
    float LpvBiasMultiplier = 1.0f;
    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshSocket>> Sockets;
    UPROPERTY()
    int32 ElementToIgnoreForTexFactor = -1;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;
    UPROPERTY()
    TObjectPtr<UNavCollisionBase> NavCollision = nullptr;
    UPROPERTY()
    FPerPlatformInt MinLOD;
    UPROPERTY()
    FVector PositiveBoundsExtension = FVector::ZeroVector;
    UPROPERTY()
    FVector NegativeBoundsExtension = FVector::ZeroVector;
    UPROPERTY()
    FBoxSphereBounds ExtendedBounds;
    UPROPERTY()
    int32 LODForCollision = 0;
    UPROPERTY()
    bool bAllowCPUAccess = false;
    UPROPERTY()
    TArray<FStaticMaterial> StaticMaterials;
    UPROPERTY()
    float LightmapUVDensity = 0;
    UPROPERTY()
    bool bGenerateMeshDistanceField = false;
    UPROPERTY()
    bool bLODsShareStaticLighting = false;
    UPROPERTY()
    float DistanceFieldSelfShadowBias = 0;
    UPROPERTY()
    bool bRequiresAreaWeightedSampling = false;
    UPROPERTY()
    bool bSupportUniformlyDistributedSampling = false;
    UPROPERTY()
    TObjectPtr<UObject> EditableMesh = nullptr;
    UPROPERTY()
    bool bSupportGpuUniformlyDistributedSampling = false;
    UPROPERTY()
    TObjectPtr<UStaticMesh> ComplexCollisionMesh = nullptr;
    UPROPERTY()
    bool bIsBuiltAtRuntime = false;
    UPROPERTY()
    bool bSupportPhysicalMaterialMasks = false;
    UPROPERTY()
    bool bSupportRayTracing = true;
    UPROPERTY()
    FPerQualityLevelInt MinQualityLevelLOD;
    UPROPERTY()
    bool bDoFastBuild = false;
    UPROPERTY()
    EStaticMeshPaintSupport StaticMeshPaintSupport = EStaticMeshPaintSupport::Default;
    UPROPERTY()
    int32 MeshPaintTextureCoordinateIndex = 0;
    UPROPERTY()
    int32 MeshPaintTextureResolution = 0;

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
    UFUNCTION(BlueprintCallable)
    int32 GetNumNaniteVertices() const;
    UFUNCTION(BlueprintCallable)
    int32 GetNumNaniteTriangles() const;
    UStaticMeshSocket* FindSocket(FName InSocketName) const;
    int32 GetMaterialIndex(FName MaterialSlotName) const;

private:
    void ConvertLegacyMaterials();
    static void SkipOccluderData(FArchive& Ar);

    std::unique_ptr<FStaticMeshRenderData> RenderData;
    FGuid LightingGuid;
    bool bHasSpeedTreeWind = false;
};

}

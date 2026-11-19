#pragma once

#include <memory>

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/TextureStreamingTypes.h"
#include "Engine/Source/Runtime/Engine/Rendering/ColorVertexBuffer.h"

#include "StaticMeshComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialCacheVirtualTexture;
class UNavAreaBase;
class UStaticMesh;
class UTexture;

struct FStaticMeshComponentLODInfo {
    FGuid MapBuildDataId;
    FGuid OriginalMapBuildDataId;
    std::shared_ptr<FColorVertexBuffer> OverrideVertexColors;

    friend FArchive& operator<<(FArchive& Ar, FStaticMeshComponentLODInfo& Info);
};

UCLASS()
class UStaticMeshComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 ForcedLodModel = 0;
    UPROPERTY()
    int32 MinLOD = 0;
    UPROPERTY()
    int32 SubDivisionStepSize = 32;
    UPROPERTY()
    FColor WireframeColorOverride = FColor(255, 255, 255, 255);
    UPROPERTY(ReplicatedUsing = OnRep_StaticMesh)
    TObjectPtr<UStaticMesh> StaticMesh = nullptr;
    UPROPERTY()
    int32 WorldPositionOffsetDisableDistance = 0;
    UPROPERTY()
    uint8 bForceNaniteForMasked : 1 = false;
    UPROPERTY()
    uint8 bDisallowNanite : 1 = false;
    UPROPERTY()
    uint8 bForceDisableNanite : 1 = false;
    UPROPERTY()
    uint8 bEvaluateWorldPositionOffset : 1 = true;
    UPROPERTY()
    uint8 bWorldPositionOffsetWritesVelocity : 1 = true;
    UPROPERTY()
    uint8 bEvaluateWorldPositionOffsetInRayTracing : 1 = false;
    UPROPERTY()
    uint8 bOverrideWireframeColor : 1 = 0;
    UPROPERTY()
    uint8 bOverrideMinLOD : 1 = 0;
    UPROPERTY()
    uint8 bOverrideNavAreaClass : 1 = false;
    UPROPERTY()
    TSubclassOf<UNavAreaBase> NavAreaClass;
    UPROPERTY()
    uint8 bDisallowMeshPaintPerInstance_DEPRECATED : 1 = 0;
    UPROPERTY()
    uint8 bIgnoreInstanceForTextureStreaming : 1 = 0;
    UPROPERTY()
    uint8 bOverrideLightMapRes : 1 = false;
    UPROPERTY()
    uint8 bCastDistanceFieldIndirectShadow : 1 = 0;
    UPROPERTY()
    uint8 bOverrideDistanceFieldSelfShadowBias : 1 = 0;
    UPROPERTY()
    uint8 bUseSubDivisions : 1 = true;
    UPROPERTY()
    uint8 bUseDefaultCollision : 1 = false;
    UPROPERTY()
    int32 PreviousLODLevel_DEPRECATED = 0;
    UPROPERTY()
    uint8 bSortTriangles : 1 = 0;
    UPROPERTY()
    uint8 bReverseCulling : 1 = 0;
    UPROPERTY()
    uint8 bEnableVertexColorMeshPainting : 1 = true;
    UPROPERTY()
    uint8 bEnableTextureColorMeshPainting : 1 = true;
    UPROPERTY()
    uint8 bWritesToMaterialCache : 1 = 0;
    UPROPERTY()
    uint8 bOverrideMeshPaintTextureCoordinateIndex : 1 = 0;
    UPROPERTY()
    uint8 bOverrideMeshPaintTextureResolution : 1 = 0;
    UPROPERTY()
    int32 OverriddenMeshPaintTextureCoordinateIndex = 0;
    UPROPERTY()
    int32 OverriddenMeshPaintTextureResolution = 0;
    UPROPERTY()
    int32 OverriddenLightMapRes = 64;
    UPROPERTY()
    FIntPoint MaterialCacheTileCount = FIntPoint(8, 8);
    UPROPERTY()
    FBox2f MaterialCacheUVRegion = FBox2f(FVector2f(0, 0), FVector2f(1, 1));
    UPROPERTY()
    int32 MaterialCacheUVCoordinateIndex = 0;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialCacheVirtualTexture>> MaterialCacheTextures;
    UPROPERTY()
    float DistanceFieldIndirectShadowMinVisibility = .1f;
    UPROPERTY()
    float DistanceFieldSelfShadowBias = 0;
    UPROPERTY()
    float StreamingDistanceMultiplier = 1.0f;
    UPROPERTY()
    float NanitePixelProgrammableDistance = 0;
    UPROPERTY()
    TArray<FGuid> IrrelevantLights_DEPRECATED;
    UPROPERTY()
    TArray<FStreamingTextureBuildInfo> StreamingTextureData;
    UPROPERTY()
    FLightmassPrimitiveSettings LightmassSettings;
    UPROPERTY(SkipSerialization)
    TObjectPtr<UTexture> MeshPaintTextureCooked = nullptr;

    explicit UStaticMeshComponent(const FObjectInitializer& ObjectInitializer);

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

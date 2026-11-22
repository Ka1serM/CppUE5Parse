#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "BaseDynamicMeshComponent.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EDynamicMeshComponentTangentsMode : uint8 { NoTangents, AutoCalculated, ExternallyProvided, Default = 255 };

UENUM()
enum class EDynamicMeshComponentColorOverrideMode : uint8 { None, VertexColors, Polygroups, Constant };

UENUM()
enum class EDynamicMeshDrawPath : uint8 { DynamicDraw = 0, StaticDraw = 1 };

UENUM()
enum class EDynamicMeshComponentDistanceFieldMode : uint8 { NoDistanceField = 0, AsyncCPUDistanceField = 1 };

UENUM()
enum class EDynamicMeshVertexColorTransformMode : uint8 { NoTransform = 0, LinearToSRGB, SRGBToLinear };

UCLASS(Abstract)
class UBaseDynamicMeshComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UBaseDynamicMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    int32 GetNumMaterials() const override { return BaseMaterials.Num(); }
    UMaterialInterface* GetMaterial(int32 ElementIndex) const override {
        return BaseMaterials.IsValidIndex(ElementIndex) ? BaseMaterials[ElementIndex].Get() : nullptr;
    }

    UPROPERTY()
    bool bExplicitShowWireframe = false;
    UPROPERTY()
    FLinearColor WireframeColor = FLinearColor(0, 0.5f, 1.f, 1.f);
    UPROPERTY()
    EDynamicMeshComponentColorOverrideMode ColorMode = EDynamicMeshComponentColorOverrideMode::None;
    UPROPERTY()
    FColor ConstantColor = FColor(255, 255, 255, 255);
    UPROPERTY()
    EDynamicMeshVertexColorTransformMode ColorSpaceMode = EDynamicMeshVertexColorTransformMode::NoTransform;
    UPROPERTY()
    bool bTwoSided = false;
    UPROPERTY()
    bool bEnableFlatShading = false;
    UPROPERTY()
    bool bEnableViewModeOverrides = true;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> OverrideRenderMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> SecondaryRenderMaterial = nullptr;
    UPROPERTY()
    bool bDrawSecondaryBuffers = true;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> WireframeMaterialOverride = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> SecondaryWireframeMaterialOverride = nullptr;
    UPROPERTY()
    bool bEnableRaytracing = true;
    UPROPERTY()
    EDynamicMeshDrawPath DrawPath = EDynamicMeshDrawPath::DynamicDraw;
    UPROPERTY()
    EDynamicMeshComponentDistanceFieldMode DistanceFieldMode_DEPRECATED = EDynamicMeshComponentDistanceFieldMode::NoDistanceField;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> BaseMaterials;
};

}

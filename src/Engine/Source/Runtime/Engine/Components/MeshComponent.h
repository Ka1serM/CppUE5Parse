#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "MeshComponent.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UMeshComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> OverrideMaterials;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> OverlayMaterial = nullptr;
    UPROPERTY()
    float OverlayMaterialMaxDrawDistance = 0;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> MaterialSlotsOverlayMaterial;
    UPROPERTY()
    uint8 bEnableMaterialParameterCaching : 1 = false;

    explicit UMeshComponent(const FObjectInitializer& ObjectInitializer);

    UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
    void SetMaterial(int32 ElementIndex, UMaterialInterface* Material) override;
    void SetMaterialByName(FName MaterialSlotName, UMaterialInterface* Material) override;
    UFUNCTION(BlueprintCallable)
    virtual TArray<UMaterialInterface*> GetMaterials() const;
    UFUNCTION(BlueprintCallable)
    virtual int32 GetMaterialIndex(FName MaterialSlotName) const { return INDEX_NONE; }
    UFUNCTION(BlueprintCallable)
    void SetScalarParameterValueOnMaterials(const FName ParameterName, const float ParameterValue);
    UFUNCTION(BlueprintCallable)
    void SetVectorParameterValueOnMaterials(const FName ParameterName, const FVector ParameterValue);
    float GetScalarParameterDefaultValue(FName ParameterName) const;
    UFUNCTION(BlueprintCallable)
    void EmptyOverrideMaterials() { OverrideMaterials.Reset(); }
};

}

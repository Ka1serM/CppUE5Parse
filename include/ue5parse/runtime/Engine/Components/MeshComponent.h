#pragma once

#include "ue5parse/runtime/Engine/Components/PrimitiveComponent.h"

#include "MeshComponent.generated.h"

namespace ue5parse::runtime {

// UMeshComponent: a primitive with material slots that override its mesh's (UE's Components/MeshComponent.h).
UCLASS(Abstract)
class UMeshComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> OverrideMaterials;

    UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
    void SetMaterial(int32 ElementIndex, UMaterialInterface* Material) override;
    void SetMaterialByName(FName MaterialSlotName, UMaterialInterface* Material) override;
    UFUNCTION(BlueprintCallable)
    virtual TArray<UMaterialInterface*> GetMaterials() const;
    // The slot of a name; the mesh components that have named slots override it.
    UFUNCTION(BlueprintCallable)
    virtual int32 GetMaterialIndex(FName MaterialSlotName) const { return INDEX_NONE; }
    // Sets the parameter on the slots whose material has it, each as a dynamic instance that the slot takes.
    UFUNCTION(BlueprintCallable)
    void SetScalarParameterValueOnMaterials(const FName ParameterName, const float ParameterValue);
    UFUNCTION(BlueprintCallable)
    void SetVectorParameterValueOnMaterials(const FName ParameterName, const FVector ParameterValue);
    // The default of the parameter in the last slot whose material has it, without the instances' overrides; 0 when no
    // slot has it.
    float GetScalarParameterDefaultValue(FName ParameterName) const;
    UFUNCTION(BlueprintCallable)
    void EmptyOverrideMaterials() { OverrideMaterials.Reset(); }
};

}

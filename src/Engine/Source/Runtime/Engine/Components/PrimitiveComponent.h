#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "PrimitiveComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPhysicalMaterial;

UCLASS(Abstract)
class UPrimitiveComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bIsFirstPerson = false;
    UPROPERTY()
    bool bRenderInMainPass = true;
    UPROPERTY()
    bool CastShadow = true;
    virtual int32 GetNumMaterials() const { return 0; }
    UFUNCTION(BlueprintPure)
    virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const { return nullptr; }
    UFUNCTION(BlueprintCallable)
    virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* Material) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetMaterialByName(FName MaterialSlotName, UMaterialInterface* Material) {}
    UFUNCTION(BlueprintCallable)
    virtual UMaterialInstanceDynamic* CreateDynamicMaterialInstance(int32 ElementIndex, UMaterialInterface* SourceMaterial = nullptr,
                                                                    FName OptionalName = FName());
    UFUNCTION(BlueprintCallable)
    virtual void PrestreamTextures(float Seconds, bool bPrioritizeCharacterTextures, int32 CinematicTextureGroups) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetCollisionEnabled(TEnumAsByte<ECollisionEnabled::Type> NewType) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetPhysMaterialOverride(UPhysicalMaterial* NewPhysMaterial) {}
    UFUNCTION(BlueprintCallable)
    void SetTranslucentSortPriority(int32 NewTranslucentSortPriority) {}
    UFUNCTION(BlueprintCallable)
    void SetCollisionResponseToChannel(TEnumAsByte<ECollisionChannel> Channel, TEnumAsByte<ECollisionResponse> NewResponse) {}
    bool IsAnySimulatingPhysics() const { return false; }
    void AddImpulseAtLocation(FVector Impulse, FVector Location, FName BoneName = NAME_None) {}
};

}

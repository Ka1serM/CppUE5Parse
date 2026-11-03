#pragma once

#include "ue5parse/runtime/Engine/Components/SceneComponent.h"
#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"

#include "PrimitiveComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPhysicalMaterial;

// UPrimitiveComponent: a scene component with geometry that is rendered or collides (UE's
// Components/PrimitiveComponent.h).
UCLASS(Abstract)
class UPrimitiveComponent : public USceneComponent {
    GENERATED_BODY()
public:
    // VALORANT's addition: the component shows in the first-person view only.
    UPROPERTY()
    bool bIsFirstPerson = false;
    // The component draws in the main view; without it, only in depth, shadows, or another view.
    UPROPERTY()
    bool bRenderInMainPass = true;
    // The component casts shadows of the lights that cast them.
    UPROPERTY()
    bool CastShadow = true;
    // The number of material slots; the base class has none.
    virtual int32 GetNumMaterials() const { return 0; }
    // The material of a slot: an override, else the mesh's.
    UFUNCTION(BlueprintPure)
    virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const { return nullptr; }
    UFUNCTION(BlueprintCallable)
    virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* Material) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetMaterialByName(FName MaterialSlotName, UMaterialInterface* Material) {}
    // The slot's material as a dynamic instance: the slot's own when it is one, else a new one that the slot takes.
    UFUNCTION(BlueprintCallable)
    virtual UMaterialInstanceDynamic* CreateDynamicMaterialInstance(int32 ElementIndex, UMaterialInterface* SourceMaterial = nullptr,
                                                                    FName OptionalName = FName());
    // Streams the textures' mips in for a time. Without texture streaming every mip is there when it is read.
    UFUNCTION(BlueprintCallable)
    virtual void PrestreamTextures(float Seconds, bool bPrioritizeCharacterTextures, int32 CinematicTextureGroups) {}
    // Collision and physics state, which the runtime does not simulate.
    UFUNCTION(BlueprintCallable)
    virtual void SetCollisionEnabled(TEnumAsByte<ECollisionEnabled::Type> NewType) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetPhysMaterialOverride(UPhysicalMaterial* NewPhysMaterial) {}
    // The order of translucent primitives, which the scene does not sort by.
    UFUNCTION(BlueprintCallable)
    void SetTranslucentSortPriority(int32 NewTranslucentSortPriority) {}
    UFUNCTION(BlueprintCallable)
    void SetCollisionResponseToChannel(TEnumAsByte<ECollisionChannel> Channel, TEnumAsByte<ECollisionResponse> NewResponse) {}
    // The runtime simulates no rigid bodies.
    bool IsAnySimulatingPhysics() const { return false; }
    void AddImpulseAtLocation(FVector Impulse, FVector Location, FName BoneName = NAME_None) {}
};

}

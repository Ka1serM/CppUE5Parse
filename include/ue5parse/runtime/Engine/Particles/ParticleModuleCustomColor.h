#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleCustomColor.generated.h"

namespace ue5parse::runtime {

class UMaterialInstanceDynamic;

// Riot's: a module that colors the emitter's material through a dynamic material instance, which the emitter instance
// makes when it initializes (VALORANT's Engine module).
UCLASS(abstract)
class UParticleModuleCustomColor : public UParticleModule {
    GENERATED_BODY()
public:
    explicit UParticleModuleCustomColor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FName MaterialParameter;

    virtual void ApplyColor(UParticleSystemComponent* Component, UMaterialInstanceDynamic* MID) {}
    virtual FLinearColor GetColor(UParticleSystemComponent* Component) const { return FLinearColor(); }
};

// Riot's: the color of the skin variant that the component shows (UParticleSystemComponent::ParticleVariantColor).
UCLASS()
class UAresParticleModuleVariantColor : public UParticleModuleCustomColor {
    GENERATED_BODY()
public:
    explicit UAresParticleModuleVariantColor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FName VariantParameter = FName("LUT_Variant");
    UPROPERTY()
    FLinearColor BaseColor = FLinearColor(1, 1, 1, 1);
    UPROPERTY()
    FLinearColor Variant1Color = FLinearColor(1, 1, 1, 1);
    UPROPERTY()
    FLinearColor Variant2Color = FLinearColor(1, 1, 1, 1);
    UPROPERTY()
    FLinearColor Variant3Color = FLinearColor(1, 1, 1, 1);
    UPROPERTY()
    FLinearColor VariantRadiantColor = FLinearColor(1, 1, 1, 1);

    void ApplyColor(UParticleSystemComponent* Component, UMaterialInstanceDynamic* MID) override;
    FLinearColor GetColor(UParticleSystemComponent* Component) const override;
};

UENUM()
enum class EAresOutlineMode : uint8 { None, Outline, Block, Enemy, AlwaysOutline, AlwaysEnemy };

// Riot's: a module that sets the outline mode of the mesh particles (VALORANT's Engine module). The outline is not
// exported, so the modes only load.
UCLASS(abstract)
class UParticleModuleCustomOutline : public UParticleModule {
    GENERATED_BODY()
};

}

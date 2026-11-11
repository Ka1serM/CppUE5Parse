#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleCustomColor.generated.h"

namespace ue5parse::runtime {

class UMaterialInstanceDynamic;

UENUM()
enum class EAresParticleVariantColor : uint8 {
    AresVariantBaseColor,
    AresVariant1Color,
    AresVariant2Color,
    AresVariant3Color,
    AresVariant4Color,
};

UCLASS(abstract)
class UParticleModuleCustomColor : public UParticleModule {
    GENERATED_BODY()
public:
    explicit UParticleModuleCustomColor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FName MaterialParameter;

    bool IsCustomColor() const override { return true; }
    void ApplyCustomColor(UParticleSystemComponent* Component, UMaterialInstanceDynamic* MID) override { ApplyColor(Component, MID); }

    virtual void ApplyColor(UParticleSystemComponent* Component, UMaterialInstanceDynamic* MID) {}
    virtual FLinearColor GetColor(UParticleSystemComponent* Component) const { return FLinearColor(); }
};

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
    FLinearColor Variant4Color = FLinearColor(1, 1, 1, 1);

    void ApplyColor(UParticleSystemComponent* Component, UMaterialInstanceDynamic* MID) override;
    FLinearColor GetColor(UParticleSystemComponent* Component) const override;
};

UENUM()
enum class EAresOutlineMode : uint8 { None, Outline, Block, Enemy, AlwaysOutline, AlwaysEnemy };

UCLASS(abstract)
class UParticleModuleCustomOutline : public UParticleModule {
    GENERATED_BODY()
};

}

#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleCustomColor.h"

#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInstanceDynamic.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

UParticleModuleCustomColor::UParticleModuleCustomColor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
}

UAresParticleModuleVariantColor::UAresParticleModuleVariantColor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    MaterialParameter = FName("Variant Color");
}

static EAresParticleVariantColor GetComponentVariant(UParticleSystemComponent* Component) {
    const UWorld* World = Component->GetWorld();
    if (World == nullptr)
        return EAresParticleVariantColor::AresVariantBaseColor;
    const EWorldType::Type Type = World->WorldType;
    if (Type != EWorldType::Game && Type != EWorldType::PIE && Type != EWorldType::GamePreview && Type != EWorldType::GameRPC)
        return EAresParticleVariantColor::AresVariantBaseColor;
    return static_cast<EAresParticleVariantColor>(Component->ParticleVariantColor);
}

FLinearColor UAresParticleModuleVariantColor::GetColor(UParticleSystemComponent* Component) const {
    switch (GetComponentVariant(Component)) {
    case EAresParticleVariantColor::AresVariantBaseColor:
        return BaseColor;
    case EAresParticleVariantColor::AresVariant1Color:
        return Variant1Color;
    case EAresParticleVariantColor::AresVariant2Color:
        return Variant2Color;
    case EAresParticleVariantColor::AresVariant3Color:
        return Variant3Color;
    case EAresParticleVariantColor::AresVariant4Color:
        return Variant4Color;
    }
    return FLinearColor();
}

void UAresParticleModuleVariantColor::ApplyColor(UParticleSystemComponent* Component, UMaterialInstanceDynamic* MID) {
    if (Component == nullptr || MID == nullptr)
        return;
    MID->SetVectorParameterValueByInfo(FMaterialParameterInfo(MaterialParameter, GlobalParameter), GetColor(Component));
    MID->SetScalarParameterValueByInfo(FMaterialParameterInfo(VariantParameter, GlobalParameter), float(int8(GetComponentVariant(Component))));
}

}

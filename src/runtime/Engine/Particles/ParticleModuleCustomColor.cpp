#include "ue5parse/runtime/Engine/Particles/ParticleModuleCustomColor.h"

#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/Materials/MaterialInstanceDynamic.h"
#include "ue5parse/runtime/Engine/Particles/ParticleSystemComponent.h"

// VALORANT's UParticleModuleCustomColor and UAresParticleModuleVariantColor (from the game's binary).

namespace ue5parse::runtime {

UParticleModuleCustomColor::UParticleModuleCustomColor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
}

UAresParticleModuleVariantColor::UAresParticleModuleVariantColor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    MaterialParameter = FName("Variant Color");
}

// The component's variant counts only in a game world (Game, PIE, GamePreview and GameRPC); elsewhere it is the base.
static EAresParticleVariantColor GetComponentVariant(UParticleSystemComponent* Component) {
    const UWorld* World = Component->GetWorld();
    if (World == nullptr) return EAresParticleVariantColor::AresVariantBaseColor;
    const EWorldType::Type Type = World->WorldType;
    if (Type != EWorldType::Game && Type != EWorldType::PIE && Type != EWorldType::GamePreview && Type != EWorldType::GameRPC)
        return EAresParticleVariantColor::AresVariantBaseColor;
    return Component->ParticleVariantColor;
}

FLinearColor UAresParticleModuleVariantColor::GetColor(UParticleSystemComponent* Component) const {
    switch (GetComponentVariant(Component)) {
    case EAresParticleVariantColor::AresVariantBaseColor: return BaseColor;
    case EAresParticleVariantColor::AresVariant1Color: return Variant1Color;
    case EAresParticleVariantColor::AresVariant2Color: return Variant2Color;
    case EAresParticleVariantColor::AresVariant3Color: return Variant3Color;
    case EAresParticleVariantColor::AresVariant4Color: return VariantRadiantColor;
    }
    return FLinearColor();
}

void UAresParticleModuleVariantColor::ApplyColor(UParticleSystemComponent* Component, UMaterialInstanceDynamic* MID) {
    if (Component == nullptr || MID == nullptr) return;
    MID->SetVectorParameterValueByInfo(FMaterialParameterInfo(MaterialParameter, GlobalParameter), GetColor(Component));
    MID->SetScalarParameterValueByInfo(FMaterialParameterInfo(VariantParameter, GlobalParameter), float(int8(GetComponentVariant(Component))));
}

}

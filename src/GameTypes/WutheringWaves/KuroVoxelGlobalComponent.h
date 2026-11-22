#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "KuroVoxelGlobalComponent.generated.h"

namespace ue5parse::runtime {

//! CUE4Parse reads nothing of this Wuthering Waves component.
UCLASS()
class UKuroVoxelGlobalComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UKuroVoxelGlobalComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override {
        if (Ar.GetGame() != EGame::GAME_WutheringWaves && Ar.GetGame() != EGame::GAME_WutheringWavesFastGeo)
            Super::Serialize(Ar);
    }
};

}

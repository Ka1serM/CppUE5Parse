#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "EntityLayout.generated.h"

namespace ue5parse::runtime {

//! Dune: Awakening saves an entity layout with no tagged properties.
UCLASS()
class UEntityLayout : public UObject {
    GENERATED_BODY()
public:
    explicit UEntityLayout(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override {
        if (Ar.GetGame() != EGame::GAME_DuneAwakening) {
            Super::Serialize(Ar);
            return;
        }
        Ar << m_FlatLayout << m_ParentLayout;
    }

    UPROPERTY()
    TObjectPtr<UObject> m_FlatLayout = nullptr;
    UPROPERTY()
    FSoftObjectPath m_ParentLayout;
};

}

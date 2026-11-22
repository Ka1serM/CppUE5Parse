#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "UniqueID.generated.h"

namespace ue5parse::runtime {

//! Conan Exiles Enhanced saves only the 64 bit ID, without the properties.
UCLASS()
class UUniqueID : public UObject {
    GENERATED_BODY()
public:
    explicit UUniqueID(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override {
        if (Ar.GetGame() != EGame::GAME_ConanExilesEnhanced) {
            Super::Serialize(Ar);
            return;
        }
        Ar << ID;
    }

    UPROPERTY()
    int64 ID = 0;
};

}

#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "SimpleBox.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSimpleBox {
    GENERATED_BODY()
    UPROPERTY()
    FVector3f A;
    UPROPERTY()
    FVector3f B;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_SuicideSquad)
            return false;
        Ar << A << B;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FSimpleBox> : TStructOpsTypeTraitsBase2<FSimpleBox> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FRRotationTranslation {
    GENERATED_BODY()
    UPROPERTY()
    FQuat4f Rotation;
    UPROPERTY()
    FVector3f Translation;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_SuicideSquad)
            return false;
        Ar << Rotation << Translation;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FRRotationTranslation> : TStructOpsTypeTraitsBase2<FRRotationTranslation> {
    enum { WithSerializer = true };
};

}

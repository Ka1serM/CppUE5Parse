#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ActorReference.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FActorReference {
    GENERATED_BODY()
    UPROPERTY()
    bool bIsAlias = false;
    UPROPERTY()
    FString ActorName;
    UPROPERTY()
    uint8 Type = 0;
    UPROPERTY()
    uint8 SelectedType = 0;

    static bool IsSupermassiveGame(EGame Game) {
        switch (Game) {
        case EGame::GAME_DarkPicturesAnthologyHouseOfAshes:
        case EGame::GAME_DarkPicturesAnthologyManofMedan:
        case EGame::GAME_DarkPicturesAnthologyLittleHope:
        case EGame::GAME_DarkPicturesAnthologyTheDevilinMe:
        case EGame::GAME_TheQuarry:
        case EGame::GAME_TheCastingofFrankStone:
        case EGame::GAME_Directive8020:
            return true;
        default:
            return false;
        }
    }

    bool Serialize(FArchive& Ar) {
        if (!IsSupermassiveGame(Ar.GetGame()))
            return false;
        Ar << bIsAlias << ActorName << Type << SelectedType;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FActorReference> : TStructOpsTypeTraitsBase2<FActorReference> {
    enum { WithSerializer = true };
};

}

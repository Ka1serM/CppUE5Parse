#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "TQ2Structs.generated.h"

namespace ue5parse::runtime {

//! Titan Quest II saves these asset pointers as soft object paths.
USTRUCT()
struct FGrimItemDescriptionPtr {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath Path;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_TitanQuest2)
            return false;
        Ar << Path;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FGrimItemDescriptionPtr> : TStructOpsTypeTraitsBase2<FGrimItemDescriptionPtr> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FGrimAbilityProjectileParametersPtr {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath Path;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_TitanQuest2)
            return false;
        Ar << Path;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FGrimAbilityProjectileParametersPtr> : TStructOpsTypeTraitsBase2<FGrimAbilityProjectileParametersPtr> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FSoftJsonDataAssetPtr {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath Path;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_TitanQuest2)
            return false;
        Ar << Path;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FSoftJsonDataAssetPtr> : TStructOpsTypeTraitsBase2<FSoftJsonDataAssetPtr> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FGrimAbilitySummonDataPtr {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath Path;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_TitanQuest2)
            return false;
        Ar << Path;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FGrimAbilitySummonDataPtr> : TStructOpsTypeTraitsBase2<FGrimAbilitySummonDataPtr> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FJsonDataAssetPath {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath Path;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_TitanQuest2)
            return false;
        Ar << Path;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FJsonDataAssetPath> : TStructOpsTypeTraitsBase2<FJsonDataAssetPath> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FJsonDataAssetPtr {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath Path;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_TitanQuest2)
            return false;
        Ar << Path;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FJsonDataAssetPtr> : TStructOpsTypeTraitsBase2<FJsonDataAssetPtr> {
    enum { WithSerializer = true };
};

//! Titan Quest II saves a type byte and then a bool or two integers, and has no other value types.
USTRUCT()
struct FGrimDialogueVariableValue {
    GENERATED_BODY()
    UPROPERTY()
    uint8 Type = 0;
    UPROPERTY()
    bool bValue = false;
    UPROPERTY()
    int32 IntValue = 0;
    UPROPERTY()
    int32 IntExtra = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_TitanQuest2)
            return false;
        Ar << Type;
        if (Type == 0)
            Ar << bValue;
        else if (Type == 1)
            Ar << IntValue << IntExtra;
        else
            Ar.SetError(ErrorCode::InvalidData, "FGrimDialogueVariableValue.Type");
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FGrimDialogueVariableValue> : TStructOpsTypeTraitsBase2<FGrimDialogueVariableValue> {
    enum { WithSerializer = true };
};

}

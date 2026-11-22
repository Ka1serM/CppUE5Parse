#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "DuneStructs.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FBotAutoBorderCrossingConfig {
    GENERATED_BODY()
    UPROPERTY()
    uint8 m_bEnabled = 0;
    UPROPERTY()
    float m_DetectBorderDistance = 0;
    UPROPERTY()
    float m_JumpOverBorderDistance = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_DuneAwakening)
            return false;
        Ar << m_bEnabled << m_DetectBorderDistance << m_JumpOverBorderDistance;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FBotAutoBorderCrossingConfig> : TStructOpsTypeTraitsBase2<FBotAutoBorderCrossingConfig> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FGenericTeamId {
    GENERATED_BODY()
    UPROPERTY()
    uint8 TeamID = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_DuneAwakening)
            return false;
        Ar << TeamID;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FGenericTeamId> : TStructOpsTypeTraitsBase2<FGenericTeamId> {
    enum { WithSerializer = true };
};

//! Dune: Awakening saves the string enum value as its name.
USTRUCT()
struct FStringEnumValue {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_DuneAwakening)
            return false;
        Ar << Name;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FStringEnumValue> : TStructOpsTypeTraitsBase2<FStringEnumValue> {
    enum { WithSerializer = true };
};

//! Dune: Awakening saves the persistence IDs as one 64 bit value.
USTRUCT()
struct FPersistenceId {
    GENERATED_BODY()
    UPROPERTY()
    int64 UID = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_DuneAwakening)
            return false;
        Ar << UID;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FPersistenceId> : TStructOpsTypeTraitsBase2<FPersistenceId> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPersistence_DEPRECATED_Id : public FPersistenceId {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FPersistence_DEPRECATED_Id> : TStructOpsTypeTraitsBase2<FPersistence_DEPRECATED_Id> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPersistenceActorId : public FPersistenceId {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FPersistenceActorId> : TStructOpsTypeTraitsBase2<FPersistenceActorId> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPersistenceItemId : public FPersistenceId {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FPersistenceItemId> : TStructOpsTypeTraitsBase2<FPersistenceItemId> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPersistenceBuildingBlueprintId : public FPersistenceId {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FPersistenceBuildingBlueprintId> : TStructOpsTypeTraitsBase2<FPersistenceBuildingBlueprintId> {
    enum { WithSerializer = true };
};

}

#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/JsonUtilities/JsonObjectWrapper.h"

#include "UgcData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FUgcData {
    GENERATED_BODY()
    UPROPERTY()
    FString Name;
    UPROPERTY()
    TArray<FString> Tags;
    UPROPERTY()
    FJsonObjectWrapper SharedMetaData;
    UPROPERTY()
    FJsonObjectWrapper MetaData;
    UPROPERTY()
    int32 MinPlayerCount = 0;
    UPROPERTY()
    int32 MaxPlayerCount = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_MindsEye)
            return false;
        Ar << Name << Tags;
        SharedMetaData.Serialize(Ar);
        MetaData.Serialize(Ar);
        Ar << MinPlayerCount << MaxPlayerCount;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FUgcData> : TStructOpsTypeTraitsBase2<FUgcData> {
    enum { WithSerializer = true };
};

//! MindsEye saves the override as two bytes of its enum values.
USTRUCT()
struct FUGCPropertyDefaultValueOverride {
    GENERATED_BODY()
    UPROPERTY()
    TArray<uint8> EnumBytes;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_MindsEye)
            return false;
        EnumBytes.SetNum(2);
        Ar.Serialize(EnumBytes.GetData(), EnumBytes.Num());
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FUGCPropertyDefaultValueOverride> : TStructOpsTypeTraitsBase2<FUGCPropertyDefaultValueOverride> {
    enum { WithSerializer = true };
};

}

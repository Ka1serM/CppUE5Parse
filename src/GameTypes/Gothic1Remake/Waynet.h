#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "Waynet.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FWaynetPath {
    GENERATED_BODY()
    UPROPERTY()
    uint8 IsValid = 0;
    UPROPERTY()
    TArray<FVector> Path;
    UPROPERTY()
    TArray<uint8> Flags;
    UPROPERTY()
    FVector2f Position;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Gothic1Remake)
            return false;
        Ar << IsValid << Path << Flags << Position;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FWaynetPath> : TStructOpsTypeTraitsBase2<FWaynetPath> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FWaynetNode {
    GENERATED_BODY()
    UPROPERTY()
    uint8 IsValid = 0;
    UPROPERTY()
    uint64 Index = 0;
    UPROPERTY()
    FVector Position;
    UPROPERTY()
    bool NotFailed = false;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    TArray<uint64> Neighbours;
    UPROPERTY()
    uint16 Flags = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Gothic1Remake)
            return false;
        Ar << IsValid << Index << Position << NotFailed << Name << Neighbours << Flags;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FWaynetNode> : TStructOpsTypeTraitsBase2<FWaynetNode> {
    enum { WithSerializer = true };
};

//! Gothic 1 Remake saves 96 bytes of the proxy before the raw property values.
USTRUCT()
struct FAlkimiaLightweightStaticMeshProxyDesc {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Gothic1Remake)
            return false;
        Ar.SkipBytes(96);
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FAlkimiaLightweightStaticMeshProxyDesc> : TStructOpsTypeTraitsBase2<FAlkimiaLightweightStaticMeshProxyDesc> {
    enum { WithSerializer = true };
};

}

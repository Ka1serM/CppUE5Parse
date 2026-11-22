#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "MarvelSoftObjectPath.generated.h"

namespace ue5parse::runtime {

//! Marvel Rivals saves the soft object path as one string.
USTRUCT()
struct FMarvelSoftObjectPath {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath SoftObjectPath;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_MarvelRivals)
            return false;
        FString Path;
        Ar << Path;
        SoftObjectPath = FSoftObjectPath(Path);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMarvelSoftObjectPath> : TStructOpsTypeTraitsBase2<FMarvelSoftObjectPath> {
    enum { WithSerializer = true };
};

//! Marvel Rivals saves the property path as a byte count of strings that join with dots.
USTRUCT()
struct FSerializablePropertySoftPath {
    GENERATED_BODY()
    UPROPERTY()
    TArray<uint8> Data;
    UPROPERTY()
    FString PropertySoftPath;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_MarvelRivals)
            return false;
        Ar << Data;
        const uint8 Count = Ar.Read<uint8>();
        PropertySoftPath.Empty();
        for (uint8 Index = 0; Index < Count && !Ar.IsError(); Index++) {
            FString Segment;
            Ar << Segment;
            if (Index > 0)
                PropertySoftPath += FString(".");
            PropertySoftPath += Segment;
        }
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FSerializablePropertySoftPath> : TStructOpsTypeTraitsBase2<FSerializablePropertySoftPath> {
    enum { WithSerializer = true };
};

}

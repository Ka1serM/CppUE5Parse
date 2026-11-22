#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "AssaultFireFutureStructs.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FGPRowName {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName Id;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_AssaultFireFuture)
            return false;
        Ar << Name << Id;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FGPRowName> : TStructOpsTypeTraitsBase2<FGPRowName> {
    enum { WithSerializer = true };
};

//! Assault Fire Future saves a vector and then the data of the point type, a second vector and a tagged property list.
USTRUCT()
struct FAnnotationPointData2 {
    GENERATED_BODY()
    UPROPERTY()
    uint8 Type = 0;
    UPROPERTY()
    FVector3f SomeVector;
    UPROPERTY()
    uint8 SomeByte = 0;
    UPROPERTY()
    FVector3f SomeStructVector;
    UPROPERTY()
    FSoftObjectPath SomeStructPath;
    UPROPERTY()
    FInstancedStruct SomeStruct;
    UPROPERTY()
    float SomeStructFloat = 0;
    UPROPERTY()
    FVector2f SomeVector2D;
    UPROPERTY()
    FInstancedStruct StructFallback2;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FAnnotationPointData2> : TStructOpsTypeTraitsBase2<FAnnotationPointData2> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FWeaponPassiveID {
    GENERATED_BODY()
    UPROPERTY()
    uint32 Value = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_AssaultFireFuture)
            return false;
        Ar << Value;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FWeaponPassiveID> : TStructOpsTypeTraitsBase2<FWeaponPassiveID> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FMGEPassiveEquipStruct {
    GENERATED_BODY()
    UPROPERTY()
    uint64 Value = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_AssaultFireFuture)
            return false;
        Ar << Value;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMGEPassiveEquipStruct> : TStructOpsTypeTraitsBase2<FMGEPassiveEquipStruct> {
    enum { WithSerializer = true };
};

//! Assault Fire Future saves the asset data with the layout of the asset registry.
USTRUCT()
struct FAssetDataSerializable {
    GENERATED_BODY()
    UPROPERTY()
    FName ObjectPath;
    UPROPERTY()
    FName PackagePath;
    UPROPERTY()
    FName AssetClass;
    UPROPERTY()
    FName PackageName;
    UPROPERTY()
    FName AssetName;
    UPROPERTY()
    TMap<FName, FString> TagsAndValues;
    UPROPERTY()
    TArray<int32> ChunkIDs;
    UPROPERTY()
    uint32 PackageFlags = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_AssaultFireFuture)
            return false;
        Ar << ObjectPath << PackagePath << AssetClass << PackageName << AssetName << TagsAndValues << ChunkIDs << PackageFlags;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FAssetDataSerializable> : TStructOpsTypeTraitsBase2<FAssetDataSerializable> {
    enum { WithSerializer = true };
};

}

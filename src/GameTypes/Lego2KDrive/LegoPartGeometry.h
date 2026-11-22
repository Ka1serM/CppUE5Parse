#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/Core/Math/Box.h"

#include "LegoPartGeometry.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FLegoGraphPartInstance {
    GENERATED_BODY()
    UPROPERTY()
    uint32 Id = 0;
    UPROPERTY()
    uint32 Color = 0;

    bool Serialize(FArchive& Ar) {
        Ar << Id << Color;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FLegoGraphPartInstance> : TStructOpsTypeTraitsBase2<FLegoGraphPartInstance> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FLegoPartLODGeometry {
    GENERATED_BODY()
    UPROPERTY()
    bool bUsingFallback = false;
    UPROPERTY()
    bool bHasUVs = false;
    UPROPERTY()
    bool bGeneratedTangents = false;
    UPROPERTY()
    TArray<FVector3f> Positions;
    UPROPERTY()
    TArray<FVector4f> Tangents;
    UPROPERTY()
    TArray<FVector2f> UVs;
    UPROPERTY()
    TArray<uint16> Indices;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FLegoPartLODGeometry> : TStructOpsTypeTraitsBase2<FLegoPartLODGeometry> {
    enum { WithSerializer = true };
};

//! Lego 2K Drive saves only the default value of these per platform properties.
USTRUCT()
struct FPerPlatformUObject {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath Default;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Lego2KDrive)
            return false;
        Ar << Default;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FPerPlatformUObject> : TStructOpsTypeTraitsBase2<FPerPlatformUObject> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPerPlatformSoftObjectPtr {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath Default;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Lego2KDrive)
            return false;
        Ar << Default;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FPerPlatformSoftObjectPtr> : TStructOpsTypeTraitsBase2<FPerPlatformSoftObjectPtr> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPerPlatformMediaSource {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UObject> Default;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Lego2KDrive)
            return false;
        Ar << Default;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FPerPlatformMediaSource> : TStructOpsTypeTraitsBase2<FPerPlatformMediaSource> {
    enum { WithSerializer = true };
};

UCLASS()
class ULegoPartGeometry : public UObject {
    GENERATED_BODY()
public:
    explicit ULegoPartGeometry(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    TArray<uint8> Buffer;
    UPROPERTY()
    FIntVector Size;
    UPROPERTY()
    FBox3f Box;
    UPROPERTY()
    FVector2f SomeVector;
    UPROPERTY()
    FIntVector SomeSize;
};

}

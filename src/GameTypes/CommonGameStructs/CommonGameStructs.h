#pragma once

#include "Engine/Source/Runtime/Core/Math/Float16.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "CommonGameStructs.generated.h"

namespace ue5parse::runtime {

//! Games save these structs, whatever the game, as raw property values.
USTRUCT()
struct FLSAAudioSectionkey {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FLSAAudioSectionkey> : TStructOpsTypeTraitsBase2<FLSAAudioSectionkey> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FGG_CrowdGeneratorLocations {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FGG_CrowdGeneratorLocations> : TStructOpsTypeTraitsBase2<FGG_CrowdGeneratorLocations> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FParamRegistryInfo {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FParamRegistryInfo> : TStructOpsTypeTraitsBase2<FParamRegistryInfo> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FR5CollisionApproximation {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FR5CollisionApproximation> : TStructOpsTypeTraitsBase2<FR5CollisionApproximation> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FAnimMontageContainer {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FAnimMontageContainer> : TStructOpsTypeTraitsBase2<FAnimMontageContainer> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FNamedGuid {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FNamedGuid> : TStructOpsTypeTraitsBase2<FNamedGuid> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FSOS_GDValue {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        constexpr FRawPropertyRange Ranges[] = {{1, 1}};
        StaticStruct()->SerializeRawProperties(Ar, this, Ranges, RAW_RawValues);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FSOS_GDValue> : TStructOpsTypeTraitsBase2<FSOS_GDValue> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FR5SoftAssetPath {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        constexpr FRawPropertyRange Ranges[] = {{0, 1}};
        StaticStruct()->SerializeRawProperties(Ar, this, Ranges, RAW_RawValues);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FR5SoftAssetPath> : TStructOpsTypeTraitsBase2<FR5SoftAssetPath> {
    enum { WithSerializer = true };
};

//! The struct is three doubles in every game.
USTRUCT()
struct FVectorDouble {
    GENERATED_BODY()
    UPROPERTY()
    double X = 0;
    UPROPERTY()
    double Y = 0;
    UPROPERTY()
    double Z = 0;

    bool Serialize(FArchive& Ar) {
        Ar << X << Y << Z;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FVectorDouble> : TStructOpsTypeTraitsBase2<FVectorDouble> {
    enum { WithSerializer = true };
};

//! The struct is one half precision float in every game.
USTRUCT()
struct FVoyageFloat16 {
    GENERATED_BODY()
    UPROPERTY()
    float Value = 0;

    bool Serialize(FArchive& Ar) {
        FFloat16 Stored;
        Ar << Stored;
        Value = Stored.GetFloat();
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FVoyageFloat16> : TStructOpsTypeTraitsBase2<FVoyageFloat16> {
    enum { WithSerializer = true };
};

//! The struct is one name in every game.
USTRUCT()
struct FCore1047ReleaseFlag {
    GENERATED_BODY()
    UPROPERTY()
    FName Release;

    bool Serialize(FArchive& Ar) {
        Ar << Release;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FCore1047ReleaseFlag> : TStructOpsTypeTraitsBase2<FCore1047ReleaseFlag> {
    enum { WithSerializer = true };
};

//! The struct is 13 bytes in every game.
USTRUCT()
struct FRDialogueFactValue {
    GENERATED_BODY()
    UPROPERTY()
    TArray<uint8> Data;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetRemaining() < 13) {
            Ar.SetError(ErrorCode::ReadPastEnd, "FRDialogueFactValue");
            return true;
        }
        Data.SetNum(13);
        Ar.Serialize(Data.GetData(), 13);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FRDialogueFactValue> : TStructOpsTypeTraitsBase2<FRDialogueFactValue> {
    enum { WithSerializer = true };
};

}

#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"

#include "MK1Structs.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FCompressedFloatTrackData {
    GENERATED_BODY()
    UPROPERTY()
    uint8 Format = 0;
    UPROPERTY()
    TArray<int32> Keys;
    UPROPERTY()
    TArray<uint8> DataBuffer;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_MortalKombat1)
            return false;
        Ar << Format;
        BulkSerialize(Ar, Keys);
        BulkSerialize(Ar, DataBuffer);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FCompressedFloatTrackData> : TStructOpsTypeTraitsBase2<FCompressedFloatTrackData> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FTimelinePredicateState {
    GENERATED_BODY()
    UPROPERTY()
    TArray<uint64> Unknown;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_MortalKombat1)
            return false;
        Ar << Unknown;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FTimelinePredicateState> : TStructOpsTypeTraitsBase2<FTimelinePredicateState> {
    enum { WithSerializer = true };
};

//! Mortal Kombat 1 saves the C++ type name of the parameter and then the value of that type; an unknown type has no value.
USTRUCT()
struct FMK1ParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FName Type;
    UPROPERTY()
    bool BoolValue = false;
    UPROPERTY()
    float FloatValue = 0;
    UPROPERTY()
    int32 IntValue = 0;
    UPROPERTY()
    TObjectPtr<UObject> ObjectValue = nullptr;
    UPROPERTY()
    FVector3f VectorValue;
    UPROPERTY()
    FName NameValue;

    friend FArchive& operator<<(FArchive& Ar, FMK1ParameterValue& Value);
};

USTRUCT()
struct FCompiledTimelinePredicate {
    GENERATED_BODY()
    UPROPERTY()
    TArray<uint8> Unknown;
    UPROPERTY()
    TArray<FMK1ParameterValue> Parameters;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_MortalKombat1)
            return false;
        Ar << Unknown << Parameters;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FCompiledTimelinePredicate> : TStructOpsTypeTraitsBase2<FCompiledTimelinePredicate> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FGameParameterBase {
    GENERATED_BODY()
    UPROPERTY()
    FMK1ParameterValue Value;
    UPROPERTY()
    TArray<uint8> Data;
    UPROPERTY()
    FName Type;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_MortalKombat1)
            return false;
        Ar << Value;
        Data.SetNum(6);
        Ar.Serialize(Data.GetData(), 6);
        Ar << Type;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FGameParameterBase> : TStructOpsTypeTraitsBase2<FGameParameterBase> {
    enum { WithSerializer = true };
};

//! Mortal Kombat 1 saves the rows again after the table, as tagged properties, and these rows replace the first rows.
UCLASS()
class UMKDialogueTable : public UDataTable {
    GENERATED_BODY()
public:
    explicit UMKDialogueTable(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;
};

}

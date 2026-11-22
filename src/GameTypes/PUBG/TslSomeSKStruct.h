#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "TslSomeSKStruct.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FTslSomeBoneStruct {
    GENERATED_BODY()
    UPROPERTY()
    uint8 Type = 0;
    UPROPERTY()
    int32 Bone = 0;
};

USTRUCT()
struct FTslSomeBonePair {
    GENERATED_BODY()
    UPROPERTY()
    FTslSomeBoneStruct Key;
    UPROPERTY()
    FTslSomeBoneStruct Value;
};

USTRUCT()
struct FTslSomeBoneMap {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FTslSomeBonePair> Pairs;
};

USTRUCT()
struct FTslSomeBoneMapArray {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FTslSomeBoneMap> Maps;
};

//! PUBG saves arrays of arrays of maps, and each key and value is a packed byte and 32 bit bone.
USTRUCT()
struct FTslSomeSKStruct {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FTslSomeBoneMapArray> SomeBoneStructs;

    bool Serialize(FArchive& Ar) {
        SomeBoneStructs.SetNum(Ar.ReadCount(4));
        for (FTslSomeBoneMapArray& Outer : SomeBoneStructs) {
            Outer.Maps.SetNum(Ar.ReadCount(4));
            for (FTslSomeBoneMap& Map : Outer.Maps) {
                Map.Pairs.SetNum(Ar.ReadCount(10));
                for (FTslSomeBonePair& Pair : Map.Pairs)
                    Ar << Pair.Key.Type << Pair.Key.Bone << Pair.Value.Type << Pair.Value.Bone;
            }
        }
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FTslSomeSKStruct> : TStructOpsTypeTraitsBase2<FTslSomeSKStruct> {
    enum { WithSerializer = true };
};

}

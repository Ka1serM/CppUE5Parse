#pragma once

#include "GameTypes/OuterWorlds2/PropertyDataWithObjects.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "HavokAIAnyArray.generated.h"

namespace ue5parse::runtime {

//! Killing Floor 3 saves the elements after the tagged properties that give their struct and their count.
USTRUCT()
struct FHavokAIAnyArray {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UObject> ElementType = nullptr;
    UPROPERTY()
    int32 NumElements = 0;
    UPROPERTY()
    TArray<FInstancedStruct> Data;

    bool Serialize(FArchive& Ar) {
        StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(this));
        if (Ar.IsError() || NumElements <= 0)
            return true;
        if (ElementType == nullptr) {
            Ar.SetError(ErrorCode::InvalidData, "FHavokAIAnyArray.ElementType");
            return true;
        }
        Data.Empty(NumElements);
        for (int32 Index = 0; Index < NumElements && !Ar.IsError(); Index++)
            SerializeStructByObject(Ar, ElementType.Get(), Data[Data.AddDefaulted()]);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FHavokAIAnyArray> : TStructOpsTypeTraitsBase2<FHavokAIAnyArray> {
    enum { WithSerializer = true };
};

}

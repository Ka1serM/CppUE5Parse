#pragma once

#include "Engine/Source/Runtime/Core/Internationalization/Text.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "SUDSValue.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ESUDSValueType : uint8 {
    Text = 0,
    Int = 1,
    Float = 2,
    Boolean = 3,
    Gender = 4,
    Name = 5,
    Variable = 10,
    Empty = 99,
};

//! Upin & Ipin Universe saves the type of the value and then only the value of that type; names are saved as strings.
USTRUCT()
struct FSUDSValue {
    GENERATED_BODY()
    UPROPERTY()
    ESUDSValueType Type = ESUDSValueType::Empty;
    UPROPERTY()
    FText TextValue;
    UPROPERTY()
    int32 IntValue = 0;
    UPROPERTY()
    float FloatValue = 0;
    UPROPERTY()
    bool BoolValue = false;
    UPROPERTY()
    FString NameValue;

    bool Serialize(FArchive& Ar) {
        Ar << Type;
        if (Type == ESUDSValueType::Name || Type == ESUDSValueType::Text || Type == ESUDSValueType::Variable || Type == ESUDSValueType::Empty)
            Ar.SkipBytes(4);
        switch (Type) {
        case ESUDSValueType::Text:
            Ar << TextValue;
            return true;
        case ESUDSValueType::Int:
            Ar << IntValue;
            return true;
        case ESUDSValueType::Float:
            Ar << FloatValue;
            return true;
        case ESUDSValueType::Boolean:
            Ar << BoolValue;
            return true;
        case ESUDSValueType::Name:
        case ESUDSValueType::Variable:
            Ar << NameValue;
            return true;
        case ESUDSValueType::Empty:
            return true;
        default:
            Ar.SetError(ErrorCode::InvalidData, "FSUDSValue.Type");
            return true;
        }
    }
};

template <>
struct TStructOpsTypeTraits<FSUDSValue> : TStructOpsTypeTraitsBase2<FSUDSValue> {
    enum { WithSerializer = true };
};

}

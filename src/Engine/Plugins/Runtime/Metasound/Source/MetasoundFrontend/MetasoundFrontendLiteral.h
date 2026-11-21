#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "MetasoundFrontendLiteral.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EMetasoundFrontendLiteralType : uint8 {
    None,
    Boolean,
    Integer,
    Float,
    String,
    UObject,
    NoneArray,
    BooleanArray,
    IntegerArray,
    FloatArray,
    StringArray,
    UObjectArray,
    Invalid,
};

USTRUCT()
struct FMetasoundFrontendLiteral {
    GENERATED_BODY()

    UPROPERTY()
    EMetasoundFrontendLiteralType Type = EMetasoundFrontendLiteralType::None;
    UPROPERTY()
    int32 AsNumDefault = 0;
    UPROPERTY()
    TArray<bool> AsBoolean;
    UPROPERTY()
    TArray<int32> AsInteger;
    UPROPERTY()
    TArray<float> AsFloat;
    UPROPERTY()
    TArray<FString> AsString;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> AsUObject;
};

}

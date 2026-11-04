#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "MaterialExpression.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FExpressionInput {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UObject> Expression = nullptr;
    UPROPERTY()
    int32 OutputIndex = 0;
    UPROPERTY()
    FName InputName;
    UPROPERTY()
    int32 Mask = 0;
    UPROPERTY()
    int32 MaskR = 0;
    UPROPERTY()
    int32 MaskG = 0;
    UPROPERTY()
    int32 MaskB = 0;
    UPROPERTY()
    int32 MaskA = 0;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FExpressionInput> : TStructOpsTypeTraitsBase2<FExpressionInput> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FMaterialAttributesInput : public FExpressionInput {
    GENERATED_BODY()
    UPROPERTY()
    uint64 PropertyConnectedMask = 0;

    bool Serialize(FArchive& Ar) { return FExpressionInput::Serialize(Ar); }
};

template <>
struct TStructOpsTypeTraits<FMaterialAttributesInput> : TStructOpsTypeTraitsBase2<FMaterialAttributesInput> {
    enum { WithSerializer = true };
};

template <class TInput>
void SerializeExpressionInput(FArchive& Ar, TInput& Input) {
    Ar << Input.Expression << Input.OutputIndex << Input.InputName << Input.Mask << Input.MaskR << Input.MaskG << Input.MaskB
       << Input.MaskA;
}

}

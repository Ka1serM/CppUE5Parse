#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

#include "MaterialExpression.generated.h"

namespace ue5parse::runtime {

// FExpressionInput: an input pin of a material expression: the expression it connects to and the output and mask it
// reads (UE's Materials/MaterialExpression.h). Cooked data writes it natively.
USTRUCT()
struct FExpressionInput {
    GENERATED_BODY()
    // A UMaterialExpression; the runtime has no expression classes.
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

// FMaterialAttributesInput: an input that carries all material attributes.
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

// The members of an expression input, as cooked data writes them (UE's SerializeExpressionInput); for FExpressionInput
// and the reflected FMaterialInput, which have the same members.
template <class TInput>
void SerializeExpressionInput(FArchive& Ar, TInput& Input) {
    Ar << Input.Expression << Input.OutputIndex << Input.InputName << Input.Mask << Input.MaskR << Input.MaskG << Input.MaskB
       << Input.MaskA;
}

}

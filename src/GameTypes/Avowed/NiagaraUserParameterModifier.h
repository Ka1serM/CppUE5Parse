#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "NiagaraUserParameterModifier.generated.h"

namespace ue5parse::runtime {

//! Avowed saves the type of the value and then only the value of that type.
USTRUCT()
struct FNiagaraUserParameterModifier {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    bool bUseDefaultValue = false;
    UPROPERTY()
    uint8 Type = 0;
    UPROPERTY()
    int32 IntValue = 0;
    UPROPERTY()
    float FloatValue = 0;
    UPROPERTY()
    bool BoolValue = false;
    UPROPERTY()
    FVector3f VectorValue;
    UPROPERTY()
    FLinearColor ColorValue;

    bool Serialize(FArchive& Ar) {
        Ar << Name << bUseDefaultValue << Type;
        switch (Type) {
        case 0:
            Ar << IntValue;
            return true;
        case 1:
            Ar << FloatValue;
            return true;
        case 2:
            Ar << BoolValue;
            return true;
        case 3:
            Ar << VectorValue;
            return true;
        case 4:
            Ar << ColorValue;
            return true;
        default:
            Ar.SetError(ErrorCode::InvalidData, "FNiagaraUserParameterModifier.Type");
            return true;
        }
    }
};

template <>
struct TStructOpsTypeTraits<FNiagaraUserParameterModifier> : TStructOpsTypeTraitsBase2<FNiagaraUserParameterModifier> {
    enum { WithSerializer = true };
};

}

#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

namespace ue5parse::runtime {

struct FNiagaraVariableCommonReference {
    FName Name;
    UObject* UnderlyingType = nullptr;
    bool operator==(const FNiagaraVariableCommonReference&) const = default;

    bool Serialize(FArchive& Ar) {
        Ar << Name << UnderlyingType;
        return true;
    }

    friend FArchive& operator<<(FArchive& Ar, FNiagaraVariableCommonReference& Variable) {
        Variable.Serialize(Ar);
        return Ar;
    }
};

template <>
struct TStructOpsTypeTraits<FNiagaraVariableCommonReference> : TStructOpsTypeTraitsBase2<FNiagaraVariableCommonReference> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}

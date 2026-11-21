#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "NiagaraUserRedirectionParameterStore.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraParameterStore {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UObject> Owner = nullptr;
    UPROPERTY()
    TArray<FNiagaraVariableWithOffset> SortedParameterOffsets;
    UPROPERTY()
    TArray<uint8> ParameterData;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> DataInterfaces;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> UObjects;
};

USTRUCT()
struct FNiagaraUserRedirectionParameterStore : public FNiagaraParameterStore {
    GENERATED_BODY()

    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FNiagaraUserRedirectionParameterStore> : TStructOpsTypeTraitsBase2<FNiagaraUserRedirectionParameterStore> {
    enum { WithSerializeFromMismatchedTag = true };
};

}

#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

#include "NiagaraCompileHash.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraCompileHash {
    GENERATED_BODY()

    UPROPERTY()
    TArray<uint8> DataHash;

    friend FArchive& operator<<(FArchive& Ar, FNiagaraCompileHash& Id);
};

}

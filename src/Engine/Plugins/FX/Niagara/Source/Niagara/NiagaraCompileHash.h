#pragma once


#include "NiagaraCompileHash.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraCompileHash {
    GENERATED_BODY()

    UPROPERTY()
    TArray<uint8> DataHash;
};

}

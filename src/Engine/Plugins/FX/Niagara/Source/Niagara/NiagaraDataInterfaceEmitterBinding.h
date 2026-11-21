#pragma once


#include "NiagaraDataInterfaceEmitterBinding.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ENiagaraDataInterfaceEmitterBindingMode : int32 { Self, Other };

USTRUCT()
struct FNiagaraDataInterfaceEmitterBinding {
    GENERATED_BODY()

    UPROPERTY()
    ENiagaraDataInterfaceEmitterBindingMode BindingMode = ENiagaraDataInterfaceEmitterBindingMode::Other;
    UPROPERTY()
    FName EmitterName;
};

}

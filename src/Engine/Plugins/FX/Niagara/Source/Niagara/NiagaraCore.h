#pragma once

#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "NiagaraCore.generated.h"

namespace ue5parse::runtime {

class UObject;

UENUM()
enum class ENiagaraIterationSource : uint8 { Particles = 0, DataInterface, DirectSet };

USTRUCT()
struct FNiagaraVariableCommonReference {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    TObjectPtr<UObject> UnderlyingType = nullptr;

    bool Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FNiagaraVariableCommonReference& Reference) {
        Reference.Serialize(Ar);
        return Ar;
    }
};

template <>
struct TStructOpsTypeTraits<FNiagaraVariableCommonReference> : TStructOpsTypeTraitsBase2<FNiagaraVariableCommonReference> {
    enum { WithSerializer = true };
};

}

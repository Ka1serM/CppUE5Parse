#pragma once

#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"

#include "FortHomebaseNodeGameplayEffectDataTable.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UFortHomebaseNodeGameplayEffectDataTable : public UDataTable {
    GENERATED_BODY()
public:
    explicit UFortHomebaseNodeGameplayEffectDataTable(const FObjectInitializer& ObjectInitializer);
};

}

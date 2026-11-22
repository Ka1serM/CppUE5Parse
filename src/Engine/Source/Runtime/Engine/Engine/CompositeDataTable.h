#pragma once

#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"

#include "CompositeDataTable.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCompositeDataTable : public UDataTable {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<UDataTable>> ParentTables;

    void Serialize(FArchive& Ar) override;
};

}

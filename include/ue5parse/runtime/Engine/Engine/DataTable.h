#pragma once

#include "ue5parse/runtime/Core/Containers/Map.h"
#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "DataTable.generated.h"

namespace ue5parse::runtime {

// UDataTable: rows of one struct, by name (UE's Engine/DataTable.h). The rows are serialized natively.
UCLASS()
class UDataTable : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UScriptStruct> RowStruct = nullptr;

    // Each row's memory, laid out as RowStruct, in the table's order.
    TMap<FName, uint8*> RowMap;

    void FinishDestroy() override;
    void Serialize(FArchive& Ar) override;
    const UScriptStruct* GetRowStruct() const { return RowStruct; }
    const TMap<FName, uint8*>& GetRowMap() const { return RowMap; }
    // The row of a name; null when the table has none.
    uint8* FindRowUnchecked(FName RowName) const;
    TArray<FName> GetRowNames() const;

private:
    void EmptyTable();
};

}

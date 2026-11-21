#pragma once

#include <unordered_map>

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SerializedProperties.h"

#include "DataTable.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UDataTable : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UScriptStruct> RowStruct = nullptr;
    UPROPERTY()
    uint8 bStripFromClientBuilds : 1 = false;
    UPROPERTY()
    uint8 bIgnoreExtraFields : 1 = false;
    UPROPERTY()
    uint8 bIgnoreMissingFields : 1 = false;
    UPROPERTY()
    uint8 bPreserveExistingValues : 1 = false;
    UPROPERTY()
    FString ImportKeyField;

    TMap<FName, uint8*> RowMap;

    void FinishDestroy() override;
    void Serialize(FArchive& Ar) override;
    const UScriptStruct* GetRowStruct() const { return RowStruct; }
    uint8* FindRowUnchecked(FName RowName) const;
    const FSerializedPropertyList* GetRowSerializedProperties(FName RowName) const;
    TArray<FName> GetRowNames() const;

private:
    std::unordered_map<FName, FSerializedPropertyList, FNameHasher> SerializedRows;
    void EmptyTable();
};

USTRUCT()
struct FDataTableRowHandle {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<const UDataTable> DataTable = nullptr;
    UPROPERTY()
    FName RowName;
};

USTRUCT()
struct FDataTableCategoryHandle {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<const UDataTable> DataTable = nullptr;
    UPROPERTY()
    FName ColumnName;
    UPROPERTY()
    FName RowContents;
};

}

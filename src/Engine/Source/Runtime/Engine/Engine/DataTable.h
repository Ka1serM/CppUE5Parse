#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "DataTable.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UDataTable : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UScriptStruct> RowStruct = nullptr;

    TMap<FName, uint8*> RowMap;

    void FinishDestroy() override;
    void Serialize(FArchive& Ar) override;
    const UScriptStruct* GetRowStruct() const { return RowStruct; }
    uint8* FindRowUnchecked(FName RowName) const;
    TArray<FName> GetRowNames() const;

private:
    void EmptyTable();
};

}

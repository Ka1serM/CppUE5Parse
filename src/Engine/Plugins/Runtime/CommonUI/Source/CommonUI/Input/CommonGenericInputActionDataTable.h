#pragma once

#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"

#include "CommonGenericInputActionDataTable.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCommonGenericInputActionDataTable : public UDataTable {
    GENERATED_BODY()
public:
    UCommonGenericInputActionDataTable() {
        RowStruct = FindNativeStruct(FName("CommonInputActionDataBase"));
    }
};

}

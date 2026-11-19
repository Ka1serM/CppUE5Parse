#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "CurveTable.generated.h"

namespace ue5parse::runtime {

class UCurveTable;

USTRUCT()
struct FCurveTableRowHandle {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<const UCurveTable> CurveTable = nullptr;
    UPROPERTY()
    FName RowName;
};

}

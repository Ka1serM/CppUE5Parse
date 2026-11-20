#pragma once

#include "Engine/Source/Runtime/Engine/Engine/CurveTable.h"

#include "CompositeCurveTable.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCompositeCurveTable : public UCurveTable {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<UCurveTable>> ParentTables;
};

}

#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "CurveTable.generated.h"

namespace ue5parse::runtime {

struct FRealCurve;

UENUM()
enum class ECurveTableMode : uint8 { Empty, SimpleCurves, RichCurves };

UCLASS()
class UCurveTable : public UObject {
    GENERATED_BODY()
public:
    void FinishDestroy() override;
    void Serialize(FArchive& Ar) override;
    const TMap<FName, FRealCurve*>& GetRowMap() const { return RowMap; }
    ECurveTableMode GetCurveTableMode() const { return CurveTableMode; }

private:
    void EmptyTable();

    TMap<FName, FRealCurve*> RowMap;
    ECurveTableMode CurveTableMode = ECurveTableMode::Empty;
};

USTRUCT()
struct FCurveTableRowHandle {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<const UCurveTable> CurveTable = nullptr;
    UPROPERTY()
    FName RowName;
};

}

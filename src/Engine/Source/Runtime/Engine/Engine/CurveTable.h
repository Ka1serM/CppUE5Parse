#pragma once

#include <memory>

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/Engine/Curves/RealCurve.h"

#include "CurveTable.generated.h"

namespace ue5parse::runtime {

enum class ECurveTableMode : uint8 { Empty, SimpleCurves, RichCurves };

UCLASS()
class UCurveTable : public UObject {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;
    ECurveTableMode GetCurveTableMode() const { return CurveTableMode; }
    const FRealCurve* FindCurve(FName RowName) const;
    TArray<FName> GetRowNames() const;

private:
    TMap<FName, std::unique_ptr<FRealCurve>> RowMap;
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

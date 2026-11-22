#include "Engine/Source/Runtime/Engine/Engine/CurveTable.h"

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Curves/RichCurve.h"
#include "Engine/Source/Runtime/Engine/Curves/SimpleCurve.h"

namespace ue5parse::runtime {

namespace {

template <class CurveType>
std::unique_ptr<FRealCurve> LoadCurve(FArchive& Ar) {
    auto Curve = std::make_unique<CurveType>();
    CurveType::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(Curve.get()));
    return Curve;
}

}

void UCurveTable::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    RowMap.Empty();
    int32 NumRows = 0;
    Ar << NumRows;
    const bool bUpgradingCurveTable = !Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::ShrinkCurveTableSize,
                                                           EGame::GAME_UE4_22);
    if (bUpgradingCurveTable)
        CurveTableMode = NumRows > 0 ? ECurveTableMode::RichCurves : ECurveTableMode::Empty;
    else
        Ar.Serialize(&CurveTableMode, sizeof(CurveTableMode));
    for (int32 RowIdx = 0; RowIdx < NumRows && !Ar.IsError(); RowIdx++) {
        FName RowName;
        Ar << RowName;
        RowMap.Add(RowName, CurveTableMode == ECurveTableMode::SimpleCurves ? LoadCurve<FSimpleCurve>(Ar) : LoadCurve<FRichCurve>(Ar));
    }
}

const FRealCurve* UCurveTable::FindCurve(FName RowName) const {
    const std::unique_ptr<FRealCurve>* Curve = RowMap.Find(RowName);
    return Curve ? Curve->get() : nullptr;
}

TArray<FName> UCurveTable::GetRowNames() const {
    TArray<FName> Names;
    for (const auto& Pair : RowMap)
        Names.Add(Pair.Key);
    return Names;
}

}

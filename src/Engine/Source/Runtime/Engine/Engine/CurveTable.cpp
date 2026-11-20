#include "Engine/Source/Runtime/Engine/Engine/CurveTable.h"

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Curves/RichCurve.h"
#include "Engine/Source/Runtime/Engine/Curves/SimpleCurve.h"

namespace ue5parse::runtime {

void UCurveTable::FinishDestroy() {
    EmptyTable();
    Super::FinishDestroy();
}

void UCurveTable::EmptyTable() {
    for (auto& Pair : RowMap)
        delete Pair.Value;
    RowMap.Empty();
}

void UCurveTable::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    EmptyTable();
    const int32 NumRows = Ar.ReadCount(8);
    if (Ar.CustomVer(FFortniteMainBranchObjectVersion::GUID) < FFortniteMainBranchObjectVersion::ShrinkCurveTableSize) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "UCurveTable.CurveTableMode");
        return;
    }
    Ar << CurveTableMode;
    for (int32 RowIdx = 0; RowIdx < NumRows && !Ar.IsError(); RowIdx++) {
        FName RowName;
        Ar << RowName;
        if (CurveTableMode == ECurveTableMode::SimpleCurves) {
            FSimpleCurve* NewCurve = new FSimpleCurve();
            FSimpleCurve::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(NewCurve));
            RowMap.Add(RowName, NewCurve);
            continue;
        }
        FRichCurve* NewCurve = new FRichCurve();
        FRichCurve::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(NewCurve));
        RowMap.Add(RowName, NewCurve);
    }
}

}

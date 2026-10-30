#include "ue5parse/runtime/Engine/Engine/DataTable.h"

#include <cstring>

#include "ue5parse/runtime/Core/Serialization/Archive.h"

// UE's DataTable.cpp (UE 5.3).

namespace ue5parse::runtime {

// The rows go with the object, while their struct is still alive.
void UDataTable::FinishDestroy() {
    EmptyTable();
    Super::FinishDestroy();
}

void UDataTable::EmptyTable() {
    for (auto& Pair : RowMap) {
        if (RowStruct != nullptr) RowStruct->DestroyStruct(Pair.Value);
        RuntimeFree(Pair.Value);
    }
    RowMap.Empty();
}

void UDataTable::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    EmptyTable();
    if (RowStruct == nullptr) {
        Ar.SetError(ErrorCode::MissingSchema, "UDataTable.RowStruct");
        return;
    }
    // UE's LoadStructData: each row's name, then its value as the row struct.
    const int32 RowCount = Ar.ReadCount(8);
    for (int32 i = 0; i < RowCount && !Ar.IsError(); i++) {
        FName RowName;
        Ar << RowName;
        uint8* RowData = static_cast<uint8*>(RuntimeMalloc(static_cast<std::size_t>(RowStruct->GetStructureSize()),
                                                           static_cast<std::size_t>(RowStruct->GetMinAlignment())));
        std::memset(RowData, 0, static_cast<std::size_t>(RowStruct->GetStructureSize()));
        RowStruct->InitializeStruct(RowData);
        RowStruct->SerializeItem(Ar, RowData);
        if (uint8** Existing = RowMap.Find(RowName)) {
            RowStruct->DestroyStruct(*Existing);
            RuntimeFree(*Existing);
            *Existing = RowData;
        } else {
            RowMap.Add(RowName, RowData);
        }
    }
}

uint8* UDataTable::FindRowUnchecked(FName RowName) const {
    uint8* const* Row = RowMap.Find(RowName);
    return Row ? *Row : nullptr;
}

TArray<FName> UDataTable::GetRowNames() const {
    TArray<FName> Names;
    for (const auto& Pair : RowMap) Names.Add(Pair.Key);
    return Names;
}

}

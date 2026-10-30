#include "ue5parse/runtime/Engine/Engine/DataTable.h"

#include <cstring>

#include "ue5parse/runtime/Core/Serialization/Archive.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/Engine/EngineNativeMissing.h"

// UE's DataTable.cpp and DataTableFunctionLibrary.cpp (UE 5.3).

namespace ue5parse::runtime {

namespace {

// UE's FStructUtils::TheSameLayout: the same property types at the same offsets.
bool TheSameLayout(const UStruct* StructA, const UStruct* StructB) {
    const FProperty* PropertyA = StructA->PropertyLink;
    const FProperty* PropertyB = StructB->PropertyLink;
    for (; PropertyA != nullptr && PropertyB != nullptr; PropertyA = PropertyA->PropertyLinkNext, PropertyB = PropertyB->PropertyLinkNext) {
        if (!PropertyA->SameType(PropertyB) || PropertyA->GetOffset_ForInternal() != PropertyB->GetOffset_ForInternal() ||
            PropertyA->GetSize() != PropertyB->GetSize())
            return false;
    }
    return PropertyA == PropertyB;
}

}

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

bool UDataTableFunctionLibrary::GetDataTableRowFromName(UDataTable*, FName, FTableRowBase&) { return false; }

bool UDataTableFunctionLibrary::Generic_GetDataTableRowFromName(const UDataTable* Table, FName RowName, void* OutRowPtr) {
    if (OutRowPtr == nullptr || Table == nullptr) return false;
    const uint8* RowPtr = Table->FindRowUnchecked(RowName);
    const UScriptStruct* StructType = Table->GetRowStruct();
    if (RowPtr == nullptr || StructType == nullptr) return false;
    StructType->CopyScriptStruct(OutRowPtr, RowPtr);
    return true;
}

// The out row is stepped without a buffer: its address is the Blueprint's variable, laid out as its own struct.
DEFINE_FUNCTION(UDataTableFunctionLibrary::execGetDataTableRowFromName) {
    P_GET_OBJECT(UDataTable, Table);
    P_GET_STRUCT(FName, RowName);
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentProperty = nullptr;
    Stack.StepCompiledIn(nullptr);
    void* OutRowPtr = Stack.MostRecentPropertyAddress;
    const auto* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);
    P_FINISH;
    bool bSuccess = false;
    if (Table == nullptr) {
        Stack.KismetExecutionMessage("GetDataTableRow", "Missing data table input in GetDataTableRow", EBlueprintExceptionType::AccessViolation);
    } else if (StructProp != nullptr && OutRowPtr != nullptr) {
        const UScriptStruct* OutputType = StructProp->Struct;
        const UScriptStruct* TableType = Table->GetRowStruct();
        const bool bCompatible = OutputType == TableType ||
                                 (OutputType != nullptr && TableType != nullptr && OutputType->IsChildOf(TableType) && TheSameLayout(OutputType, TableType));
        if (bCompatible)
            bSuccess = Generic_GetDataTableRowFromName(Table, RowName, OutRowPtr);
        else
            Stack.KismetExecutionMessage("GetDataTableRow", "Incompatible output parameter; the data table's type is not the same as the return type.");
    } else {
        Stack.KismetExecutionMessage("GetDataTableRow", "Failed to resolve the output parameter for GetDataTableRow.");
    }
    *static_cast<bool*>(RESULT_PARAM) = bSuccess;
}

}

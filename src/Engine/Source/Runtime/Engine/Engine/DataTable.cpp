#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"

#include <cstring>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SerializedProperties.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"
#include "Engine/Source/Runtime/Engine/EngineNativeMissing.h"

namespace ue5parse::runtime {

namespace {

bool TheSameLayout(const UStruct* StructA, const UStruct* StructB) {
    const FProperty* PropertyA = StructA->PropertyLink;
    const FProperty* PropertyB = StructB->PropertyLink;
    for (; PropertyA != nullptr && PropertyB != nullptr; PropertyA = PropertyA->PropertyLinkNext, PropertyB = PropertyB->PropertyLinkNext)
        if (!PropertyA->SameType(PropertyB) || PropertyA->GetOffset_ForInternal() != PropertyB->GetOffset_ForInternal()
            || PropertyA->GetSize() != PropertyB->GetSize())
            return false;
    return PropertyA == PropertyB;
}

//! Lost Soul Aside writes the table name and a map of metadata after the rows.
void SerializeLostSoulAsideMetaData(FArchive& Ar) {
    FString DataTableName;
    Ar << DataTableName;
    TMap<FString, TMap<FName, FString>> MetaData;
    Ar << MetaData;
}

}

void UDataTable::FinishDestroy() {
    EmptyTable();
    Super::FinishDestroy();
}

void UDataTable::EmptyTable() {
    for (auto& Pair : RowMap) {
        if (RowStruct != nullptr)
            RowStruct->DestroyStruct(Pair.Value);
        RuntimeFree(Pair.Value);
    }
    RowMap.Empty();
}

void UDataTable::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    EmptyTable();
    if (RowStruct == nullptr) {
        Ar.SetError(ErrorCode::MissingSchema, "UDataTable.RowStruct");
        return;
    }
    if (Ar.GetGame() == EGame::GAME_HonorofKingsWorld)
        Ar.SkipBytes(16);
    const int32 RowCount = Ar.ReadCount(8);
    //! Honor of Kings: World writes a map of the row names to two uint64 and an int32 before the rows.
    if (Ar.GetGame() == EGame::GAME_HonorofKingsWorld) {
        for (int32 i = 0; i < RowCount && !Ar.IsError(); i++) {
            FName RowName;
            Ar << RowName;
            Ar.SkipBytes(20);
        }
    }
    for (int32 i = 0; i < RowCount && !Ar.IsError(); i++) {
        FName RowName;
        Ar << RowName;
        LoadRow(Ar, RowName);
    }
    if (Ar.GetGame() == EGame::GAME_LostSoulAside)
        SerializeLostSoulAsideMetaData(Ar);
}

void UDataTable::LoadRow(FArchive& Ar, FName RowName) {
    uint8* RowData =
        static_cast<uint8*>(RuntimeMalloc(static_cast<std::size_t>(RowStruct->GetStructureSize()), static_cast<std::size_t>(RowStruct->GetMinAlignment())));
    std::memset(RowData, 0, static_cast<std::size_t>(RowStruct->GetStructureSize()));
    RowStruct->InitializeStruct(RowData);
    FSerializedPropertyScope Scope(Ar, RowName, RowStruct, RowData);
    RowStruct->SerializeItem(Ar, RowData);
    if (uint8** Existing = RowMap.Find(RowName)) {
        RowStruct->DestroyStruct(*Existing);
        RuntimeFree(*Existing);
        *Existing = RowData;
        return;
    }
    RowMap.Add(RowName, RowData);
}

uint8* UDataTable::FindRowUnchecked(FName RowName) const {
    uint8* const* Row = RowMap.Find(RowName);
    return Row ? *Row : nullptr;
}

TArray<FName> UDataTable::GetRowNames() const {
    TArray<FName> Names;
    for (const auto& Pair : RowMap)
        Names.Add(Pair.Key);
    return Names;
}

bool UDataTableFunctionLibrary::GetDataTableRowFromName(UDataTable*, FName, FTableRowBase&) {
    return false;
}

bool UDataTableFunctionLibrary::Generic_GetDataTableRowFromName(const UDataTable* Table, FName RowName, void* OutRowPtr) {
    if (OutRowPtr == nullptr || Table == nullptr)
        return false;
    const uint8* RowPtr = Table->FindRowUnchecked(RowName);
    const UScriptStruct* StructType = Table->GetRowStruct();
    if (RowPtr == nullptr || StructType == nullptr)
        return false;
    StructType->CopyScriptStruct(OutRowPtr, RowPtr);
    return true;
}

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
        const bool bCompatible = OutputType == TableType
                                 || (OutputType != nullptr && TableType != nullptr && OutputType->IsChildOf(TableType) && TheSameLayout(OutputType, TableType));
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

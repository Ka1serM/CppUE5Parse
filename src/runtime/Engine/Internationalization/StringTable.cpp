#include "ue5parse/runtime/Engine/Internationalization/StringTable.h"

#include "ue5parse/runtime/Core/Serialization/Archive.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"

// UE's Internationalization/StringTable.cpp and StringTableCore.cpp (UE 5.3).

namespace ue5parse::runtime {

namespace {

// The display string of a string table text: the table's source string (the runtime has no localization).
FString ResolveStringTableEntry(FName TableId, const FString& Key) {
    const UStringTable* Table = LoadObject<UStringTable>(nullptr, NameToString(TableId));
    FString Source;
    if (Table != nullptr) Table->GetSourceString(Key, Source);
    return Source;
}

// Installs the resolver when the program starts, as the Engine module's startup does.
const bool GStringTableResolverInstalled = [] {
    SetStringTableResolver(&ResolveStringTableEntry);
    return true;
}();

}

void UStringTable::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    Ar << TableNamespace;
    const int32 EntryCount = Ar.ReadCount(8);
    KeysToEntries.reserve(static_cast<std::size_t>(EntryCount));
    for (int32 i = 0; i < EntryCount && !Ar.IsError(); i++) {
        FString Key, SourceString;
        Ar << Key << SourceString;
        KeysToEntries.insert_or_assign(Key.ToUtf8(), std::move(SourceString));
    }
    // KeysToMetaData: key, then meta data name and text pairs. Nothing reads them.
    const int32 MetaDataCount = Ar.ReadCount(8);
    for (int32 i = 0; i < MetaDataCount && !Ar.IsError(); i++) {
        FString Key;
        Ar << Key;
        const int32 FieldCount = Ar.ReadCount(12);
        for (int32 f = 0; f < FieldCount && !Ar.IsError(); f++) {
            FName Name;
            FString Text;
            Ar << Name << Text;
        }
    }
}

FName UStringTable::GetStringTableId() const { return FName(GetPathName().ToUtf8()); }

bool UStringTable::GetSourceString(const FString& Key, FString& OutSourceString) const {
    const auto Found = KeysToEntries.find(Key.ToUtf8());
    if (Found == KeysToEntries.end()) return false;
    OutSourceString = Found->second;
    return true;
}

}

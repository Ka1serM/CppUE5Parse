#include "Engine/Source/Runtime/Engine/Internationalization/StringTable.h"

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

namespace {

FString ResolveStringTableEntry(FName TableId, const FString& Key) {
    const UStringTable* Table = LoadObject<UStringTable>(nullptr, NameToString(TableId));
    FString Source;
    if (Table != nullptr)
        Table->GetSourceString(Key, Source);
    return Source;
}

const bool GStringTableResolverInstalled = [] {
    SetStringTableResolver(&ResolveStringTableEntry);
    return true;
}();

}

void UStringTable::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    Ar << TableNamespace;
    const int32 EntryCount = Ar.ReadCount(8);
    KeysToEntries.reserve(static_cast<std::size_t>(EntryCount));
    for (int32 i = 0; i < EntryCount && !Ar.IsError(); i++) {
        FString Key, SourceString;
        Ar << Key << SourceString;
        KeysToEntries.insert_or_assign(Key.ToUtf8(), std::move(SourceString));
    }
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

bool UStringTable::GetSourceString(const FString& Key, FString& OutSourceString) const {
    const auto Found = KeysToEntries.find(Key.ToUtf8());
    if (Found == KeysToEntries.end())
        return false;
    OutSourceString = Found->second;
    return true;
}

}

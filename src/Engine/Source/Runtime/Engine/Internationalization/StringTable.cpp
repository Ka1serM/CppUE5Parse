#include "Engine/Source/Runtime/Engine/Internationalization/StringTable.h"

#include <charconv>

#include "Engine/Source/Runtime/Core/Internationalization/Text.h"
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

//! Marvel Rivals saves one more string after each source string, Lost Records: Bloom & Rage a string and a counted list of strings.
void SkipGameEntryData(FArchive& Ar) {
    FString Skipped;
    if (Ar.GetGame() == EGame::GAME_MarvelRivals) {
        Ar << Skipped;
        return;
    }
    if (Ar.GetGame() != EGame::GAME_LostRecordsBloomAndRage)
        return;
    FString CountText;
    Ar << Skipped << CountText;
    const std::string Digits = CountText.ToUtf8();
    int32 Count = 0;
    const auto [End, Error] = std::from_chars(Digits.data(), Digits.data() + Digits.size(), Count);
    if (Error != std::errc() || End != Digits.data() + Digits.size())
        Count = 0;
    for (int32 i = 0; i < Count && !Ar.IsError(); i++)
        Ar << Skipped;
}

}

void UStringTable::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    Ar << TableNamespace;
    const int32 EntryCount = Ar.ReadCount(8);
    KeysToEntries.reserve(static_cast<std::size_t>(EntryCount));
    const EGame Game = Ar.GetGame();
    for (int32 i = 0; i < EntryCount && !Ar.IsError(); i++) {
        FString Key;
        Ar << Key;
        FString SourceString;
        if (Game == EGame::GAME_CodeVein2)
            SourceString = SerializeCodeVein2String(Ar, CodeVein2StringKey::StringTable);
        else
            Ar << SourceString;
        SkipGameEntryData(Ar);
        KeysToEntries.insert_or_assign(Key.ToUtf8(), std::move(SourceString));
    }
    if (Game == EGame::GAME_Wildgate)
        return;
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

void UStringTable::EnumerateSourceStrings(const std::function<bool(const FString&, const FString&)>& InEnumerator) const {
    for (const auto& [Key, SourceString] : KeysToEntries)
        if (!InEnumerator(FString(Key), SourceString))
            return;
}

}

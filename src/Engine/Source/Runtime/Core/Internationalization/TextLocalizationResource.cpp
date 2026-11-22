#include "Engine/Source/Runtime/Core/Internationalization/TextLocalizationResource.h"

#include <string>
#include <utility>

#include "CppUE5Parse/GameTypes/NevernessToEverness/NevernessToEvernessLocres.h"
#include "Engine/Source/Runtime/Core/Internationalization/Text.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

namespace {

constexpr FGuid LocResMagic{0x7574140E, 0xFC034A67, 0x9D90154A, 0x1B7F37C3};

struct FLocalizedString {
    FString String;
    int32 RefCount = -1;
};

FString SerializeTextKey(FArchive& Ar, ELocResVersion Version) {
    if (Version >= ELocResVersion::Optimized_CRC32)
        Ar.Read<uint32>();
    FString Text;
    Ar << Text;
    return Text;
}

bool IsLocalizationPath(const FString& LocResID, std::string_view Prefix, bool bAnywhere) {
    std::string Path = LocResID.ToUtf8();
    if (Path.starts_with('/'))
        Path.erase(0, 1);
    return bAnywhere ? Path.find(Prefix) != std::string::npos : Path.starts_with(Prefix);
}

//! Neverness to Everness writes a format version, and from version 10000 encrypts each localized string.
bool ReadNevernessToEvernessStrings(FArchive& Ar, TArray<FLocalizedString>& Strings) {
    const int32 Version = Ar.Read<int32>();
    bool bEncrypted = Version >= 10000;
    if (Version >= 10100)
        Ar << bEncrypted;
    const int64 Offset = Ar.Read<int64>();
    if (Ar.IsError() || Offset == INDEX_NONE)
        return !Ar.IsError();
    const int64 Resume = Ar.Tell();
    Ar.Seek(Offset);
    const int32 Count = Ar.ReadCount(8);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        FLocalizedString& Entry = Strings.Emplace_GetRef();
        Ar << Entry.String;
        if (bEncrypted) {
            Result<std::string> Decrypted = decryptNevernessToEvernessLocresString(Entry.String.ToUtf8());
            if (!Decrypted) {
                Ar.SetError(Decrypted.error().code, "FTextLocalizationResource.NevernessToEverness");
                return false;
            }
            Entry.String = FString(std::string_view(*Decrypted));
        }
        Ar << Entry.RefCount;
    }
    Ar.Seek(Resume);
    return !Ar.IsError();
}

bool ReadLocalizedStrings(FArchive& Ar, const FString& LocResID, ELocResVersion Version, TArray<FLocalizedString>& Strings) {
    const EGame Game = Ar.GetGame();
    const bool bNevernessToEverness = Game == EGame::GAME_NevernessToEverness || Game == EGame::GAME_NevernessToEverness_CBT2;
    if (bNevernessToEverness && IsLocalizationPath(LocResID, "HT/Content/Localization/", false))
        return ReadNevernessToEvernessStrings(Ar, Strings);
    const int64 Offset = Ar.Read<int64>();
    if (Ar.IsError() || Offset == INDEX_NONE)
        return !Ar.IsError();
    const int64 Resume = Ar.Tell();
    Ar.Seek(Offset);
    const bool bCodeVein2 = Game == EGame::GAME_CodeVein2 && IsLocalizationPath(LocResID, "CodeVein2/Content/Localization/", true);
    const int32 Count = Ar.ReadCount(4);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        FLocalizedString& Entry = Strings.Emplace_GetRef();
        if (bCodeVein2) {
            Entry.String = SerializeCodeVein2String(Ar, CodeVein2StringKey::Locres);
        } else if (Game == EGame::GAME_EmbersofTheUncrowned) {
            FString Stored;
            Ar << Stored;
            std::optional<FString> Decrypted = DecryptEmbersOfTheUncrownedString(Stored);
            if (!Decrypted) {
                Ar.SetError(ErrorCode::InvalidData, "FTextLocalizationResource.EmbersOfTheUncrowned");
                return false;
            }
            Entry.String = std::move(*Decrypted);
        } else {
            Ar << Entry.String;
        }
        if (bCodeVein2 || Game == EGame::GAME_EmbersofTheUncrowned || Version >= ELocResVersion::Optimized_CRC32)
            Ar << Entry.RefCount;
    }
    Ar.Seek(Resume);
    return !Ar.IsError();
}

//! Honor of Kings: World saves the tables of its own format in one empty namespace.
bool LoadHonorOfKingsWorld(FArchive& Ar, const FString& LocResID, TMap<FTextId, FTextLocalizationResource::FEntry>& Entries) {
    const int32 HashCount = Ar.ReadCount(4);
    Ar.SkipBytes(static_cast<int64>(HashCount) * 4);
    const int32 TableCount = Ar.ReadCount(12);
    TArray<int32> EntryCounts;
    for (int32 i = 0; i < TableCount && !Ar.IsError(); i++) {
        FString TableName;
        int32 EntryCount = 0, Unknown = 0;
        Ar << TableName << EntryCount << Unknown;
        EntryCounts.Add(EntryCount);
    }
    for (int32 EntryCount : EntryCounts) {
        if (EntryCount < 0 || EntryCount > Ar.GetRemaining() / 20) {
            Ar.SetError(ErrorCode::InvalidData, "FTextLocalizationResource.HonorOfKingsWorld");
            return false;
        }
        for (int32 i = 0; i < EntryCount && !Ar.IsError(); i++) {
            FTextLocalizationResource::FEntry Entry{LocResID};
            FString Key, Unknown;
            Ar.Read<uint32>();
            Ar << Key;
            Ar << Entry.SourceStringHash << Entry.LocalizedString << Unknown;
            Entries.Add(FTextId{FString(), std::move(Key)}, std::move(Entry));
        }
    }
    return !Ar.IsError();
}

}

bool FTextLocalizationResource::LoadFromArchive(FArchive& Archive, const FString& LocResID) {
    FGuid Magic;
    Archive << Magic;
    ELocResVersion Version = ELocResVersion::Legacy;
    if (Magic == LocResMagic)
        Version = static_cast<ELocResVersion>(Archive.Read<uint8>());
    else
        Archive.Seek(0);
    const EGame Game = Archive.GetGame();
    if (Version > ELocResVersion::Latest) {
        if (Game == EGame::GAME_HonorofKingsWorld)
            return LoadHonorOfKingsWorld(Archive, LocResID, Entries);
        if (Game != EGame::GAME_StellarBlade) {
            Archive.SetError(ErrorCode::UnsupportedVersion, "FTextLocalizationResource.Version");
            return false;
        }
    }
    TArray<FLocalizedString> LocalizedStrings;
    if (Version >= ELocResVersion::Compact && !ReadLocalizedStrings(Archive, LocResID, Version, LocalizedStrings))
        return false;
    if (Version >= ELocResVersion::Optimized_CRC32)
        Archive.Read<uint32>();
    const uint32 NamespaceCount = Archive.Read<uint32>();
    for (uint32 i = 0; i < NamespaceCount && !Archive.IsError(); i++) {
        const FString Namespace = SerializeTextKey(Archive, Version);
        const uint32 KeyCount = Archive.Read<uint32>();
        for (uint32 j = 0; j < KeyCount && !Archive.IsError(); j++) {
            FString Key = SerializeTextKey(Archive, Version);
            FEntry Entry{LocResID};
            Archive << Entry.SourceStringHash;
            if (Version < ELocResVersion::Compact) {
                Archive << Entry.LocalizedString;
                Entries.Add(FTextId{Namespace, std::move(Key)}, std::move(Entry));
                continue;
            }
            const int32 Index = Archive.Read<int32>();
            if (Index >= 0 && Index < LocalizedStrings.Num())
                Entry.LocalizedString = LocalizedStrings[Index].String;
            if (Game == EGame::GAME_StellarBlade && Version > ELocResVersion::Latest)
                Archive.SkipBytes(4);
            Entries.Add(FTextId{Namespace, std::move(Key)}, std::move(Entry));
        }
    }
    return !Archive.IsError();
}

}

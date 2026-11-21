#include "Engine/Source/Runtime/Core/Internationalization/TextLocalizationResource.h"

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

namespace {

enum class ELocResVersion : uint8 { Legacy, Compact, Optimized_CRC32, Optimized_CityHash64_UTF16 };

void ReadKey(FArchive& Ar, ELocResVersion Version, FString& Key) {
    if (Version >= ELocResVersion::Optimized_CRC32)
        Ar.SkipBytes(sizeof(uint32));
    Ar << Key;
}

}

bool FTextLocalizationResource::LoadFromArchive(FArchive& Ar) {
    ELocResVersion Version = ELocResVersion::Legacy;
    constexpr FGuid Magic{0x7574140E, 0xFC034A67, 0x9D90154A, 0x1B7F37C3};
    if (Ar.TotalSize() >= sizeof(FGuid) && Ar.Read<FGuid>() == Magic)
        Version = Ar.Read<ELocResVersion>();
    else
        Ar.Seek(0);
    if (Version > ELocResVersion::Optimized_CityHash64_UTF16) {
        Ar.SetError(ErrorCode::InvalidData, "FTextLocalizationResource.Version");
        return false;
    }

    std::vector<FString> NewStrings;
    if (Version >= ELocResVersion::Compact) {
        const int64 Offset = Ar.Read<int64>();
        if (Offset != INDEX_NONE) {
            const int64 Resume = Ar.Tell();
            Ar.Seek(Offset);
            const int32 Count = Ar.ReadCount(Version >= ELocResVersion::Optimized_CRC32 ? 8 : 4);
            NewStrings.reserve(Count);
            for (int32 I = 0; I < Count && !Ar.IsError(); ++I) {
                Ar << NewStrings.emplace_back();
                if (Version >= ELocResVersion::Optimized_CRC32)
                    Ar.SkipBytes(sizeof(int32));
            }
            Ar.Seek(Resume);
        }
    }

    if (Version >= ELocResVersion::Optimized_CRC32)
        Ar.SkipBytes(sizeof(uint32));
    const int32 NamespaceCount = Ar.ReadCount(8);
    std::vector<FNamespace> NewNamespaces;
    NewNamespaces.reserve(NamespaceCount);
    for (int32 I = 0; I < NamespaceCount && !Ar.IsError(); ++I) {
        auto& Namespace = NewNamespaces.emplace_back();
        ReadKey(Ar, Version, Namespace.Name);
        const int32 KeyCount = Ar.ReadCount(12);
        Namespace.Entries.reserve(KeyCount);
        for (int32 J = 0; J < KeyCount && !Ar.IsError(); ++J) {
            auto& Entry = Namespace.Entries.emplace_back();
            ReadKey(Ar, Version, Entry.Key);
            Ar << Entry.SourceStringHash;
            if (Version >= ELocResVersion::Compact)
                Ar << Entry.StringIndex;
            else {
                Entry.StringIndex = static_cast<int32>(NewStrings.size());
                Ar << NewStrings.emplace_back();
            }
        }
    }
    if (Ar.IsError())
        return false;
    Namespaces = std::move(NewNamespaces);
    Strings = std::move(NewStrings);
    return true;
}

const FString* FTextLocalizationResource::GetLocalizedString(const FEntry& Entry) const {
    if (Entry.StringIndex < 0 || static_cast<std::size_t>(Entry.StringIndex) >= Strings.size())
        return nullptr;
    return &Strings[Entry.StringIndex];
}

}

#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"

namespace ue5parse::runtime {

class FArchive;

enum class ELocResVersion : uint8 {
    Legacy = 0,
    Compact,
    Optimized_CRC32,
    Optimized_CityHash64_UTF16,
    LatestPlusOne,
    Latest = LatestPlusOne - 1
};

struct FTextId {
    FString Namespace;
    FString Key;
    bool operator==(const FTextId& Other) const {
        return Namespace.View() == Other.Namespace.View() && Key.View() == Other.Key.View();
    }
};

inline uint32 GetTypeHash(const FTextId& Id) { return HashCombine(GetTypeHash(Id.Namespace), GetTypeHash(Id.Key)); }

class FTextLocalizationResource {
public:
    struct FEntry {
        FString LocResID;
        uint32 SourceStringHash = 0;
        FString LocalizedString;
    };

    //! The resource path selects the string encryption of Neverness to Everness and Code Vein II.
    bool LoadFromArchive(FArchive& Archive, const FString& LocResID);
    const FEntry* FindEntry(const FTextId& Id) const { return Entries.Find(Id); }
    const TMap<FTextId, FEntry>& GetEntries() const { return Entries; }

private:
    TMap<FTextId, FEntry> Entries;
};

}

#pragma once

#include <vector>

#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"

namespace ue5parse::runtime {

class FArchive;

class FTextLocalizationResource {
public:
    struct FEntry {
        FString Key;
        uint32 SourceStringHash = 0;
        int32 StringIndex = INDEX_NONE;
    };

    struct FNamespace {
        FString Name;
        std::vector<FEntry> Entries;
    };

    bool LoadFromArchive(FArchive& Ar);
    const std::vector<FNamespace>& GetNamespaces() const { return Namespaces; }
    const FString* GetLocalizedString(const FEntry& Entry) const;

private:
    std::vector<FNamespace> Namespaces;
    std::vector<FString> Strings;
};

}

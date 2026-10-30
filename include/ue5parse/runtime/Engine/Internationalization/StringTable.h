#pragma once

#include <string>
#include <unordered_map>

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "StringTable.generated.h"

namespace ue5parse::runtime {

// UStringTable: the texts that string table FTexts name by key (UE's Internationalization/StringTable.h, with the
// table itself, UE's FStringTable). The table is serialized natively.
UCLASS()
class UStringTable : public UObject {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;
    // The id that texts name the table by: its path, "/Game/Text/ST_Agents.ST_Agents".
    FName GetStringTableId() const;
    const FString& GetNamespace() const { return TableNamespace; }
    // The source string of a key; false when the table has no such key.
    bool GetSourceString(const FString& Key, FString& OutSourceString) const;

private:
    FString TableNamespace;
    // By key; keys compare with case, as UE's FTextKey.
    std::unordered_map<std::string, FString> KeysToEntries;
};

}

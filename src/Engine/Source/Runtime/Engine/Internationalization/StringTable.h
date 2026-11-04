#pragma once

#include <string>
#include <unordered_map>

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "StringTable.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UStringTable : public UObject {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;
    const FString& GetNamespace() const { return TableNamespace; }
    bool GetSourceString(const FString& Key, FString& OutSourceString) const;

private:
    FString TableNamespace;
    std::unordered_map<std::string, FString> KeysToEntries;
};

}

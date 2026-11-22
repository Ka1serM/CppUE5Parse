#pragma once

#include <atomic>
#include <optional>

#include "CppUE5Parse/GameTypes/CodeVein2/CodeVein2StringEncryption.h"

#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

class FArchive;

namespace ETextFlag {

enum Type : uint32 {
    Transient = 1 << 0,
    CultureInvariant = 1 << 1,
    ConvertedProperty = 1 << 2,
    Immutable = 1 << 3,
    InitializedFromString = 1 << 4,
};

}

class FText {
public:
    FText() = default;
    FText(const FText& Other);
    FText(FText&& Other) noexcept : Data(Other.Data), Flags(Other.Flags) { Other.Data = nullptr; }
    FText& operator=(const FText& Other);
    FText& operator=(FText&& Other) noexcept;
    ~FText();

    static FText FromString(FString String);
    static FText AsCultureInvariant(FString String);
    static FText MakeLocalized(FString Source, FString Namespace, FString Key);
    static FText FromStringTable(FName TableId, FString Key);
    static const FText& GetEmpty();

    const FString& ToString() const;
    bool IsEmpty() const { return ToString().IsEmpty(); }
    uint32 GetFlags() const { return Flags; }
    void SetFlags(uint32 InFlags) { Flags = InFlags; }
    const FString& GetNamespace() const;
    const FString& GetKey() const;
    bool IsCultureInvariant() const { return (Flags & ETextFlag::CultureInvariant) != 0; }
    bool IsFromStringTable() const;
    bool IdenticalTo(const FText& Other) const;
    bool operator==(const FText& Other) const { return IdenticalTo(Other); }

    struct FTextData;

private:
    friend class FTextInspector;

    FTextData* Data = nullptr;
    uint32 Flags = 0;
};

class FTextInspector {
public:
    static bool GetTableIdAndKey(const FText& Text, FName& OutTableId, FString& OutKey);
};

using FStringTableResolver = FString (*)(FName TableId, const FString& Key);
void SetStringTableResolver(FStringTableResolver Resolver);

FArchive& operator<<(FArchive& Ar, FText& Value);

//! Embers of the Uncrowned stores source strings as base64 of UTF-16 XORed with a key and marks them with the suffix "ENCRYPTED".
std::optional<FString> DecryptEmbersOfTheUncrownedString(const FString& Stored);
//! Code Vein II reads an FString whose characters it XORs with the locres or the string table key.
FString SerializeCodeVein2String(FArchive& Ar, CodeVein2StringKey Key);

}

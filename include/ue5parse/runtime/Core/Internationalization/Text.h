#pragma once

#include <atomic>

#include "ue5parse/runtime/Core/Containers/UnrealString.h"
#include "ue5parse/runtime/Core/UObject/NameTypes.h"

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

// FText: shared immutable text data and flags, as UE's TSharedRef<ITextData>. Bitwise relocatable. The runtime has
// no localization: the display string is the source string (or the string table's entry).
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
    // A localized literal: source string, namespace and key.
    static FText MakeLocalized(FString Source, FString Namespace, FString Key);
    // An entry of a string table; its display string is looked up on first use (SetStringTableResolver).
    static FText FromStringTable(FName TableId, FString Key);
    static const FText& GetEmpty();

    const FString& ToString() const;
    bool IsEmpty() const { return ToString().IsEmpty(); }
    bool IsCultureInvariant() const { return (Flags & ETextFlag::CultureInvariant) != 0; }
    bool IsFromStringTable() const;
    uint32 GetFlags() const { return Flags; }
    void SetFlags(uint32 InFlags) { Flags = InFlags; }
    const FString& GetNamespace() const;
    const FString& GetKey() const;
    FName GetTableId() const;
    // Whether the texts share their data, or are both empty: UE's FText::IdenticalTo.
    bool IdenticalTo(const FText& Other) const;
    // UE's EqualTo with the invariant culture: the display strings compared with case.
    bool EqualTo(const FText& Other) const { return ToString().Equals(Other.ToString()); }
    bool operator==(const FText& Other) const { return IdenticalTo(Other); }

    struct FTextData;

private:
    FTextData* Data = nullptr;
    uint32 Flags = 0;
};

// The display string of a string table entry ("/Game/Text/ST_Agents", "Key"). The Engine module installs the
// resolver that loads UStringTable assets; without one, entries display as empty.
using FStringTableResolver = FString (*)(FName TableId, const FString& Key);
void SetStringTableResolver(FStringTableResolver Resolver);

// Reads a cooked FText (its flags and text history). Only the base and string table histories keep their own
// strings; a formatted text shows the source string of its format, as without localization data.
FArchive& operator<<(FArchive& Ar, FText& Value);

}

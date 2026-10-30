#pragma once

#include <string>
#include <string_view>

#include "ue5parse/runtime/Core/Containers/Array.h"

namespace ue5parse::runtime {

namespace ESearchCase {
enum Type { CaseSensitive, IgnoreCase };
}
namespace ESearchDir {
enum Type { FromStart, FromEnd };
}

// FString: UTF-16 code units with a terminating zero, as UE's TCHAR string. An empty string holds no memory. Lengths
// and indices count code units, as in UE.
class FString {
public:
    FString() = default;
    FString(const char* Utf8) : FString(std::string_view(Utf8)) {}
    // From UTF-8.
    FString(std::string_view Utf8);
    FString(std::u16string_view Text);
    FString(const TCHAR* Text) : FString(std::u16string_view(Text)) {}

    int32 Len() const { return Data.Num() ? Data.Num() - 1 : 0; }
    bool IsEmpty() const { return Data.Num() <= 1; }
    bool IsValidIndex(int32 Index) const { return Index >= 0 && Index < Len(); }
    // The text with its terminator; "" for an empty string.
    const TCHAR* operator*() const { return Data.Num() ? Data.GetData() : u""; }
    std::u16string_view View() const { return std::u16string_view(**this, static_cast<std::size_t>(Len())); }
    std::string ToUtf8() const;
    TCHAR& operator[](int32 Index) { return Data[Index]; }
    TCHAR operator[](int32 Index) const { return Data[Index]; }
    TArray<TCHAR>& GetCharArray() { return Data; }
    const TArray<TCHAR>& GetCharArray() const { return Data; }

    void Empty(int32 Slack = 0) { Data.Empty(Slack); }
    void Reset() { Data.Reset(); }
    FString& Append(std::u16string_view Text);
    FString& operator+=(const FString& Other) { return Append(Other.View()); }
    FString& operator+=(std::u16string_view Text) { return Append(Text); }
    FString& operator+=(const TCHAR* Text) { return Append(std::u16string_view(Text)); }
    FString& operator+=(TCHAR Char) { return Append(std::u16string_view(&Char, 1)); }
    FString& operator+=(const char* Utf8) { return *this += FString(Utf8); }
    friend FString operator+(const FString& A, const FString& B) {
        FString Result(A);
        Result += B;
        return Result;
    }

    // UE compares strings without case: operator== is Equals(Other, IgnoreCase).
    bool operator==(const FString& Other) const { return Equals(Other, ESearchCase::IgnoreCase); }
    bool Equals(const FString& Other, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive) const;
    int32 Compare(const FString& Other, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive) const;
    int32 Find(const FString& SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase,
               ESearchDir::Type SearchDir = ESearchDir::FromStart, int32 StartPosition = INDEX_NONE) const;
    bool Contains(const FString& SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase,
                  ESearchDir::Type SearchDir = ESearchDir::FromStart) const {
        return Find(SubStr, SearchCase, SearchDir) != INDEX_NONE;
    }
    bool StartsWith(const FString& Prefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;
    bool EndsWith(const FString& Suffix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;
    FString Mid(int32 Start, int32 Count = 0x7fffffff) const;
    FString Left(int32 Count) const { return Mid(0, Count); }
    FString Right(int32 Count) const;
    FString LeftChop(int32 Count) const { return Mid(0, Len() - Count); }
    FString RightChop(int32 Count) const { return Mid(Count); }
    FString ToUpper() const;
    FString ToLower() const;
    FString TrimStart() const;
    FString TrimEnd() const;
    FString TrimStartAndEnd() const { return TrimStart().TrimEnd(); }
    FString Replace(const FString& From, const FString& To, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;
    FString Reverse() const;
    // Splits at each delimiter; empty parts are left out when CullEmpty is set.
    int32 ParseIntoArray(TArray<FString>& Out, const FString& Delimiter, bool CullEmpty = true) const;

private:
    TArray<TCHAR> Data;
};

// UE's GetTypeHash(FString): without case.
uint32 GetTypeHash(const FString& Text);

namespace FChar {
TCHAR ToUpper(TCHAR Char);
TCHAR ToLower(TCHAR Char);
bool IsWhitespace(TCHAR Char);
}

}

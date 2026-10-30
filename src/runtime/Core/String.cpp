#include <algorithm>
#include <memory>
#include <mutex>
#include <deque>

#include <ankerl/unordered_dense.h>

#include "ue5parse/runtime/Core/Containers/UnrealString.h"
#include "ue5parse/runtime/Core/Internationalization/Text.h"
#include "ue5parse/runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

namespace {

void AppendUtf8AsUtf16(TArray<TCHAR>& Out, std::string_view Utf8) {
    std::size_t i = 0;
    while (i < Utf8.size()) {
        uint32 c = static_cast<uint8>(Utf8[i]);
        uint32 Code;
        int Extra;
        if (c < 0x80) {
            Code = c;
            Extra = 0;
        } else if ((c >> 5) == 0x6) {
            Code = c & 0x1f;
            Extra = 1;
        } else if ((c >> 4) == 0xe) {
            Code = c & 0x0f;
            Extra = 2;
        } else if ((c >> 3) == 0x1e) {
            Code = c & 0x07;
            Extra = 3;
        } else {
            Code = 0xfffd;
            Extra = 0;
        }
        i++;
        for (int k = 0; k < Extra; k++, i++) {
            if (i >= Utf8.size() || (static_cast<uint8>(Utf8[i]) >> 6) != 0x2) {
                Code = 0xfffd;
                break;
            }
            Code = (Code << 6) | (static_cast<uint8>(Utf8[i]) & 0x3f);
        }
        if (Code >= 0x10000) {
            Code -= 0x10000;
            Out.Add(static_cast<TCHAR>(0xd800 + (Code >> 10)));
            Out.Add(static_cast<TCHAR>(0xdc00 + (Code & 0x3ff)));
        } else {
            Out.Add(static_cast<TCHAR>(Code));
        }
    }
}

void AppendCodePoint(std::string& Out, uint32 Code) {
    if (Code < 0x80) {
        Out.push_back(static_cast<char>(Code));
    } else if (Code < 0x800) {
        Out.push_back(static_cast<char>(0xc0 | (Code >> 6)));
        Out.push_back(static_cast<char>(0x80 | (Code & 0x3f)));
    } else if (Code < 0x10000) {
        Out.push_back(static_cast<char>(0xe0 | (Code >> 12)));
        Out.push_back(static_cast<char>(0x80 | ((Code >> 6) & 0x3f)));
        Out.push_back(static_cast<char>(0x80 | (Code & 0x3f)));
    } else {
        Out.push_back(static_cast<char>(0xf0 | (Code >> 18)));
        Out.push_back(static_cast<char>(0x80 | ((Code >> 12) & 0x3f)));
        Out.push_back(static_cast<char>(0x80 | ((Code >> 6) & 0x3f)));
        Out.push_back(static_cast<char>(0x80 | (Code & 0x3f)));
    }
}

bool CharsEqual(TCHAR A, TCHAR B, ESearchCase::Type SearchCase) {
    return SearchCase == ESearchCase::CaseSensitive ? A == B : FChar::ToUpper(A) == FChar::ToUpper(B);
}

}

TCHAR FChar::ToUpper(TCHAR Char) {
    // UE's TChar<WIDECHAR>::ToUpper: ASCII and Latin-1 letters (towupper of the C runtime on other platforms; the
    // Latin-1 range covers what Blueprint strings hold).
    if (Char >= u'a' && Char <= u'z') return static_cast<TCHAR>(Char - 32);
    if ((Char >= 0xe0 && Char <= 0xfe && Char != 0xf7)) return static_cast<TCHAR>(Char - 32);
    return Char;
}

TCHAR FChar::ToLower(TCHAR Char) {
    if (Char >= u'A' && Char <= u'Z') return static_cast<TCHAR>(Char + 32);
    if ((Char >= 0xc0 && Char <= 0xde && Char != 0xd7)) return static_cast<TCHAR>(Char + 32);
    return Char;
}

bool FChar::IsWhitespace(TCHAR Char) {
    return Char == u' ' || Char == u'\t' || Char == u'\n' || Char == u'\r' || Char == u'\v' || Char == u'\f' ||
           Char == 0x85 || Char == 0xa0 || Char == 0x1680 || (Char >= 0x2000 && Char <= 0x200a) || Char == 0x2028 ||
           Char == 0x2029 || Char == 0x202f || Char == 0x205f || Char == 0x3000;
}

FString::FString(std::string_view Utf8) {
    if (Utf8.empty()) return;
    Data.Reserve(static_cast<int32>(Utf8.size()) + 1);
    AppendUtf8AsUtf16(Data, Utf8);
    Data.Add(0);
}

FString::FString(std::u16string_view Text) { Append(Text); }

std::string FString::ToUtf8() const {
    std::string Out;
    std::u16string_view Text = View();
    Out.reserve(Text.size());
    for (std::size_t i = 0; i < Text.size(); i++) {
        uint32 Code = Text[i];
        if (Code >= 0xd800 && Code < 0xdc00 && i + 1 < Text.size() && Text[i + 1] >= 0xdc00 && Text[i + 1] < 0xe000) {
            Code = 0x10000 + ((Code - 0xd800) << 10) + (Text[i + 1] - 0xdc00);
            i++;
        }
        AppendCodePoint(Out, Code);
    }
    return Out;
}

FString& FString::Append(std::u16string_view Text) {
    if (Text.empty()) return *this;
    if (Data.Num() > 0) Data.RemoveAt(Data.Num() - 1);
    Data.Reserve(Data.Num() + static_cast<int32>(Text.size()) + 1);
    for (TCHAR Char : Text) Data.Add(Char);
    Data.Add(0);
    return *this;
}

bool FString::Equals(const FString& Other, ESearchCase::Type SearchCase) const {
    if (Len() != Other.Len()) return false;
    for (int32 i = 0; i < Len(); i++)
        if (!CharsEqual((*this)[i], Other[i], SearchCase)) return false;
    return true;
}

int32 FString::Compare(const FString& Other, ESearchCase::Type SearchCase) const {
    // FCString::Strcmp / Stricmp: by code unit, the shorter string first on a common prefix.
    int32 Count = std::min(Len(), Other.Len());
    for (int32 i = 0; i < Count; i++) {
        TCHAR A = (*this)[i];
        TCHAR B = Other[i];
        if (SearchCase == ESearchCase::IgnoreCase) {
            A = FChar::ToLower(A);
            B = FChar::ToLower(B);
        }
        if (A != B) return A < B ? -1 : 1;
    }
    return Len() == Other.Len() ? 0 : (Len() < Other.Len() ? -1 : 1);
}

int32 FString::Find(const FString& SubStr, ESearchCase::Type SearchCase, ESearchDir::Type SearchDir,
                    int32 StartPosition) const {
    int32 SubLen = SubStr.Len();
    if (SubLen == 0) return INDEX_NONE;
    auto MatchesAt = [&](int32 At) {
        for (int32 k = 0; k < SubLen; k++)
            if (!CharsEqual((*this)[At + k], SubStr[k], SearchCase)) return false;
        return true;
    };
    if (SearchDir == ESearchDir::FromStart) {
        int32 Start = StartPosition == INDEX_NONE ? 0 : std::clamp(StartPosition, 0, Len());
        for (int32 i = Start; i + SubLen <= Len(); i++)
            if (MatchesAt(i)) return i;
    } else {
        // From the end: StartPosition is where the search starts going back from (the match must end before it).
        int32 Start = StartPosition == INDEX_NONE || StartPosition > Len() ? Len() : StartPosition;
        for (int32 i = Start - SubLen; i >= 0; i--)
            if (MatchesAt(i)) return i;
    }
    return INDEX_NONE;
}

bool FString::StartsWith(const FString& Prefix, ESearchCase::Type SearchCase) const {
    if (Prefix.Len() > Len()) return false;
    for (int32 i = 0; i < Prefix.Len(); i++)
        if (!CharsEqual((*this)[i], Prefix[i], SearchCase)) return false;
    return true;
}

bool FString::EndsWith(const FString& Suffix, ESearchCase::Type SearchCase) const {
    if (Suffix.Len() > Len()) return false;
    int32 Offset = Len() - Suffix.Len();
    for (int32 i = 0; i < Suffix.Len(); i++)
        if (!CharsEqual((*this)[Offset + i], Suffix[i], SearchCase)) return false;
    return true;
}

FString FString::Mid(int32 Start, int32 Count) const {
    // UE clamps: a negative start counts from 0 and shortens the count.
    int64 End = int64(Start) + Count;
    int64 First = std::clamp<int64>(Start, 0, Len());
    int64 Last = std::clamp<int64>(End, First, Len());
    return FString(View().substr(static_cast<std::size_t>(First), static_cast<std::size_t>(Last - First)));
}

FString FString::Right(int32 Count) const {
    int32 Clamped = std::clamp(Count, 0, Len());
    return Mid(Len() - Clamped, Clamped);
}

FString FString::ToUpper() const {
    FString Result(*this);
    for (int32 i = 0; i < Result.Len(); i++) Result[i] = FChar::ToUpper(Result[i]);
    return Result;
}

FString FString::ToLower() const {
    FString Result(*this);
    for (int32 i = 0; i < Result.Len(); i++) Result[i] = FChar::ToLower(Result[i]);
    return Result;
}

FString FString::TrimStart() const {
    int32 First = 0;
    while (First < Len() && FChar::IsWhitespace((*this)[First])) First++;
    return Mid(First);
}

FString FString::TrimEnd() const {
    int32 End = Len();
    while (End > 0 && FChar::IsWhitespace((*this)[End - 1])) End--;
    return Mid(0, End);
}

FString FString::Replace(const FString& From, const FString& To, ESearchCase::Type SearchCase) const {
    if (IsEmpty() || From.IsEmpty()) return *this;
    FString Result;
    int32 Position = 0;
    for (;;) {
        int32 Found = Find(From, SearchCase, ESearchDir::FromStart, Position);
        if (Found == INDEX_NONE) break;
        Result += View().substr(Position, Found - Position);
        Result += To;
        Position = Found + From.Len();
    }
    Result += View().substr(Position);
    return Result;
}

FString FString::Reverse() const {
    FString Result(*this);
    std::reverse(Result.Data.begin(), Result.Data.begin() + Result.Len());
    return Result;
}

int32 FString::ParseIntoArray(TArray<FString>& Out, const FString& Delimiter, bool CullEmpty) const {
    Out.Reset();
    if (Delimiter.IsEmpty()) {
        if (!IsEmpty()) Out.Add(*this);
        return Out.Num();
    }
    int32 Position = 0;
    for (;;) {
        int32 Found = Find(Delimiter, ESearchCase::CaseSensitive, ESearchDir::FromStart, Position);
        int32 End = Found == INDEX_NONE ? Len() : Found;
        if (!CullEmpty || End > Position) Out.Add(Mid(Position, End - Position));
        if (Found == INDEX_NONE) break;
        Position = Found + Delimiter.Len();
    }
    return Out.Num();
}

uint32 GetTypeHash(const FString& Text) {
    uint32 Hash = 2166136261u;
    for (int32 i = 0; i < Text.Len(); i++) {
        Hash ^= FChar::ToUpper(Text[i]);
        Hash *= 16777619u;
    }
    return Hash;
}

namespace {

// The texts of names made at run time. Each text is kept once, for the process's life.
class FNamePool {
public:
    FName Find(std::string_view Text) {
        FName Probe(Text);
        std::lock_guard Lock(Mutex);
        auto Found = Names.find(Probe.getHash());
        if (Found != Names.end()) return FName(Found->second, Probe.getHash(), Probe.getNumber());
        // The number suffix was split off: keep the base text only.
        std::string_view Base = Probe.getText();
        const std::string& Kept = Texts.emplace_back(Base);
        Names.emplace(Probe.getHash(), std::string_view(Kept));
        return FName(Kept, Probe.getHash(), Probe.getNumber());
    }

private:
    std::mutex Mutex;
    std::deque<std::string> Texts;
    ankerl::unordered_dense::map<uint64, std::string_view> Names;
};

FNamePool& GetNamePool() {
    static FNamePool Pool;
    return Pool;
}

}

FName MakeName(std::string_view Text) { return GetNamePool().Find(Text); }

FName MakeName(const FString& Text) { return MakeName(Text.ToUtf8()); }

FString NameToString(FName Name) { return FString(Name.toString()); }

struct FText::FTextData {
    std::atomic<int32> RefCount{1};
    FString Display;
    FString Namespace;
    FString Key;
    FName TableId;
    bool FromStringTable = false;
    // A string table entry finds its display string on first use (the table is an asset that loads then).
    std::once_flag ResolveOnce;
};

namespace {

FStringTableResolver GStringTableResolver = nullptr;

}

void SetStringTableResolver(FStringTableResolver Resolver) { GStringTableResolver = Resolver; }

namespace {

FText::FTextData* AddRef(FText::FTextData* Data) {
    if (Data != nullptr) Data->RefCount.fetch_add(1, std::memory_order_relaxed);
    return Data;
}

void Release(FText::FTextData* Data) {
    if (Data != nullptr && Data->RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1) delete Data;
}

const FString& EmptyString() {
    static const FString Empty;
    return Empty;
}

}

FText::FText(const FText& Other) : Data(AddRef(Other.Data)), Flags(Other.Flags) {}

FText& FText::operator=(const FText& Other) {
    if (this == &Other) return *this;
    FTextData* Old = Data;
    Data = AddRef(Other.Data);
    Flags = Other.Flags;
    Release(Old);
    return *this;
}

FText& FText::operator=(FText&& Other) noexcept {
    if (this == &Other) return *this;
    Release(Data);
    Data = Other.Data;
    Flags = Other.Flags;
    Other.Data = nullptr;
    return *this;
}

FText::~FText() { Release(Data); }

FText FText::FromString(FString String) {
    FText Text;
    Text.Data = new FTextData();
    Text.Data->Display = std::move(String);
    Text.Flags = ETextFlag::InitializedFromString;
    return Text;
}

FText FText::AsCultureInvariant(FString String) {
    FText Text = FromString(std::move(String));
    Text.Flags = ETextFlag::CultureInvariant;
    return Text;
}

FText FText::MakeLocalized(FString Source, FString Namespace, FString Key) {
    FText Text;
    Text.Data = new FTextData();
    Text.Data->Display = std::move(Source);
    Text.Data->Namespace = std::move(Namespace);
    Text.Data->Key = std::move(Key);
    return Text;
}

FText FText::FromStringTable(FName TableId, FString Key) {
    FText Text;
    Text.Data = new FTextData();
    Text.Data->Key = std::move(Key);
    Text.Data->TableId = TableId;
    Text.Data->FromStringTable = true;
    return Text;
}

const FText& FText::GetEmpty() {
    static const FText Empty;
    return Empty;
}

const FString& FText::ToString() const {
    if (Data == nullptr) return EmptyString();
    if (Data->FromStringTable) {
        std::call_once(Data->ResolveOnce, [this] {
            if (GStringTableResolver != nullptr) Data->Display = GStringTableResolver(Data->TableId, Data->Key);
        });
    }
    return Data->Display;
}
bool FText::IsFromStringTable() const { return Data && Data->FromStringTable; }
const FString& FText::GetNamespace() const { return Data ? Data->Namespace : EmptyString(); }
const FString& FText::GetKey() const { return Data ? Data->Key : EmptyString(); }
FName FText::GetTableId() const { return Data ? Data->TableId : FName(); }

bool FText::IdenticalTo(const FText& Other) const {
    if (Data == Other.Data) return true;
    return IsEmpty() && Other.IsEmpty();
}

}

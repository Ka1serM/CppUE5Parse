#include <string>

#include "ue5parse/core/Text.h"
#include "ue5parse/runtime/Core/Internationalization/Text.h"
#include "ue5parse/runtime/Core/Serialization/Archive.h"
#include "ue5parse/runtime/CoreUObject/UObject/ObjectPtr.h"

namespace ue5parse::runtime {

FArchive& FArchive::operator<<(FName& Value) {
    Value = FName();
    SetError(ErrorCode::InvalidData, "FArchive.FName");
    return *this;
}

FArchive& FArchive::operator<<(UObject*& Value) {
    Value = nullptr;
    SetError(ErrorCode::InvalidData, "FArchive.UObject");
    return *this;
}

FArchive& FArchive::operator<<(FObjectPtr& Value) {
    UObject* Object = nullptr;
    *this << Object;
    Value = Object;
    return *this;
}

FArchive& FArchive::operator<<(FSoftObjectPath&) {
    SetError(ErrorCode::InvalidData, "FArchive.FSoftObjectPath");
    return *this;
}

FArchive& FArchive::operator<<(FSoftObjectPtr&) {
    SetError(ErrorCode::InvalidData, "FArchive.FSoftObjectPtr");
    return *this;
}

FArchive& FArchive::operator<<(FWeakObjectPtr&) {
    SetError(ErrorCode::InvalidData, "FArchive.FWeakObjectPtr");
    return *this;
}

FString FArchive::ReadUtf8String() {
    int32 Count = ReadCount(1);
    if (Count == 0) return FString();
    std::string Bytes(static_cast<std::size_t>(Count), '\0');
    Serialize(Bytes.data(), Count);
    if (IsError()) return FString();
    return FString(std::string_view(Bytes));
}

FArchive& operator<<(FArchive& Ar, FString& Value) {
    // The length with the terminator; negative for UTF-16.
    int32 SaveNum = 0;
    Ar << SaveNum;
    Value.Empty();
    if (Ar.IsError() || SaveNum == 0) return Ar;
    const bool bWide = SaveNum < 0;
    const int64 Count = bWide ? -static_cast<int64>(SaveNum) : static_cast<int64>(SaveNum);
    const int64 ByteCount = bWide ? Count * 2 : Count;
    if (Count > 0x7fffffff || ByteCount > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "FString");
        return Ar;
    }
    if (bWide) {
        std::u16string Text(static_cast<std::size_t>(Count), u'\0');
        Ar.Serialize(Text.data(), ByteCount);
        if (Ar.IsError()) return Ar;
        if (!Text.empty() && Text.back() == u'\0') Text.pop_back();
        Value = FString(std::u16string_view(Text));
        return Ar;
    }
    // Latin-1, one byte per character.
    std::string Bytes(static_cast<std::size_t>(Count), '\0');
    Ar.Serialize(Bytes.data(), ByteCount);
    if (Ar.IsError()) return Ar;
    if (!Bytes.empty() && Bytes.back() == '\0') Bytes.pop_back();
    std::u16string Text;
    Text.reserve(Bytes.size());
    for (char Byte : Bytes) Text.push_back(static_cast<char16_t>(static_cast<uint8>(Byte)));
    Value = FString(std::u16string_view(Text));
    return Ar;
}

namespace {

// ETextHistoryType.
enum class ETextHistoryType : int8 {
    None = -1,
    Base = 0,
    NamedFormat,
    OrderedFormat,
    ArgumentFormat,
    AsNumber,
    AsPercent,
    AsCurrency,
    AsDate,
    AsTime,
    AsDateTime,
    Transform,
    StringTableEntry,
    TextGenerator,
};

constexpr int32 MaxTextDepth = 32;

FText ReadText(FArchive& Ar, int32 Depth);

// A format argument value (FFormatArgumentValue): the display string of a number, or a text.
FString ReadFormatArgument(FArchive& Ar, int32 Depth) {
    uint8 Type = 0;
    Ar << Type;
    switch (Type) {
    case 0: {
        int64 Value = 0;
        Ar << Value;
        return FString(std::to_string(Value));
    }
    case 1: {
        uint64 Value = 0;
        Ar << Value;
        return FString(std::to_string(Value));
    }
    case 2: {
        float Value = 0;
        Ar << Value;
        return FString(std::to_string(Value));
    }
    case 3: {
        double Value = 0;
        Ar << Value;
        return FString(std::to_string(Value));
    }
    case 4:
        return ReadText(Ar, Depth + 1).ToString();
    default:
        Ar.SetError(ErrorCode::InvalidData, "FText.FormatArgument");
        return FString();
    }
}

FText ReadText(FArchive& Ar, int32 Depth) {
    if (Depth > MaxTextDepth) {
        Ar.SetError(ErrorCode::InvalidData, "FText.Depth");
        return FText();
    }
    uint32 Flags = 0;
    int8 History = 0;
    Ar << Flags << History;
    FText Result;
    switch (static_cast<ETextHistoryType>(History)) {
    case ETextHistoryType::None: {
        bool bHasCultureInvariantString = false;
        Ar << bHasCultureInvariantString;
        if (bHasCultureInvariantString) {
            FString String;
            Ar << String;
            Result = FText::AsCultureInvariant(std::move(String));
        }
        break;
    }
    case ETextHistoryType::Base: {
        FString Namespace, Key, Source;
        Ar << Namespace << Key << Source;
        Result = FText::MakeLocalized(std::move(Source), std::move(Namespace), std::move(Key));
        break;
    }
    case ETextHistoryType::NamedFormat:
    case ETextHistoryType::OrderedFormat:
    case ETextHistoryType::ArgumentFormat: {
        FText Format = ReadText(Ar, Depth + 1);
        int32 Count = Ar.ReadCount(1);
        for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
            if (History != static_cast<int8>(ETextHistoryType::OrderedFormat)) {
                FString Name;
                Ar << Name;
            }
            ReadFormatArgument(Ar, Depth + 1);
        }
        Result = FText::FromString(Format.ToString());
        break;
    }
    case ETextHistoryType::AsNumber:
    case ETextHistoryType::AsPercent:
    case ETextHistoryType::AsCurrency: {
        if (History == static_cast<int8>(ETextHistoryType::AsCurrency)) {
            FString CurrencyCode;
            Ar << CurrencyCode;
        }
        FString Value = ReadFormatArgument(Ar, Depth + 1);
        bool bHasFormatOptions = false;
        Ar << bHasFormatOptions;
        if (bHasFormatOptions) {
            // FNumberFormattingOptions: AlwaysSign, UseGrouping, RoundingMode, and four digit counts.
            Ar.SkipBytes(4 + 4 + 1 + 4 * 4);
        }
        FString TargetCulture;
        Ar << TargetCulture;
        Result = FText::FromString(std::move(Value));
        break;
    }
    case ETextHistoryType::AsDate:
    case ETextHistoryType::AsTime:
    case ETextHistoryType::AsDateTime: {
        int64 Ticks = 0;
        uint8 DateStyle = 0, TimeStyle = 0;
        Ar << Ticks << DateStyle;
        if (History == static_cast<int8>(ETextHistoryType::AsDateTime)) Ar << TimeStyle;
        FString TimeZone, TargetCulture;
        Ar << TimeZone << TargetCulture;
        Result = FText::FromString(FString(std::to_string(Ticks)));
        break;
    }
    case ETextHistoryType::Transform: {
        FText Source = ReadText(Ar, Depth + 1);
        uint8 TransformType = 0;
        Ar << TransformType;
        Result = FText::FromString(TransformType == 0 ? Source.ToString().ToLower() : Source.ToString().ToUpper());
        break;
    }
    case ETextHistoryType::StringTableEntry: {
        FName TableId;
        FString Key;
        Ar << TableId << Key;
        Result = FText::FromStringTable(TableId, std::move(Key));
        break;
    }
    case ETextHistoryType::TextGenerator: {
        FName GeneratorTypeId;
        Ar << GeneratorTypeId;
        break;
    }
    default:
        Ar.SetError(ErrorCode::InvalidData, "FText.History");
        break;
    }
    Result.SetFlags(Flags);
    return Result;
}

}

FArchive& operator<<(FArchive& Ar, FText& Value) {
    Value = ReadText(Ar, 0);
    return Ar;
}

}

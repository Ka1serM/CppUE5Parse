#include "GameTypes/OctopathTraveler/BinaryAsset.h"

#include <bit>
#include <cstdio>
#include <cstring>

namespace ue5parse::runtime {

namespace {

constexpr int32 MaxDepth = 64;

class FMessagePackReader {
public:
    explicit FMessagePackReader(std::span<const uint8> InBytes) : Bytes(InBytes) {}

    bool ReadValue(std::string& Out, int32 Depth) {
        if (Depth > MaxDepth || Position >= Bytes.size())
            return false;
        const uint8 Type = Bytes[Position++];
        if (Type <= 0x7F) {
            Out += std::to_string(Type);
            return true;
        }
        if (Type >= 0xE0) {
            Out += std::to_string(static_cast<int8>(Type));
            return true;
        }
        if (Type <= 0x8F)
            return ReadMap(Out, Type & 0x0F, Depth);
        if (Type <= 0x9F)
            return ReadArray(Out, Type & 0x0F, Depth);
        if (Type <= 0xBF)
            return ReadString(Out, Type & 0x1F);
        switch (Type) {
        case 0xC0:
            Out += "null";
            return true;
        case 0xC2:
        case 0xC3:
            Out += (Type & 1) ? "true" : "false";
            return true;
        case 0xCA:
            return ReadFloat<uint32, float>(Out);
        case 0xCB:
            return ReadFloat<uint64, double>(Out);
        case 0xCC:
            return ReadNumber<uint8, uint8>(Out);
        case 0xCD:
            return ReadNumber<uint16, uint16>(Out);
        case 0xCE:
            return ReadNumber<uint32, uint32>(Out);
        case 0xCF:
            return ReadNumber<uint64, uint64>(Out);
        case 0xD0:
            return ReadNumber<uint8, int8>(Out);
        case 0xD1:
            return ReadNumber<uint16, int16>(Out);
        case 0xD2:
            return ReadNumber<uint32, int32>(Out);
        case 0xD3:
            return ReadNumber<uint64, int64>(Out);
        case 0xD9:
        case 0xDA:
        case 0xDB:
            return ReadSizedString(Out, Type);
        case 0xDC:
        case 0xDD: {
            uint32 Count = 0;
            return ReadLength(Type == 0xDC ? 2 : 4, Count) && ReadArray(Out, Count, Depth);
        }
        case 0xDE:
        case 0xDF: {
            uint32 Count = 0;
            return ReadLength(Type == 0xDE ? 2 : 4, Count) && ReadMap(Out, Count, Depth);
        }
        default:
            return false;
        }
    }

    bool AtEnd() const { return Position == Bytes.size(); }

private:
    bool ReadLength(int32 Size, uint32& OutLength) {
        if (Bytes.size() - Position < static_cast<std::size_t>(Size))
            return false;
        OutLength = 0;
        for (int32 i = 0; i < Size; i++)
            OutLength = (OutLength << 8) | Bytes[Position++];
        return true;
    }

    template <class StoredType, class ValueType>
    bool ReadNumber(std::string& Out) {
        uint32 High = 0, Low = 0;
        if constexpr (sizeof(StoredType) == 8) {
            if (!ReadLength(4, High) || !ReadLength(4, Low))
                return false;
        } else if (!ReadLength(sizeof(StoredType), Low)) {
            return false;
        }
        const StoredType Stored = static_cast<StoredType>((static_cast<uint64>(High) << 32) | Low);
        const ValueType Value = std::bit_cast<ValueType>(Stored);
        if constexpr (sizeof(ValueType) == 1)
            Out += std::to_string(static_cast<int32>(Value));
        else
            Out += std::to_string(Value);
        return true;
    }

    template <class StoredType, class ValueType>
    bool ReadFloat(std::string& Out) {
        uint32 High = 0, Low = 0;
        if constexpr (sizeof(StoredType) == 8) {
            if (!ReadLength(4, High) || !ReadLength(4, Low))
                return false;
        } else if (!ReadLength(4, Low)) {
            return false;
        }
        const ValueType Value = std::bit_cast<ValueType>(static_cast<StoredType>((static_cast<uint64>(High) << 32) | Low));
        char Text[32];
        std::snprintf(Text, sizeof(Text), "%.17g", static_cast<double>(Value));
        Out += Text;
        return true;
    }

    bool ReadSizedString(std::string& Out, uint8 Type) {
        uint32 Length = 0;
        return ReadLength(Type == 0xD9 ? 1 : Type == 0xDA ? 2 : 4, Length) && ReadString(Out, Length);
    }

    bool ReadString(std::string& Out, uint32 Length) {
        if (Bytes.size() - Position < Length)
            return false;
        Out += '"';
        for (uint32 i = 0; i < Length; i++) {
            const char Character = static_cast<char>(Bytes[Position++]);
            if (Character == '"' || Character == '\\') {
                Out += '\\';
                Out += Character;
            } else if (static_cast<uint8>(Character) < 0x20) {
                char Escaped[8];
                std::snprintf(Escaped, sizeof(Escaped), "\\u%04x", static_cast<uint8>(Character));
                Out += Escaped;
            } else {
                Out += Character;
            }
        }
        Out += '"';
        return true;
    }

    bool ReadKey(std::string& Out) {
        if (Position >= Bytes.size())
            return false;
        const uint8 Type = Bytes[Position++];
        if (Type >= 0xA0 && Type <= 0xBF)
            return ReadString(Out, Type & 0x1F);
        if (Type >= 0xD9 && Type <= 0xDB)
            return ReadSizedString(Out, Type);
        return false;
    }

    bool ReadArray(std::string& Out, uint32 Count, int32 Depth) {
        if (Count > Bytes.size() - Position)
            return false;
        Out += '[';
        for (uint32 i = 0; i < Count; i++) {
            if (i > 0)
                Out += ',';
            if (!ReadValue(Out, Depth + 1))
                return false;
        }
        Out += ']';
        return true;
    }

    //! A member without a value is invalid, as Octopath Traveler saves no nil members.
    bool ReadMap(std::string& Out, uint32 Count, int32 Depth) {
        if (Count > (Bytes.size() - Position) / 2)
            return false;
        Out += '{';
        for (uint32 i = 0; i < Count; i++) {
            if (i > 0)
                Out += ',';
            if (!ReadKey(Out))
                return false;
            Out += ':';
            if (Position < Bytes.size() && Bytes[Position] == 0xC0)
                return false;
            if (!ReadValue(Out, Depth + 1))
                return false;
        }
        Out += '}';
        return true;
    }

    std::span<const uint8> Bytes;
    std::size_t Position = 0;
};

}

std::optional<std::string> UBinaryAsset::DecodeMessagePack(std::span<const uint8> Bytes) {
    FMessagePackReader Reader(Bytes);
    std::string Json;
    if (!Reader.ReadValue(Json, 0) || !Reader.AtEnd())
        return std::nullopt;
    return Json;
}

void UBinaryAsset::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    const EGame Game = Ar.GetGame();
    if (Ar.IsError() || (Game != EGame::GAME_OctopathTraveler0 && Game != EGame::GAME_OctopathTravelerCoTC))
        return;
    const std::span<const uint8> Bytes(BinaryData.GetData(), static_cast<std::size_t>(BinaryData.Num()));
    if (GetName().View().starts_with(u"VillageBuildingFillMap")) {
        Data = FString(std::string_view(reinterpret_cast<const char*>(Bytes.data()), Bytes.size()));
        return;
    }
    std::optional<std::string> Json = DecodeMessagePack(Bytes);
    if (!Json) {
        Ar.SetError(ErrorCode::InvalidData, "UBinaryAsset.BinaryData");
        return;
    }
    Data = FString(std::string_view(*Json));
}

}

#include "GameTypes/FinalFantasy7Rebirth/MemoryMappedAsset.h"

#include <cstring>

namespace ue5parse::runtime {

void FMemoryMappedImageReader::ReadBytes(void* Data, int64 Size) {
    std::span<const std::byte> Bytes = Image->GetFrozenBytes();
    if (bError || Position < 0 || Size < 0 || static_cast<uint64>(Position) + static_cast<uint64>(Size) > Bytes.size()) {
        std::memset(Data, 0, static_cast<std::size_t>(std::max<int64>(Size, 0)));
        bError = true;
        return;
    }
    std::memcpy(Data, Bytes.data() + Position, static_cast<std::size_t>(Size));
    Position += Size;
}

int64 FMemoryMappedImageReader::ReadPointerOffset() {
    const uint64 Packed = Read<uint64>();
    return Image->GetGame() >= EGame::GAME_UE5_0 ? static_cast<int64>(Packed) >> 24 : static_cast<int64>(Packed) >> 1;
}

int64 FMemoryMappedImageReader::ReadArrayHeader(int32& OutNum, bool bRequireExactSize) {
    const int64 Start = Position;
    const int64 Data = Start + ReadPointerOffset();
    OutNum = Read<int32>();
    const int32 Max = Read<int32>();
    if (OutNum < 0 || (bRequireExactSize && OutNum != Max))
        bError = true;
    return Data;
}

FName FMemoryMappedImageReader::ReadName() {
    const uint32 Offset = static_cast<uint32>(Position);
    if (FName Name = Image->FindMemoryImageName(Offset); !Name.isNone()) {
        Position += 8;
        return Name;
    }
    if (FName Name = Image->FindScriptName(Offset); !Name.isNone()) {
        Position += 12;
        return Name;
    }
    Position += 8;
    return FName();
}

FString FMemoryMappedImageReader::ReadString() {
    int32 Num = 0;
    const int64 Data = ReadArrayHeader(Num, true);
    if (Num <= 1 || bError)
        return FString();
    const int64 Continue = Position;
    Position = Data;
    std::vector<char16_t> Characters(static_cast<std::size_t>(Num));
    ReadBytes(Characters.data(), static_cast<int64>(Num) * 2);
    Position = Continue;
    if (bError || Characters.back() != 0) {
        bError = true;
        return FString();
    }
    return FString(std::u16string_view(Characters.data(), Characters.size() - 1));
}

std::vector<bool> FMemoryMappedImageReader::ReadBitArray() {
    const int64 Start = Position;
    const int64 Data = Start + ReadPointerOffset();
    const int32 NumBits = Read<int32>();
    Read<int32>();
    std::vector<bool> Bits;
    if (NumBits <= 0 || bError)
        return Bits;
    const int64 Continue = Position;
    Position = Data;
    Bits.resize(static_cast<std::size_t>(NumBits));
    for (int32 Word = 0; Word < (NumBits + 31) / 32 && !bError; Word++) {
        const uint32 Value = Read<uint32>();
        for (int32 Bit = 0; Bit < 32 && Word * 32 + Bit < NumBits; Bit++)
            Bits[static_cast<std::size_t>(Word * 32 + Bit)] = (Value >> Bit) & 1u;
    }
    Position = Continue;
    return Bits;
}

void UMemoryMappedAsset::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    const uint32 FrozenSize = Ar.Read<uint32>();
    Ar.Read<uint32>();
    Ar.Read<uint16>();
    const uint16 Padding = Ar.Read<uint16>();
    Ar.SkipBytes(Padding);
    if (Ar.IsError() || FrozenSize > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "UMemoryMappedAsset.FrozenSize");
        return;
    }
    FrozenObject.resize(FrozenSize);
    Ar.Serialize(FrozenObject.data(), FrozenSize);
    Image = FMemoryImageResult::LoadPatchesFromArchive(Ar, FrozenObject);
}

}

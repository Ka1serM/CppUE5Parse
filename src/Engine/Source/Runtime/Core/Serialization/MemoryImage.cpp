#include "Engine/Source/Runtime/Core/Serialization/MemoryImage.h"

#include <algorithm>

namespace ue5parse::runtime {

std::string FSHAHash::ToString() const {
    static constexpr char Digits[] = "0123456789ABCDEF";
    std::string Text(sizeof(Hash) * 2, '0');
    for (std::size_t Index = 0; Index < sizeof(Hash); Index++) {
        Text[Index * 2] = Digits[Hash[Index] >> 4];
        Text[Index * 2 + 1] = Digits[Hash[Index] & 15];
    }
    return Text;
}

FHashedName::FHashedName(FName Name) {
    if (Name.isNone())
        return;
    std::string_view Text = Name.getText();
    char Upper[1024];
    std::size_t Size = std::min(Text.size(), sizeof(Upper));
    for (std::size_t Index = 0; Index < Size; Index++)
        Upper[Index] = Text[Index] >= 'a' && Text[Index] <= 'z' ? static_cast<char>(Text[Index] - 'a' + 'A') : Text[Index];
    Hash = ue5parse::cityHash64WithSeed(std::string_view(Upper, Size), Name.getNumber());
}

void FPointerTableBase::LoadFromArchive(FArchive& Ar) {
    int32 NumDependencies = Ar.ReadCount(32);
    TypeDependencies.Empty(NumDependencies);
    for (int32 Index = 0; Index < NumDependencies && !Ar.IsError(); Index++) {
        FTypeDependency& Dependency = TypeDependencies[TypeDependencies.AddDefaulted()];
        Ar << Dependency.Name << Dependency.SavedLayoutSize << Dependency.SavedLayoutHash;
    }
}

namespace {

template <class TPatch>
FName FindNamePatch(const std::vector<TPatch>& Patches, uint32 Offset) {
    auto Found = std::lower_bound(Patches.begin(), Patches.end(), Offset, [](const TPatch& Patch, uint32 Value) { return Patch.Offset < Value; });
    if (Found == Patches.end() || Found->Offset != Offset)
        return NAME_None;
    return Found->Name;
}

template <class TPatch>
void LoadNamePatches(FArchive& Ar, uint32 NumNames, uint32 FrozenSize, uint32 NameSize, std::vector<TPatch>& OutPatches) {
    for (uint32 Index = 0; Index < NumNames && !Ar.IsError(); Index++) {
        FName Name;
        Ar << Name;
        uint32 NumPatches = Ar.Read<uint32>();
        if (NumPatches > Ar.GetRemaining() / 4) {
            Ar.SetError(ErrorCode::ReadPastEnd, "FMemoryImageResult.NamePatches");
            return;
        }
        for (uint32 Patch = 0; Patch < NumPatches; Patch++) {
            uint32 Offset = Ar.Read<uint32>();
            if (static_cast<uint64>(Offset) + NameSize > FrozenSize) {
                Ar.SetError(ErrorCode::InvalidData, "FMemoryImageResult.NamePatchOffset");
                return;
            }
            OutPatches.push_back(TPatch{Offset, Name});
        }
    }
    std::sort(OutPatches.begin(), OutPatches.end(), [](const TPatch& A, const TPatch& B) { return A.Offset < B.Offset; });
}

constexpr uint32 FrozenScriptNameSize = 12u;
constexpr uint32 FrozenMemoryImageNameSize = 8u;

}

FName FMemoryImageObject::FindScriptName(uint32 Offset) const { return FindNamePatch(ScriptNames, Offset); }

FName FMemoryImageObject::FindMemoryImageName(uint32 Offset) const { return FindNamePatch(MemoryImageNames, Offset); }

FMemoryImageObject FMemoryImageResult::LoadFromArchive(FArchive& Ar, FPointerTableBase& PointerTable) {
    FMemoryImageObject Object;
    Object.Game = Ar.GetGame();
    Ar << Object.LayoutParameters;
    if (!Ar.IsError() && (Object.LayoutParameters.Is32Bit() || Object.LayoutParameters.WithEditorOnly() ||
                          Object.LayoutParameters.MaxFieldAlignment < 8u)) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FMemoryImageResult.LayoutParameters");
        return Object;
    }
    uint32 FrozenSize = Ar.Read<uint32>();
    Object.FrozenBytes = Ar.ReadView(FrozenSize);
    PointerTable.LoadFromArchive(Ar);

    uint32 NumVTables = Ar.Read<uint32>();
    uint32 NumScriptNames = Ar.Read<uint32>();
    uint32 NumMemoryImageNames = Ar.Read<uint32>();
    for (uint32 Index = 0; Index < NumVTables && !Ar.IsError(); Index++) {
        Ar.Read<uint64>();
        uint32 NumPatches = Ar.Read<uint32>();
        Ar.SkipBytes(static_cast<int64>(NumPatches) * 8);
    }
    LoadNamePatches(Ar, NumScriptNames, FrozenSize, FrozenScriptNameSize, Object.ScriptNames);
    LoadNamePatches(Ar, NumMemoryImageNames, FrozenSize, FrozenMemoryImageNameSize, Object.MemoryImageNames);
    return Object;
}

void FMemoryImageReader::ReadBytes(void* Data, uint32 Size) {
    std::span<const std::byte> Bytes = Object->GetFrozenBytes();
    if (IsError() || static_cast<uint64>(Offset) + Size > Bytes.size()) {
        std::memset(Data, 0, Size);
        SetError("FMemoryImageReader.ReadBytes");
        return;
    }
    std::memcpy(Data, Bytes.data() + Offset, Size);
    Offset += Size;
}

FName FMemoryImageReader::ReadMemoryImageName() {
    Align(4u);
    FName Name = IsError() ? NAME_None : Object->FindMemoryImageName(Offset);
    std::byte Skipped[FrozenMemoryImageNameSize];
    ReadBytes(Skipped, sizeof(Skipped));
    return Name;
}

FName FMemoryImageReader::ReadScriptName() {
    Align(4u);
    FName Name = IsError() ? NAME_None : Object->FindScriptName(Offset);
    std::byte Skipped[FrozenScriptNameSize];
    ReadBytes(Skipped, sizeof(Skipped));
    return Name;
}

std::optional<FMemoryImageReader> FMemoryImageReader::FollowPointer(uint32 FieldOffset, uint64 Packed) {
    if (Packed == 0u)
        return std::nullopt;
    if ((Packed & 1u) == 0u) {
        SetError("FMemoryImageReader.UnfrozenPointer");
        return std::nullopt;
    }
    int64 Target = static_cast<int64>(FieldOffset) + (static_cast<int64>(Packed) >> 24);
    if (Target < 0 || static_cast<uint64>(Target) > Object->GetFrozenBytes().size()) {
        SetError("FMemoryImageReader.PointerOffset");
        return std::nullopt;
    }
    return FMemoryImageReader(*Object, *FailedCheck, static_cast<uint32>(Target));
}

std::optional<FMemoryImageReader> FMemoryImageReader::ReadPointer() {
    Align(8u);
    uint32 FieldOffset = Offset;
    return FollowPointer(FieldOffset, Read<uint64>());
}

int32 FMemoryImageReader::ReadIndexedPointer() {
    uint64 Packed = Read<uint64>();
    if (Packed == 0u)
        return INDEX_NONE;
    if ((Packed & 1u) == 0u || (Packed >> 1) > static_cast<uint64>(INT32_MAX)) {
        SetError("FMemoryImageReader.IndexedPointer");
        return INDEX_NONE;
    }
    return static_cast<int32>(Packed >> 1);
}

std::optional<FMemoryImageReader> FMemoryImageReader::ReadArrayHeader(int32& OutNum, uint32 MinElementSize) {
    Align(8u);
    uint32 FieldOffset = Offset;
    uint64 Packed = Read<uint64>();
    int32 Num = Read<int32>();
    Read<int32>();
    OutNum = 0;
    if (IsError() || Num == 0)
        return std::nullopt;
    if (Num < 0 || static_cast<uint64>(Num) * MinElementSize > Object->GetFrozenBytes().size()) {
        SetError("FMemoryImageReader.ArrayNum");
        return std::nullopt;
    }
    std::optional<FMemoryImageReader> Data = FollowPointer(FieldOffset, Packed);
    if (!Data) {
        SetError("FMemoryImageReader.ArrayData");
        return std::nullopt;
    }
    OutNum = Num;
    return Data;
}

}

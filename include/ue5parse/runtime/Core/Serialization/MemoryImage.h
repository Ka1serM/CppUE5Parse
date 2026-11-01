#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "ue5parse/runtime/Core/Misc/SecureHash.h"
#include "ue5parse/runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

// UE's MemoryLayout.h and MemoryImage.h: the frozen memory images of the shader maps (UE 5.3). A frozen image is the
// memory of an object as the cooking target laid it out. FMemoryImageReader walks it with the target's layout rules.

struct FPlatformTypeLayoutParameters {
    enum EFlags : uint32 {
        Flag_Initialized = 1u << 0,
        Flag_Is32Bit = 1u << 1,
        Flag_AlignBases = 1u << 2,
        Flag_WithEditorOnly = 1u << 3,
    };

    uint32 MaxFieldAlignment = 0xffffffffu;
    uint32 Flags = 0u;

    bool Is32Bit() const { return (Flags & Flag_Is32Bit) != 0u; }
    bool HasAlignBases() const { return (Flags & Flag_AlignBases) != 0u; }
    bool WithEditorOnly() const { return (Flags & Flag_WithEditorOnly) != 0u; }
};

inline FArchive& operator<<(FArchive& Ar, FPlatformTypeLayoutParameters& Value) { return Ar << Value.MaxFieldAlignment << Value.Flags; }

// FHashedName: the CityHash64 of a name in upper case, seeded with its number. Shader and vertex factory types are known
// by it.
class FHashedName {
public:
    FHashedName() = default;
    explicit FHashedName(uint64 InHash) : Hash(InHash) {}
    explicit FHashedName(FName Name);
    explicit FHashedName(std::string_view Name) : FHashedName(FName(Name)) {}

    uint64 GetHash() const { return Hash; }
    bool IsNone() const { return Hash == 0u; }
    bool operator==(const FHashedName&) const = default;

private:
    uint64 Hash = 0u;
};

inline FArchive& operator<<(FArchive& Ar, FHashedName& Value) {
    uint64 Hash = Ar.Read<uint64>();
    Value = FHashedName(Hash);
    return Ar;
}

// FPointerTableBase: the type dependencies saved after the frozen object. A derived table reads its own types after
// them.
class FPointerTableBase {
public:
    struct FTypeDependency {
        FName Name;
        uint32 SavedLayoutSize = 0u;
        FSHAHash SavedLayoutHash;
    };

    virtual ~FPointerTableBase() = default;
    virtual void LoadFromArchive(FArchive& Ar);

    // The frozen size of a type as the cook computed it; 0 when the image does not depend on the type.
    uint32 FindSavedLayoutSize(FName Name) const;
    const TArray<FTypeDependency>& GetTypeDependencies() const { return TypeDependencies; }

private:
    TArray<FTypeDependency> TypeDependencies;
};

// The frozen object of a memory image, with the names that its patches write (UE's FMemoryImageObject). The frozen
// bytes are a view of the archive's memory.
class FMemoryImageObject {
public:
    std::span<const std::byte> GetFrozenBytes() const { return FrozenBytes; }
    const FPlatformTypeLayoutParameters& GetLayoutParameters() const { return LayoutParameters; }
    // The game of the archive the image was loaded from.
    EGame GetGame() const { return Game; }
    // The name that a patch writes at the offset; None where no patch writes one.
    FName FindScriptName(uint32 Offset) const;
    FName FindMemoryImageName(uint32 Offset) const;

private:
    friend struct FMemoryImageResult;

    struct FNamePatch {
        uint32 Offset = 0u;
        FName Name;
    };

    std::span<const std::byte> FrozenBytes;
    FPlatformTypeLayoutParameters LayoutParameters;
    EGame Game = EGame::GAME_UE5_LATEST;
    // Sorted by offset.
    std::vector<FNamePatch> ScriptNames;
    std::vector<FNamePatch> MemoryImageNames;
};

struct FMemoryImageResult {
    // The layout parameters, the frozen object, the pointer table, then the vtable, script name and memory image name
    // patches. A failure is set on the archive.
    static FMemoryImageObject LoadFromArchive(FArchive& Ar, FPointerTableBase& PointerTable);
};

// Reads a frozen object field by field with the rules of Freeze::DefaultWriteMemoryImage:
// - A field starts at the smaller of its type's alignment and MaxFieldAlignment.
// - A struct field, a base (with AlignBases) and an array element end padded to a multiple of their type's alignment.
// - Editor-only fields are not in a cooked image.
// - The bitfields of one type in a row share one storage unit of that type.
// A struct type T gives its frozen alignment as T::FrozenAlignment (the largest alignment of its fields, as
// Freeze::DefaultGetTargetAlignment) and its fields through a free function Unfreeze(FMemoryImageReader&, T&).
// The first failure is kept in FailedCheck, the name of the check: after it every read gives zero.
class FMemoryImageReader {
public:
    FMemoryImageReader(const FMemoryImageObject& InObject, const char*& OutFailedCheck) : Object(&InObject), FailedCheck(&OutFailedCheck) {}

    bool IsError() const { return *FailedCheck != nullptr; }
    EGame GetGame() const { return Object->GetGame(); }
    uint32 Tell() const { return Offset; }
    void Align(uint32 TypeAlignment) {
        uint32 Alignment = std::min(TypeAlignment, Object->GetLayoutParameters().MaxFieldAlignment);
        Offset = (Offset + Alignment - 1u) / Alignment * Alignment;
    }

    // A field whose C++ layout is its frozen layout: an intrinsic, an enum, FHashedName, FGuid, FSHAHash.
    template <class T>
        requires std::is_trivially_copyable_v<T>
    T Read() {
        T Value{};
        Align(alignof(T));
        ReadBytes(&Value, sizeof(T));
        return Value;
    }
    template <class T>
        requires requires { T::FrozenAlignment; }
    void ReadStruct(T& Value) {
        Align(T::FrozenAlignment);
        uint32 Start = Offset;
        Unfreeze(*this, Value);
        PadTo(Start, T::FrozenAlignment);
    }
    // A base at the start of a derived type: padded like a field when the target aligns bases.
    template <class T>
        requires requires { T::FrozenAlignment; }
    void ReadBase(T& Value) {
        uint32 Start = Offset;
        Unfreeze(*this, Value);
        if (Object->GetLayoutParameters().HasAlignBases())
            PadTo(Start, T::FrozenAlignment);
    }
    FName ReadMemoryImageName();
    FName ReadScriptName();
    // TMemoryImagePtr: a reader at the object it points to; empty for a null pointer.
    std::optional<FMemoryImageReader> ReadPointer();
    // TIndexedPtr: the index into the pointer table's list of the type; INDEX_NONE for a null pointer.
    int32 ReadIndexedPointer();

    // TMemoryImageArray (and FMemoryImageString as an array of TCHAR).
    template <class T>
    void ReadArray(TArray<T>& Out) {
        if constexpr (requires { T::FrozenAlignment; }) {
            ReadArray(Out, T::FrozenAlignment, [](FMemoryImageReader& Element, T& Value) { Unfreeze(Element, Value); });
        } else {
            static_assert(std::is_trivially_copyable_v<T>);
            int32 Num = 0;
            std::optional<FMemoryImageReader> Data = ReadArrayHeader(Num, sizeof(T));
            Out.Empty(Num);
            if (!Data)
                return;
            Data->Align(alignof(T));
            Out.AddUninitialized(Num);
            Data->ReadBytes(Out.GetData(), static_cast<uint32>(sizeof(T)) * static_cast<uint32>(Num));
        }
    }
    // ReadElement(FMemoryImageReader&, T&) reads one element; each starts aligned to ElementAlignment.
    template <class T, class FReadElement>
    void ReadArray(TArray<T>& Out, uint32 ElementAlignment, FReadElement&& ReadElement) {
        int32 Num = 0;
        std::optional<FMemoryImageReader> Data = ReadArrayHeader(Num, 1u);
        Out.Empty(Num);
        if (!Data)
            return;
        Out.AddDefaulted(Num);
        Data->Align(ElementAlignment);
        for (int32 Index = 0; Index < Num && !IsError(); Index++) {
            uint32 Start = Data->Offset;
            ReadElement(*Data, Out[Index]);
            Data->PadTo(Start, ElementAlignment);
        }
    }

    // Where names the check, a string literal.
    void SetError(const char* Where) {
        if (*FailedCheck == nullptr)
            *FailedCheck = Where;
    }

private:
    FMemoryImageReader(const FMemoryImageObject& InObject, const char*& OutFailedCheck, uint32 InOffset)
        : Object(&InObject), FailedCheck(&OutFailedCheck), Offset(InOffset) {}

    void ReadBytes(void* Data, uint32 Size);
    void PadTo(uint32 Start, uint32 Alignment) { Offset = Start + (Offset - Start + Alignment - 1u) / Alignment * Alignment; }
    // The target of a frozen pointer that was read at FieldOffset.
    std::optional<FMemoryImageReader> FollowPointer(uint32 FieldOffset, uint64 Packed);
    // The pointer, Num and Max of an array. A reader at the elements; empty when there are none.
    std::optional<FMemoryImageReader> ReadArrayHeader(int32& OutNum, uint32 MinElementSize);

    const FMemoryImageObject* Object;
    const char** FailedCheck;
    uint32 Offset = 0u;
};

}

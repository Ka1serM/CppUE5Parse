#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "Engine/Source/Runtime/Core/Misc/SecureHash.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

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

class FHashedName {
public:
    FHashedName() = default;
    explicit FHashedName(uint64 InHash) : Hash(InHash) {}
    explicit FHashedName(FName Name);
    explicit FHashedName(std::string_view Name) : FHashedName(FName(Name)) {}

    uint64 GetHash() const { return Hash; }
    bool operator==(const FHashedName&) const = default;

private:
    uint64 Hash = 0u;
};

inline FArchive& operator<<(FArchive& Ar, FHashedName& Value) {
    uint64 Hash = Ar.Read<uint64>();
    Value = FHashedName(Hash);
    return Ar;
}

class FPointerTableBase {
public:
    struct FTypeDependency {
        FName Name;
        uint32 SavedLayoutSize = 0u;
        FSHAHash SavedLayoutHash;
    };

    virtual ~FPointerTableBase() = default;
    virtual void LoadFromArchive(FArchive& Ar);

    const TArray<FTypeDependency>& GetTypeDependencies() const { return TypeDependencies; }

private:
    TArray<FTypeDependency> TypeDependencies;
};

class FMemoryImageObject {
public:
    std::span<const std::byte> GetFrozenBytes() const { return FrozenBytes; }
    const FPlatformTypeLayoutParameters& GetLayoutParameters() const { return LayoutParameters; }
    EGame GetGame() const { return Game; }
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
    std::vector<FNamePatch> ScriptNames;
    std::vector<FNamePatch> MemoryImageNames;
};

struct FMemoryImageResult {
    static FMemoryImageObject LoadFromArchive(FArchive& Ar, FPointerTableBase& PointerTable);
};

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
    std::optional<FMemoryImageReader> ReadPointer();
    int32 ReadIndexedPointer();

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
    void SkipArray() {
        int32 Num = 0;
        ReadArrayHeader(Num, 1u);
    }
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

    void SetError(const char* Where) {
        if (*FailedCheck == nullptr)
            *FailedCheck = Where;
    }

private:
    FMemoryImageReader(const FMemoryImageObject& InObject, const char*& OutFailedCheck, uint32 InOffset)
        : Object(&InObject), FailedCheck(&OutFailedCheck), Offset(InOffset) {}

    void ReadBytes(void* Data, uint32 Size);
    void PadTo(uint32 Start, uint32 Alignment) { Offset = Start + (Offset - Start + Alignment - 1u) / Alignment * Alignment; }
    std::optional<FMemoryImageReader> FollowPointer(uint32 FieldOffset, uint64 Packed);
    std::optional<FMemoryImageReader> ReadArrayHeader(int32& OutNum, uint32 MinElementSize);

    const FMemoryImageObject* Object;
    const char** FailedCheck;
    uint32 Offset = 0u;
};

}

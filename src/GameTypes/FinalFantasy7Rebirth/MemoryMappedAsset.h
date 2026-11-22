#pragma once

#include <functional>
#include <vector>

#include "Engine/Source/Runtime/Core/Serialization/MemoryImage.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "MemoryMappedAsset.generated.h"

namespace ue5parse::runtime {

//! Reads the frozen objects of Final Fantasy VII Rebirth the way CUE4Parse does: without implicit alignment, names looked up by offset.
class FMemoryMappedImageReader {
public:
    explicit FMemoryMappedImageReader(const FMemoryImageObject& InImage) : Image(&InImage) {}

    bool IsError() const { return bError; }
    int64 Tell() const { return Position; }
    void Seek(int64 InPosition) { Position = InPosition; }
    void Align(int64 Alignment) { Position = (Position + Alignment - 1) / Alignment * Alignment; }
    void SetError() { bError = true; }

    template <class T>
        requires std::is_trivially_copyable_v<T>
    T Read() {
        T Value{};
        ReadBytes(&Value, sizeof(T));
        return Value;
    }
    void ReadBytes(void* Data, int64 Size);
    FName ReadName();
    FString ReadString();
    std::vector<bool> ReadBitArray();

    template <class T>
        requires std::is_trivially_copyable_v<T>
    TArray<T> ReadRawArray() {
        TArray<T> Values;
        int32 Num = 0;
        int64 Data = ReadArrayHeader(Num, false);
        if (Num == 0 || bError)
            return Values;
        const int64 Continue = Position;
        Position = Data;
        for (int32 i = 0; i < Num && !bError; i++)
            Values.Add(Read<T>());
        Position = Continue;
        return Values;
    }

    template <class T, class ReadType>
    TArray<T> ReadArray(ReadType&& ReadElement, int64 Alignment = 1) {
        TArray<T> Values;
        int32 Num = 0;
        int64 Data = ReadArrayHeader(Num, true);
        if (Num == 0 || bError)
            return Values;
        const int64 Continue = Position;
        Position = Data;
        for (int32 i = 0; i < Num && !bError; i++) {
            Values.Add(ReadElement());
            Align(Alignment);
        }
        Position = Continue;
        return Values;
    }

    template <class T, class ReadType>
    TArray<T> ReadSparseArray(ReadType&& ReadElement, int64 ElementSize) {
        TArray<T> Values;
        const int64 Start = Position;
        const int64 Data = Start + ReadPointerOffset();
        const int32 Num = Read<int32>();
        Read<int32>();
        std::vector<bool> Allocated = ReadBitArray();
        Position += 8;
        if (Num <= 0 || bError)
            return Values;
        const int64 Continue = Position;
        Position = Data;
        for (int32 i = 0; i < Num && !bError; i++) {
            const int64 ElementStart = Position;
            if (static_cast<std::size_t>(i) < Allocated.size() && Allocated[static_cast<std::size_t>(i)])
                Values.Add(ReadElement());
            Position = ElementStart + ElementSize;
        }
        Position = Continue;
        return Values;
    }

    int64 ReadArrayHeader(int32& OutNum, bool bRequireExactSize);

private:
    int64 ReadPointerOffset();

    const FMemoryImageObject* Image;
    int64 Position = 0;
    bool bError = false;
};

UCLASS()
class UMemoryMappedAsset : public UObject {
    GENERATED_BODY()
public:
    explicit UMemoryMappedAsset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

protected:
    std::vector<std::byte> FrozenObject;
    FMemoryImageObject Image;
};

}

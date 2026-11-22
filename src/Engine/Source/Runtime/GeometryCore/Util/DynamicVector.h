#pragma once

#include <limits>
#include <span>

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

void SerializeCompressedNew(FArchive& Ar, std::span<std::byte> Output);

template <class Type>
class TDynamicVector {
public:
    void Serialize(FArchive& Ar) {
        Elements.Empty();
        uint32 SerializeNum = 0;
        Ar << SerializeNum;
        if (SerializeNum == 0 || Ar.IsError())
            return;
        bool bUseCompression = false;
        Ar << bUseCompression;
        const int64 Size = static_cast<int64>(SerializeNum) * static_cast<int64>(sizeof(Type));
        if (SerializeNum > static_cast<uint32>(std::numeric_limits<int32>::max()) || (!bUseCompression && Size > Ar.GetRemaining())) {
            Ar.SetError(ErrorCode::ReadPastEnd, "TDynamicVector.Num");
            return;
        }
        Elements.AddUninitialized(static_cast<int32>(SerializeNum));
        std::span<std::byte> Bytes(reinterpret_cast<std::byte*>(Elements.GetData()), static_cast<std::size_t>(Size));
        if (bUseCompression)
            SerializeCompressedNew(Ar, Bytes);
        else
            Ar.Serialize(Bytes.data(), Size);
    }

    int32 Num() const { return Elements.Num(); }
    const TArray<Type>& GetElements() const { return Elements; }

private:
    TArray<Type> Elements;
};

}

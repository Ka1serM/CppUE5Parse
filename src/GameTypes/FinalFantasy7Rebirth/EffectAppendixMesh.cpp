#include "GameTypes/FinalFantasy7Rebirth/EffectAppendixMesh.h"

#include "Engine/Source/Runtime/Engine/PackedNormal.h"

namespace ue5parse::runtime {

namespace {

constexpr int32 InfluenceCount = 4;

template <class T, class ReadType>
void ReadElements(FArchive& Ar, TArray<T>& Out, int32 Count, int32 ElementSize, ReadType&& Read) {
    if (Count < 0 || static_cast<int64>(Count) * ElementSize > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "UEffectAppendixMesh.Count");
        return;
    }
    Out.Empty(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++)
        Out.Add(Read());
}

void SkipUnused(FArchive& Ar, int32 BufferLength, int32 Used, int32 ElementSize) {
    Ar.SkipBytes(static_cast<int64>(BufferLength - Used) * ElementSize);
}

}

void UEffectAppendixMesh::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    Ar << Version << FullSize;
    const int64 HeaderStart = Ar.Tell();
    const int64 MeshOffset = HeaderStart + 20 + static_cast<int64>(Ar.Read<int32>()) * 2;
    Ar << SplinePathLength << TrianglesCount << VerticesCount;
    //! The top two bits of the offset half are flags.
    Offset = HalfToFloat(static_cast<uint16>(Ar.Read<uint16>() & 0x3FFF));
    TotalTime = HalfToFloat(Ar.Read<uint16>());
    ReadElements(Ar, SplinePath, SplinePathLength, 2, [&] { return Ar.Read<uint16>(); });
    ReadElements(Ar, Times, TrianglesCount, 2, [&] { return HalfToFloat(Ar.Read<uint16>()); });
    Scale1 = HalfToFloat(Ar.Read<uint16>());
    Scale2 = HalfToFloat(Ar.Read<uint16>());
    Ar.Seek(MeshOffset);

    int32 BufferLength = Ar.Read<int32>();
    ReadElements(Ar, SkinWeightVertexBuffer, VerticesCount, 12, [&] {
        FEffectAppendixSkinWeight Weight;
        for (int32 i = 0; i < InfluenceCount; i++)
            Weight.BoneIndex.Add(Ar.Read<uint16>());
        for (int32 i = 0; i < InfluenceCount; i++)
            Weight.BoneWeight.Add(Ar.Read<uint8>());
        return Weight;
    });
    SkipUnused(Ar, BufferLength, VerticesCount, 12);
    BufferLength = Ar.Read<int32>();
    ReadElements(Ar, IndexBuffer, TrianglesCount * 3, 2, [&] { return Ar.Read<uint16>(); });
    SkipUnused(Ar, BufferLength, TrianglesCount * 3, 2);
    BufferLength = Ar.Read<int32>();
    ReadElements(Ar, PositionVertexBuffer, VerticesCount, 12, [&] { return Ar.Read<FVector3f>(); });
    SkipUnused(Ar, BufferLength, VerticesCount, 12);
}

}

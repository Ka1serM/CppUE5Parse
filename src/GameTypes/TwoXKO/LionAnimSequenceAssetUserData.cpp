#include "GameTypes/TwoXKO/LionAnimSequenceAssetUserData.h"

namespace ue5parse::runtime {

namespace {

constexpr double FixedOne = 4294967296.0;

double ReadFixed64(FArchive& Ar) {
    return static_cast<double>(Ar.Read<int64>()) / FixedOne;
}

FVector ReadFixedVector(FArchive& Ar) {
    const double X = ReadFixed64(Ar);
    const double Y = ReadFixed64(Ar);
    const double Z = ReadFixed64(Ar);
    return FVector(X, Y, Z);
}

constexpr int32 FixedTransformKeySize = 88;
constexpr int32 FixedVectorKeySize = 32;

}

void ULionAnimSequenceAssetUserData::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetGame() != EGame::GAME_2XKO)
        return;
    const int32 NumTracks = Ar.ReadCount(12);
    for (int32 TrackIndex = 0; TrackIndex < NumTracks && !Ar.IsError(); TrackIndex++) {
        FName BoneName;
        Ar << BoneName;
        FLionAnimTransformTrack& Track = Data.FindOrAdd(BoneName);
        const int32 NumKeys = Ar.ReadCount(FixedTransformKeySize);
        for (int32 KeyIndex = 0; KeyIndex < NumKeys && !Ar.IsError(); KeyIndex++) {
            FLionAnimTransformKey& Key = Track.Keys[Track.Keys.AddDefaulted()];
            Key.Time = ReadFixed64(Ar);
            const double X = ReadFixed64(Ar);
            const double Y = ReadFixed64(Ar);
            const double Z = ReadFixed64(Ar);
            const double W = ReadFixed64(Ar);
            const FVector Translation = ReadFixedVector(Ar);
            const FVector Scale = ReadFixedVector(Ar);
            Key.Transform = FTransform(FQuat(X, Y, Z, W), Translation, Scale);
        }
    }
    const int32 NumVectors = Ar.ReadCount(FixedVectorKeySize);
    for (int32 Index = 0; Index < NumVectors && !Ar.IsError(); Index++) {
        FLionAnimVectorKey& Key = Vectors[Vectors.AddDefaulted()];
        Key.Time = ReadFixed64(Ar);
        Key.Value = ReadFixedVector(Ar);
    }
}

}

#include "ue5parse/runtime/Engine/AnimEncoding.h"

#include <bit>
#include <cmath>
#include <cstring>

#include "ue5parse/runtime/Engine/Animation/AnimCompress.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"

// The decoding of UE 5.3's AnimEncoding_ConstantKeyLerp, AnimEncoding_VariableKeyLerp, AnimEncoding_PerTrackCompression
// and AnimationCompression.h (FQuatFixed48NoW, FQuatIntervalFixed32NoW, FVectorIntervalFixed32NoW, ...).

namespace ue5parse::runtime {

namespace {

// Per-track: bytes per key component, and the component count by format and the low three header flags.
constexpr uint8 PerTrackComponentSize[ACF_Identity + 1] = {4, 4, 2, 4, 4, 4, 0};
constexpr uint8 PerTrackComponentCount[ACF_Identity + 1][8] = {
    {4, 4, 4, 4, 4, 4, 4, 4}, {3, 1, 1, 2, 1, 2, 2, 3}, {3, 1, 1, 2, 1, 2, 2, 3}, {6, 2, 2, 4, 2, 4, 4, 6},
    {1, 1, 1, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 1, 1, 1, 1}, {0, 0, 0, 0, 0, 0, 0, 0},
};
// Key-lerp: the stride of a key by format (CompressedRotationStrides, CompressedTranslationStrides).
constexpr uint8 CompressedRotationStrides[ACF_Identity + 1] = {16, 12, 6, 4, 4, 4, 0};
constexpr uint8 CompressedTranslationStrides[ACF_Identity + 1] = {12, 12, 0, 4, 0, 0, 0};

template <class T>
T ReadAt(const uint8* Data) {
    T Value;
    std::memcpy(&Value, Data, sizeof(T));
    return Value;
}

const uint8* Align4(const uint8* Ptr) { return reinterpret_cast<const uint8*>((reinterpret_cast<uintptr_t>(Ptr) + 3) & ~uintptr_t(3)); }

// TFloatPacker<3, MantissaBits, true>::Decode.
float UnpackFloat(uint32 Value, int MantissaBits) {
    if (Value == 0) return 0.0f;
    const uint32 Mantissa = Value & ((1u << MantissaBits) - 1u);
    const uint32 Exponent = ((Value >> MantissaBits) & 7u) - 4u + 127u;
    const uint32 Sign = Value >> (MantissaBits + 3);
    return std::bit_cast<float>((Sign << 31) | (Exponent << 23) | (Mantissa << (23 - MantissaBits)));
}

FQuat QuatWithW(float X, float Y, float Z) {
    const float WSquared = 1.0f - X * X - Y * Y - Z * Z;
    return FQuat(X, Y, Z, WSquared > 0.0f ? std::sqrt(WSquared) : 0.0f);
}

// A rotation key of a format; Mask is the per-track component mask (7 for the key-lerp codecs). Range holds the
// interval minimum and extent of each axis.
FQuat DecompressRotation(int32 Format, int32 Mask, const float* Range, const uint8* Data) {
    switch (Format) {
    case ACF_None: return FQuat(ReadAt<float>(Data), ReadAt<float>(Data + 4), ReadAt<float>(Data + 8), ReadAt<float>(Data + 12));
    case ACF_Float96NoW: return QuatWithW(ReadAt<float>(Data), ReadAt<float>(Data + 4), ReadAt<float>(Data + 8));
    case ACF_Fixed48NoW: {
        float Values[3];
        for (int Axis = 0; Axis < 3; ++Axis) {
            uint16 Quantized = 32767;
            if ((Mask & (1 << Axis)) != 0) {
                Quantized = ReadAt<uint16>(Data);
                Data += 2;
            }
            Values[Axis] = (static_cast<float>(Quantized) - 32767.0f) / 32767.0f;
        }
        return QuatWithW(Values[0], Values[1], Values[2]);
    }
    case ACF_IntervalFixed32NoW: {
        const uint32 Packed = ReadAt<uint32>(Data);
        return QuatWithW(((static_cast<int>(Packed >> 21) - 1023) / 1023.0f) * Range[1] + Range[0],
                         ((static_cast<int>((Packed >> 10) & 0x7ffu) - 1023) / 1023.0f) * Range[3] + Range[2],
                         ((static_cast<int>(Packed & 0x3ffu) - 511) / 511.0f) * Range[5] + Range[4]);
    }
    case ACF_Fixed32NoW: {
        const uint32 Packed = ReadAt<uint32>(Data);
        return QuatWithW((static_cast<int>(Packed >> 21) - 1023) / 1023.0f, (static_cast<int>((Packed >> 10) & 0x7ffu) - 1023) / 1023.0f,
                         (static_cast<int>(Packed & 0x3ffu) - 511) / 511.0f);
    }
    case ACF_Float32NoW: {
        const uint32 Packed = ReadAt<uint32>(Data);
        return QuatWithW(UnpackFloat(Packed >> 21, 7), UnpackFloat((Packed >> 10) & 0x7ffu, 7), UnpackFloat(Packed & 0x3ffu, 6));
    }
    default: return FQuat::Identity;
    }
}

// A translation or scale key. bPerTrack: the per-track codec's Fixed48 vectors (DecompressFixed16<7>).
FVector3f DecompressVector(int32 Format, int32 Mask, const float* Range, const uint8* Data) {
    float Values[3] = {0, 0, 0};
    switch (Format) {
    case ACF_None:
    case ACF_Float96NoW: {
        // A per-track mask of 0 is a full key.
        const int FullMask = Mask == 0 ? 7 : Mask;
        for (int Axis = 0; Axis < 3; ++Axis) {
            if ((FullMask & (1 << Axis)) == 0) continue;
            Values[Axis] = ReadAt<float>(Data);
            Data += 4;
        }
        break;
    }
    case ACF_Fixed48NoW:
        for (int Axis = 0; Axis < 3; ++Axis) {
            if ((Mask & (1 << Axis)) == 0) continue;
            Values[Axis] = static_cast<float>(static_cast<int>(ReadAt<uint16>(Data)) - 255);
            Data += 2;
        }
        break;
    case ACF_IntervalFixed32NoW: {
        const uint32 Packed = ReadAt<uint32>(Data);
        Values[0] = ((static_cast<int>(Packed & 0x3ffu) - 511) / 511.0f) * Range[1] + Range[0];
        Values[1] = ((static_cast<int>((Packed >> 10) & 0x7ffu) - 1023) / 1023.0f) * Range[3] + Range[2];
        Values[2] = ((static_cast<int>(Packed >> 21) - 1023) / 1023.0f) * Range[5] + Range[4];
        break;
    }
    default: break;
    }
    return FVector3f(Values[0], Values[1], Values[2]);
}

// FQuat::FastLerp, then normalized.
FQuat FastLerpNormalized(const FQuat& A, const FQuat& B, float Alpha) {
    const double Bias = (A | B) >= 0 ? 1.0 : -1.0;
    FQuat Result(B.X * Alpha + A.X * Bias * (1.0f - Alpha), B.Y * Alpha + A.Y * Bias * (1.0f - Alpha), B.Z * Alpha + A.Z * Bias * (1.0f - Alpha),
                 B.W * Alpha + A.W * Bias * (1.0f - Alpha));
    Result.Normalize();
    return Result;
}

enum class EComponent { Translation, Rotation, Scale };

// One channel of one track at the context's time: the format, the key data and the two keys with their weight.
struct FChannelSample {
    int32 Format = ACF_None;
    int32 Mask = 7;
    float Range[6] = {0, 0, 0, 0, 0, 0};
    const uint8* Key0 = nullptr;
    const uint8* Key1 = nullptr;
    float Alpha = 0;
};

// Finds the keys of a channel (UE's GetBoneAtomRotation/Translation/Scale of each codec). False for an identity
// channel.
bool SampleChannel(const FUECompressedAnimData& AnimData, FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, EComponent Component,
                   FChannelSample& Out) {
    const uint8* Stream = AnimData.CompressedByteStream.data();
    const float RelativePos = static_cast<float>(DecompContext.GetRelativePosition());
    int32 Index0 = 0, Index1 = 0;
    if (AnimData.KeyEncodingFormat == AKF_PerTrackCompression) {
        const int32 Offset = Component == EComponent::Scale ? AnimData.CompressedScaleOffsets.GetOffsetData(TrackIndex, 0)
                                                            : AnimData.CompressedTrackOffsets[TrackIndex * 2 + (Component == EComponent::Rotation ? 1 : 0)];
        if (Offset == INDEX_NONE) return false;
        const uint32 Header = ReadAt<uint32>(Stream + Offset);
        const int32 KeyFormat = static_cast<int32>(Header >> 28);
        const int32 FormatFlags = static_cast<int32>((Header >> 24) & 0xf);
        const int32 NumKeys = static_cast<int32>(Header & 0xffffff);
        if (KeyFormat > ACF_Identity) return false;
        const int32 ComponentCount = PerTrackComponentCount[KeyFormat][FormatFlags & 7];
        const bool bInterval = KeyFormat == ACF_IntervalFixed32NoW;
        const int32 FixedBytes = bInterval ? ComponentCount * 4 : 0;
        const int32 BytesPerKey = bInterval ? 4 : ComponentCount * PerTrackComponentSize[KeyFormat];
        const uint8* TrackData = Stream + Offset + 4;
        Out.Format = KeyFormat;
        Out.Mask = FormatFlags & 7;
        if (bInterval) {
            const uint8* Bounds = TrackData;
            for (int Axis = 0; Axis < 3; ++Axis) {
                if ((Out.Mask & (1 << Axis)) == 0) continue;
                Out.Range[Axis * 2] = ReadAt<float>(Bounds);
                Out.Range[Axis * 2 + 1] = ReadAt<float>(Bounds + 4);
                Bounds += 8;
            }
        }
        if (NumKeys > 1) {
            if ((FormatFlags & 0x8) == 0) {
                Out.Alpha = AnimEncoding::TimeToIndex(static_cast<float>(DecompContext.GetPlayableLength()), RelativePos, NumKeys,
                                                      DecompContext.Interpolation, Index0, Index1);
            } else {
                const uint8* FrameTable = Align4(TrackData + FixedBytes + BytesPerKey * NumKeys);
                Out.Alpha = AnimEncoding::TimeToIndex(DecompContext.Interpolation, AnimData.CompressedNumberOfKeys, FrameTable, RelativePos, NumKeys,
                                                      Index0, Index1);
            }
        }
        Out.Key0 = TrackData + FixedBytes + Index0 * BytesPerKey;
        Out.Key1 = TrackData + FixedBytes + Index1 * BytesPerKey;
        return true;
    }

    // Constant and variable key lerp: offset and key count per component.
    int32 Offset = 0, NumKeys = 0;
    if (Component == EComponent::Scale) {
        Offset = AnimData.CompressedScaleOffsets.GetOffsetData(TrackIndex, 0);
        NumKeys = AnimData.CompressedScaleOffsets.GetOffsetData(TrackIndex, 1);
    } else {
        const int32 Base = TrackIndex * 4 + (Component == EComponent::Rotation ? 2 : 0);
        Offset = AnimData.CompressedTrackOffsets[Base];
        NumKeys = AnimData.CompressedTrackOffsets[Base + 1];
    }
    const int32 StoredFormat = Component == EComponent::Rotation ? AnimData.RotationCompressionFormat
                                                                 : Component == EComponent::Scale ? AnimData.ScaleCompressionFormat
                                                                                                  : AnimData.TranslationCompressionFormat;
    // A single key is stored uncompressed.
    const int32 Format = NumKeys == 1 ? (Component == EComponent::Rotation ? ACF_Float96NoW : ACF_None) : StoredFormat;
    if (Format > ACF_Identity) return false;
    const int32 Stride = Component == EComponent::Rotation ? CompressedRotationStrides[Format] : CompressedTranslationStrides[Format];
    const uint8* TrackData = Stream + Offset;
    Out.Format = Format;
    Out.Mask = 7;
    if (Format == ACF_IntervalFixed32NoW) {
        // The minimum of each axis, then the extent of each axis.
        for (int Axis = 0; Axis < 3; ++Axis) {
            Out.Range[Axis * 2] = ReadAt<float>(TrackData + Axis * 4);
            Out.Range[Axis * 2 + 1] = ReadAt<float>(TrackData + 12 + Axis * 4);
        }
        TrackData += 24;
    }
    if (NumKeys > 1) {
        if (AnimData.KeyEncodingFormat == AKF_VariableKeyLerp) {
            const uint8* FrameTable = Align4(TrackData + Stride * NumKeys);
            Out.Alpha = AnimEncoding::TimeToIndex(DecompContext.Interpolation, AnimData.CompressedNumberOfKeys, FrameTable, RelativePos, NumKeys, Index0, Index1);
        } else {
            Out.Alpha = AnimEncoding::TimeToIndex(static_cast<float>(DecompContext.GetPlayableLength()), RelativePos, NumKeys, DecompContext.Interpolation,
                                                  Index0, Index1);
        }
    }
    Out.Key0 = TrackData + Index0 * Stride;
    Out.Key1 = TrackData + Index1 * Stride;
    return true;
}

const FUECompressedAnimData& AnimDataOf(const FAnimSequenceDecompressionContext& DecompContext) {
    return static_cast<const FUECompressedAnimData&>(*DecompContext.CompressedAnimData.CompressedDataStructure);
}

}

namespace AnimEncoding {

float TimeToIndex(float SequenceLength, float RelativePos, int32 NumKeys, EAnimInterpolationType Interpolation, int32& PosIndex0Out,
                  int32& PosIndex1Out) {
    PosIndex0Out = PosIndex1Out = 0;
    if (NumKeys < 2 || RelativePos <= 0.f) return 0.0f;
    const int32 LastKey = NumKeys - 1;
    if (RelativePos >= 1.0f) {
        PosIndex0Out = PosIndex1Out = LastKey;
        return 0.0f;
    }
    const float KeyPos = RelativePos * static_cast<float>(LastKey);
    const float KeyPosFloor = std::floor(KeyPos);
    PosIndex0Out = FMath::Min(static_cast<int32>(KeyPosFloor), LastKey);
    PosIndex1Out = FMath::Min(PosIndex0Out + 1, LastKey);
    return Interpolation == EAnimInterpolationType::Step ? 0.0f : KeyPos - KeyPosFloor;
}

namespace {
template <class TableType>
int32 FindLowKeyIndex(const TableType* FrameTable, int32 NumKeys, int32 SearchFrame, int32 KeyEstimate) {
    const int32 LastKeyIndex = NumKeys - 1;
    if (ReadAt<TableType>(reinterpret_cast<const uint8*>(FrameTable + KeyEstimate)) <= SearchFrame) {
        for (int32 i = KeyEstimate + 1; i <= LastKeyIndex; ++i)
            if (ReadAt<TableType>(reinterpret_cast<const uint8*>(FrameTable + i)) > SearchFrame) return i - 1;
        return LastKeyIndex;
    }
    for (int32 i = KeyEstimate - 1; i > 0; --i)
        if (ReadAt<TableType>(reinterpret_cast<const uint8*>(FrameTable + i)) <= SearchFrame) return i;
    return 0;
}
}

float TimeToIndex(EAnimInterpolationType Interpolation, int32 NumberOfFrames, const uint8* FrameTable, float RelativePos, int32 NumKeys,
                  int32& PosIndex0Out, int32& PosIndex1Out) {
    const int32 LastKey = NumKeys - 1;
    const int32 TotalFrames = NumberOfFrames - 1;
    if (NumKeys < 2 || RelativePos <= 0.f) {
        PosIndex0Out = PosIndex1Out = 0;
        return 0.0f;
    }
    if (RelativePos >= 1.0f) {
        PosIndex0Out = PosIndex1Out = LastKey;
        return 0.0f;
    }
    const int32 LastFrame = TotalFrames - 1;
    const float KeyPos = RelativePos * static_cast<float>(LastKey);
    const float FramePos = RelativePos * static_cast<float>(TotalFrames);
    const int32 FramePosFloor = FMath::Clamp(static_cast<int32>(FramePos), 0, LastFrame);
    const int32 KeyEstimate = FMath::Clamp(static_cast<int32>(KeyPos), 0, LastKey);
    int32 LowFrame = 0, HighFrame = 0;
    if (NumberOfFrames > 0xFF) {
        const uint16* Frames = reinterpret_cast<const uint16*>(FrameTable);
        PosIndex0Out = FindLowKeyIndex<uint16>(Frames, NumKeys, FramePosFloor, KeyEstimate);
        PosIndex1Out = FMath::Min(PosIndex0Out + 1, LastKey);
        LowFrame = ReadAt<uint16>(FrameTable + PosIndex0Out * 2);
        HighFrame = ReadAt<uint16>(FrameTable + PosIndex1Out * 2);
    } else {
        PosIndex0Out = FindLowKeyIndex<uint8>(FrameTable, NumKeys, FramePosFloor, KeyEstimate);
        PosIndex1Out = FMath::Min(PosIndex0Out + 1, LastKey);
        LowFrame = FrameTable[PosIndex0Out];
        HighFrame = FrameTable[PosIndex1Out];
    }
    const int32 Delta = FMath::Max(HighFrame - LowFrame, 1);
    return Interpolation == EAnimInterpolationType::Step ? 0.f : (FramePos - static_cast<float>(LowFrame)) / static_cast<float>(Delta);
}

void GetBoneAtomRotation(FTransform& OutAtom, FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex) {
    FChannelSample Sample;
    if (!SampleChannel(AnimDataOf(DecompContext), DecompContext, TrackIndex, EComponent::Rotation, Sample)) {
        OutAtom.SetRotation(FQuat::Identity);
        return;
    }
    FQuat R0 = DecompressRotation(Sample.Format, Sample.Mask, Sample.Range, Sample.Key0);
    if (Sample.Key0 != Sample.Key1) {
        OutAtom.SetRotation(FastLerpNormalized(R0, DecompressRotation(Sample.Format, Sample.Mask, Sample.Range, Sample.Key1), Sample.Alpha));
    } else {
        R0.Normalize();
        OutAtom.SetRotation(R0);
    }
}

void GetBoneAtomTranslation(FTransform& OutAtom, FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex) {
    FChannelSample Sample;
    if (!SampleChannel(AnimDataOf(DecompContext), DecompContext, TrackIndex, EComponent::Translation, Sample)) {
        OutAtom.SetTranslation(FVector::ZeroVector);
        return;
    }
    const FVector3f P0 = DecompressVector(Sample.Format, Sample.Mask, Sample.Range, Sample.Key0);
    const FVector3f P1 = Sample.Key0 != Sample.Key1 ? DecompressVector(Sample.Format, Sample.Mask, Sample.Range, Sample.Key1) : P0;
    OutAtom.SetTranslation(FVector(FMath::Lerp(P0, P1, Sample.Alpha)));
}

void GetBoneAtomScale(FTransform& OutAtom, FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex) {
    FChannelSample Sample;
    if (!SampleChannel(AnimDataOf(DecompContext), DecompContext, TrackIndex, EComponent::Scale, Sample)) {
        OutAtom.SetScale3D(FVector::ZeroVector);
        return;
    }
    const FVector3f S0 = DecompressVector(Sample.Format, Sample.Mask, Sample.Range, Sample.Key0);
    const FVector3f S1 = Sample.Key0 != Sample.Key1 ? DecompressVector(Sample.Format, Sample.Mask, Sample.Range, Sample.Key1) : S0;
    OutAtom.SetScale3D(FVector(FMath::Lerp(S0, S1, Sample.Alpha)));
}

}

void UAnimCompress::DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs,
                                   const BoneTrackArray& TranslationPairs, const BoneTrackArray& ScalePairs, std::span<FTransform> OutAtoms) const {
    for (const BoneTrackPair& Pair : TranslationPairs) AnimEncoding::GetBoneAtomTranslation(OutAtoms[Pair.AtomIndex], DecompContext, Pair.TrackIndex);
    for (const BoneTrackPair& Pair : RotationPairs) AnimEncoding::GetBoneAtomRotation(OutAtoms[Pair.AtomIndex], DecompContext, Pair.TrackIndex);
    if (AnimDataOf(DecompContext).CompressedScaleOffsets.IsValid())
        for (const BoneTrackPair& Pair : ScalePairs) AnimEncoding::GetBoneAtomScale(OutAtoms[Pair.AtomIndex], DecompContext, Pair.TrackIndex);
}

void UAnimCompress::DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const {
    // Identity first: the scale, and a missing channel.
    OutAtom.SetIdentity();
    AnimEncoding::GetBoneAtomTranslation(OutAtom, DecompContext, TrackIndex);
    AnimEncoding::GetBoneAtomRotation(OutAtom, DecompContext, TrackIndex);
    if (AnimDataOf(DecompContext).CompressedScaleOffsets.IsValid()) AnimEncoding::GetBoneAtomScale(OutAtom, DecompContext, TrackIndex);
}

}

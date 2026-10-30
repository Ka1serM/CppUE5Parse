#include "ue5parse/runtime/Engine/Animation/AnimCurveCompressionCodec_CompressedRichCurve.h"

#include <cstring>

#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"

// UE's AnimCurveCompressionCodec_CompressedRichCurve.cpp (UE 5.3).

namespace ue5parse::runtime {

namespace {

// FCurveDesc: the packed header of one curve.
struct FCurveDesc {
    uint8 CompressionFormat;
    uint8 KeyTimeCompressionFormat;
    uint8 PreInfinityExtrap;
    uint8 PostInfinityExtrap;
    int32 ConstantValueNumKeys;
    int32 KeyDataOffset;
};
static_assert(sizeof(FCurveDesc) == 12);

float EvaluateCurve(const FCompressedAnimSequence& AnimSeq, int32 CurveIndex, float CurrentTime) {
    const uint8* Buffer = AnimSeq.CompressedCurveByteStream.GetData();
    FCurveDesc Curve;
    std::memcpy(&Curve, Buffer + CurveIndex * sizeof(FCurveDesc), sizeof(FCurveDesc));
    return FCompressedRichCurve::StaticEval(static_cast<ERichCurveCompressionFormat>(Curve.CompressionFormat),
                                            static_cast<ERichCurveKeyTimeCompressionFormat>(Curve.KeyTimeCompressionFormat),
                                            static_cast<ERichCurveExtrapolation>(Curve.PreInfinityExtrap),
                                            static_cast<ERichCurveExtrapolation>(Curve.PostInfinityExtrap), Curve.ConstantValueNumKeys,
                                            Buffer + Curve.KeyDataOffset, CurrentTime);
}

bool HasCurveData(const FCompressedAnimSequence& AnimSeq) {
    return AnimSeq.CompressedCurveByteStream.Num() >= AnimSeq.IndexedCurveNames.Num() * static_cast<int32>(sizeof(FCurveDesc));
}

}

void UAnimCurveCompressionCodec_CompressedRichCurve::DecompressCurves(const FCompressedAnimSequence& AnimSeq, FBlendedCurve& Curves,
                                                                      float CurrentTime) const {
    if (!HasCurveData(AnimSeq)) return;
    for (const FAnimCompressedCurveIndexedName& Indexed : AnimSeq.IndexedCurveNames) {
        const int32 CurveIndex = Indexed.CurveIndex;
        Curves.Set(AnimSeq.IndexedCurveNames[CurveIndex].CurveName, EvaluateCurve(AnimSeq, CurveIndex, CurrentTime));
    }
}

float UAnimCurveCompressionCodec_CompressedRichCurve::DecompressCurve(const FCompressedAnimSequence& AnimSeq, FName CurveName,
                                                                     float CurrentTime) const {
    if (!HasCurveData(AnimSeq)) return 0.0f;
    for (int32 CurveIndex = 0; CurveIndex < AnimSeq.IndexedCurveNames.Num(); ++CurveIndex)
        if (AnimSeq.IndexedCurveNames[CurveIndex].CurveName == CurveName) return EvaluateCurve(AnimSeq, CurveIndex, CurrentTime);
    return 0.0f;
}

}

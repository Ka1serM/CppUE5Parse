#include "Engine/Plugins/Animation/ACLPlugin/Source/ACLPlugin/AnimCurveCompressionCodec_ACL.h"

#include <acl/decompression/decompress.h>

#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimCurveTypes.h"

namespace ue5parse::runtime {

namespace {

struct UECurveDecompressionSettings final : public acl::decompression_settings {
    static constexpr bool is_track_type_supported(acl::track_type8 type) { return type == acl::track_type8::float1f; }
    static constexpr acl::compressed_tracks_version16 version_supported() { return acl::compressed_tracks_version16::any; }
};

struct UECurveWriter final : public acl::track_writer {
    TArray<float>& Buffer;

    explicit UECurveWriter(TArray<float>& InBuffer) : Buffer(InBuffer) {}
    void RTM_SIMD_CALL write_float1(uint32_t TrackIndex, rtm::scalarf_arg0 Value) { Buffer[static_cast<int32>(TrackIndex)] = rtm::scalar_cast(Value); }
};

struct UEScalarCurveWriter final : public acl::track_writer {
    float SampleValue = 0.0f;

    void RTM_SIMD_CALL write_float1(uint32_t, rtm::scalarf_arg0 Value) { SampleValue = rtm::scalar_cast(Value); }
};

bool InitializeContext(const FCompressedAnimSequence& AnimSeq, float CurrentTime, acl::decompression_context<UECurveDecompressionSettings>& Context) {
    const acl::compressed_tracks* CompressedTracks = acl::make_compressed_tracks(AnimSeq.CompressedCurveByteStream.GetData());
    if (CompressedTracks == nullptr || !Context.initialize(*CompressedTracks)) {
        ReportLoadProblem("anim.acl_curve_initialize", FString("ACL cannot decompress the curves of a sequence"));
        return false;
    }
    Context.seek(CurrentTime, acl::sample_rounding_policy::none);
    return true;
}

}

void UAnimCurveCompressionCodec_ACL::DecompressCurves(const FCompressedAnimSequence& AnimSeq, FBlendedCurve& Curves, float CurrentTime) const {
    const TArray<FAnimCompressedCurveIndexedName>& IndexedCurveNames = AnimSeq.IndexedCurveNames;
    const int32 NumCurves = IndexedCurveNames.Num();
    if (NumCurves == 0)
        return;
    acl::decompression_context<UECurveDecompressionSettings> Context;
    if (!InitializeContext(AnimSeq, CurrentTime, Context))
        return;
    TArray<float> DecompressionBuffer;
    DecompressionBuffer.SetNum(NumCurves);
    UECurveWriter TrackWriter(DecompressionBuffer);
    Context.decompress_tracks(TrackWriter);
    for (int32 CurveIndex = 0; CurveIndex < NumCurves; ++CurveIndex) {
        const int32 SortedIndex = IndexedCurveNames[CurveIndex].CurveIndex;
        Curves.Set(IndexedCurveNames[SortedIndex].CurveName, DecompressionBuffer[SortedIndex]);
    }
}

float UAnimCurveCompressionCodec_ACL::DecompressCurve(const FCompressedAnimSequence& AnimSeq, FName CurveName, float CurrentTime) const {
    const TArray<FAnimCompressedCurveIndexedName>& IndexedCurveNames = AnimSeq.IndexedCurveNames;
    int32 TrackIndex = INDEX_NONE;
    for (int32 CurveIndex = 0; CurveIndex < IndexedCurveNames.Num() && TrackIndex == INDEX_NONE; ++CurveIndex)
        if (IndexedCurveNames[CurveIndex].CurveName == CurveName)
            TrackIndex = CurveIndex;
    if (TrackIndex == INDEX_NONE)
        return 0.0f;
    acl::decompression_context<UECurveDecompressionSettings> Context;
    if (!InitializeContext(AnimSeq, CurrentTime, Context))
        return 0.0f;
    UEScalarCurveWriter TrackWriter;
    Context.decompress_track(static_cast<uint32>(TrackIndex), TrackWriter);
    return TrackWriter.SampleValue;
}

}

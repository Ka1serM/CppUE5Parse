#include "ue5parse/runtime/ACLPlugin/AnimBoneCompressionCodec_ACL.h"
#include "ue5parse/runtime/ACLPlugin/AnimBoneCompressionCodec_ACLCustom.h"
#include "ue5parse/runtime/ACLPlugin/AnimBoneCompressionCodec_ACLDatabase.h"
#include "ue5parse/runtime/ACLPlugin/AnimBoneCompressionCodec_ACLSafe.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequenceDecompressionContext.h"

#include <acl/decompression/decompress.h>

// The decompression of the ACL plugin (ACLImpl.h, ACLDecompressionImpl.h and the codec classes, UE 5.3).

namespace ue5parse::runtime {

namespace {

struct UE4DefaultDecompressionSettings : public acl::default_transform_decompression_settings {
    static constexpr acl::compressed_tracks_version16 version_supported() { return acl::compressed_tracks_version16::latest; }
};
using UE4CustomDecompressionSettings = acl::debug_transform_decompression_settings;
struct UE4SafeDecompressionSettings final : public UE4DefaultDecompressionSettings {
    static constexpr bool is_rotation_format_supported(acl::rotation_format8 format) { return format == acl::rotation_format8::quatf_full; }
    static constexpr acl::rotation_format8 get_rotation_format(acl::rotation_format8) { return acl::rotation_format8::quatf_full; }
};

FQuat QuatCast(rtm::quatf_arg0 Input) { return FQuat(rtm::quat_get_x(Input), rtm::quat_get_y(Input), rtm::quat_get_z(Input), rtm::quat_get_w(Input)); }
FVector VectorCast(rtm::vector4f_arg0 Input) {
    return FVector(rtm::vector_get_x(Input), rtm::vector_get_y(Input), rtm::vector_get_z(Input));
}

// The atoms that each track writes; 0xFFFF skips a channel.
struct FAtomIndices {
    uint16 Rotation;
    uint16 Translation;
    uint16 Scale;
};

struct FUE4OutputWriter final : public acl::track_writer {
    FTransform* Atoms;
    const FAtomIndices* TrackToAtomsMap;

    FUE4OutputWriter(std::span<FTransform> InAtoms, const FAtomIndices* InTrackToAtomsMap) : Atoms(InAtoms.data()), TrackToAtomsMap(InTrackToAtomsMap) {}
    bool skip_track_rotation(uint32_t BoneIndex) const { return TrackToAtomsMap[BoneIndex].Rotation == 0xFFFF; }
    bool skip_track_translation(uint32_t BoneIndex) const { return TrackToAtomsMap[BoneIndex].Translation == 0xFFFF; }
    bool skip_track_scale(uint32_t BoneIndex) const { return TrackToAtomsMap[BoneIndex].Scale == 0xFFFF; }
    void RTM_SIMD_CALL write_rotation(uint32_t BoneIndex, rtm::quatf_arg0 Rotation) { Atoms[TrackToAtomsMap[BoneIndex].Rotation].SetRotation(QuatCast(Rotation)); }
    void RTM_SIMD_CALL write_translation(uint32_t BoneIndex, rtm::vector4f_arg0 Translation) {
        Atoms[TrackToAtomsMap[BoneIndex].Translation].SetTranslation(VectorCast(Translation));
    }
    void RTM_SIMD_CALL write_scale(uint32_t BoneIndex, rtm::vector4f_arg0 Scale) { Atoms[TrackToAtomsMap[BoneIndex].Scale].SetScale3D(VectorCast(Scale)); }
};

struct UE4OutputTrackWriter final : public acl::track_writer {
    FTransform* Atom;

    explicit UE4OutputTrackWriter(FTransform& InAtom) : Atom(&InAtom) {}
    void RTM_SIMD_CALL write_rotation(uint32_t, rtm::quatf_arg0 Rotation) { Atom->SetRotation(QuatCast(Rotation)); }
    void RTM_SIMD_CALL write_translation(uint32_t, rtm::vector4f_arg0 Translation) { Atom->SetTranslation(VectorCast(Translation)); }
    void RTM_SIMD_CALL write_scale(uint32_t, rtm::vector4f_arg0 Scale) { Atom->SetScale3D(VectorCast(Scale)); }
};

const acl::compressed_tracks* TracksOf(const FAnimSequenceDecompressionContext& DecompContext) {
    const auto& AnimData = static_cast<const FACLCompressedAnimData&>(*DecompContext.CompressedAnimData.CompressedDataStructure);
    return static_cast<const acl::compressed_tracks*>(AnimData.GetCompressedTracks());
}

template <class SettingsType>
void DecompressPoseImpl(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs, const BoneTrackArray& TranslationPairs,
                        const BoneTrackArray& ScalePairs, std::span<FTransform> OutAtoms) {
    const acl::compressed_tracks* CompressedClipData = TracksOf(DecompContext);
    if (CompressedClipData == nullptr) return;
    acl::decompression_context<SettingsType> ACLContext;
    if (!ACLContext.initialize(*CompressedClipData)) return;
    ACLContext.seek(static_cast<float>(DecompContext.GetInterpolatedEvaluationTime()), acl::sample_rounding_policy::none);
    const uint32 ACLBoneCount = CompressedClipData->get_num_tracks();
    thread_local TArray<FAtomIndices> TrackToAtomsMap;
    TrackToAtomsMap.SetNum(static_cast<int32>(ACLBoneCount));
    for (FAtomIndices& Indices : TrackToAtomsMap) Indices = {0xFFFF, 0xFFFF, 0xFFFF};
    const auto Valid = [&](const BoneTrackPair& Pair) { return Pair.TrackIndex >= 0 && static_cast<uint32>(Pair.TrackIndex) < ACLBoneCount; };
    for (const BoneTrackPair& Pair : RotationPairs)
        if (Valid(Pair)) TrackToAtomsMap[Pair.TrackIndex].Rotation = static_cast<uint16>(Pair.AtomIndex);
    for (const BoneTrackPair& Pair : TranslationPairs)
        if (Valid(Pair)) TrackToAtomsMap[Pair.TrackIndex].Translation = static_cast<uint16>(Pair.AtomIndex);
    if (acl::acl_impl::get_tracks_header(*CompressedClipData).get_has_scale())
        for (const BoneTrackPair& Pair : ScalePairs)
            if (Valid(Pair)) TrackToAtomsMap[Pair.TrackIndex].Scale = static_cast<uint16>(Pair.AtomIndex);
    FUE4OutputWriter PoseWriter(OutAtoms, TrackToAtomsMap.GetData());
    ACLContext.decompress_tracks(PoseWriter);
}

template <class SettingsType>
void DecompressBoneImpl(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) {
    const acl::compressed_tracks* CompressedClipData = TracksOf(DecompContext);
    if (CompressedClipData == nullptr || TrackIndex < 0 || static_cast<uint32>(TrackIndex) >= CompressedClipData->get_num_tracks()) return;
    acl::decompression_context<SettingsType> ACLContext;
    if (!ACLContext.initialize(*CompressedClipData)) return;
    ACLContext.seek(static_cast<float>(DecompContext.GetInterpolatedEvaluationTime()), acl::sample_rounding_policy::none);
    UE4OutputTrackWriter Writer(OutAtom);
    ACLContext.decompress_track(static_cast<uint32>(TrackIndex), Writer);
}

}

bool FACLCompressedAnimData::IsValid() const {
    if (CompressedByteStream.empty()) return false;
    const auto* Tracks = reinterpret_cast<const acl::compressed_tracks*>(CompressedByteStream.data());
    return Tracks->is_valid(false).empty();
}

UAnimBoneCompressionCodec* UAnimBoneCompressionCodec_ACL::GetCodec(const FString& DDCHandle) {
    if (UAnimBoneCompressionCodec* Found = Super::GetCodec(DDCHandle)) return Found;
    // SafetyFallbackCodec is editor data in UE 5.3; a cook that keeps it is searched too.
    UAnimBoneCompressionCodec* Fallback = nullptr;
    if (GetPropertyValue(this, FName("SafetyFallbackCodec"), Fallback) && Fallback != nullptr) return Fallback->GetCodec(DDCHandle);
    return nullptr;
}

void UAnimBoneCompressionCodec_ACL::DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs,
                                                   const BoneTrackArray& TranslationPairs, const BoneTrackArray& ScalePairs,
                                                   std::span<FTransform> OutAtoms) const {
    DecompressPoseImpl<UE4DefaultDecompressionSettings>(DecompContext, RotationPairs, TranslationPairs, ScalePairs, OutAtoms);
}
void UAnimBoneCompressionCodec_ACL::DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const {
    DecompressBoneImpl<UE4DefaultDecompressionSettings>(DecompContext, TrackIndex, OutAtom);
}

void UAnimBoneCompressionCodec_ACLSafe::DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs,
                                                       const BoneTrackArray& TranslationPairs, const BoneTrackArray& ScalePairs,
                                                       std::span<FTransform> OutAtoms) const {
    DecompressPoseImpl<UE4SafeDecompressionSettings>(DecompContext, RotationPairs, TranslationPairs, ScalePairs, OutAtoms);
}
void UAnimBoneCompressionCodec_ACLSafe::DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const {
    DecompressBoneImpl<UE4SafeDecompressionSettings>(DecompContext, TrackIndex, OutAtom);
}

void UAnimBoneCompressionCodec_ACLCustom::DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs,
                                                         const BoneTrackArray& TranslationPairs, const BoneTrackArray& ScalePairs,
                                                         std::span<FTransform> OutAtoms) const {
    DecompressPoseImpl<UE4CustomDecompressionSettings>(DecompContext, RotationPairs, TranslationPairs, ScalePairs, OutAtoms);
}
void UAnimBoneCompressionCodec_ACLCustom::DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const {
    DecompressBoneImpl<UE4CustomDecompressionSettings>(DecompContext, TrackIndex, OutAtom);
}

void UAnimBoneCompressionCodec_ACLDatabase::DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs,
                                                           const BoneTrackArray& TranslationPairs, const BoneTrackArray& ScalePairs,
                                                           std::span<FTransform> OutAtoms) const {
    DecompressPoseImpl<UE4DefaultDecompressionSettings>(DecompContext, RotationPairs, TranslationPairs, ScalePairs, OutAtoms);
}
void UAnimBoneCompressionCodec_ACLDatabase::DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex,
                                                           FTransform& OutAtom) const {
    DecompressBoneImpl<UE4DefaultDecompressionSettings>(DecompContext, TrackIndex, OutAtom);
}

}

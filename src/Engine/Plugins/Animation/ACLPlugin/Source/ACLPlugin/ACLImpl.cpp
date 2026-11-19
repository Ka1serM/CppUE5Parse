#include <acl/decompression/decompress.h>

#include "Engine/Plugins/Animation/ACLPlugin/Source/ACLPlugin/AnimBoneCompressionCodec_ACL.h"
#include "Engine/Plugins/Animation/ACLPlugin/Source/ACLPlugin/AnimBoneCompressionCodec_ACLCustom.h"
#include "Engine/Plugins/Animation/ACLPlugin/Source/ACLPlugin/AnimBoneCompressionCodec_ACLDatabase.h"
#include "Engine/Plugins/Animation/ACLPlugin/Source/ACLPlugin/AnimBoneCompressionCodec_ACLSafe.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimSequenceDecompressionContext.h"

namespace ue5parse::runtime {

namespace {

struct UE4DefaultDecompressionSettings : public acl::default_transform_decompression_settings {
    static constexpr acl::compressed_tracks_version16 version_supported() { return acl::compressed_tracks_version16::any; }
};
using UE4CustomDecompressionSettings = acl::debug_transform_decompression_settings;
struct UE4SafeDecompressionSettings final : public UE4DefaultDecompressionSettings {
    static constexpr bool is_rotation_format_supported(acl::rotation_format8 format) { return format == acl::rotation_format8::quatf_full; }
    static constexpr acl::rotation_format8 get_rotation_format(acl::rotation_format8) { return acl::rotation_format8::quatf_full; }
};

FQuat QuatCast(rtm::quatf_arg0 Input) {
    return FQuat(rtm::quat_get_x(Input), rtm::quat_get_y(Input), rtm::quat_get_z(Input), rtm::quat_get_w(Input));
}
FVector VectorCast(rtm::vector4f_arg0 Input) {
    return FVector(rtm::vector_get_x(Input), rtm::vector_get_y(Input), rtm::vector_get_z(Input));
}

rtm::vector4f DefaultScaleOf(const FAnimSequenceDecompressionContext& DecompContext) {
    return rtm::vector_set(DecompContext.IsAdditiveAnimation() ? 0.0F : 1.0F);
}

struct FAtomIndices {
    uint16 Rotation;
    uint16 Translation;
    uint16 Scale;
};

class FBindPose {
public:
    FBindPose(const FAnimSequenceDecompressionContext& DecompContext)
        : RefPoses(DecompContext.GetRefLocalPoses().GetData()), TrackToBoneMapping(DecompContext.GetTrackToSkeletonMap().GetData()) {}

    const FTransform& Of(uint32_t TrackIndex) const { return RefPoses[TrackToBoneMapping[TrackIndex].BoneTreeIndex]; }

private:
    const FTransform* RefPoses;
    const FTrackToSkeletonMap* TrackToBoneMapping;
};

rtm::quatf QuatCast(const FQuat& Input) {
    return rtm::quat_set(static_cast<float>(Input.X), static_cast<float>(Input.Y), static_cast<float>(Input.Z), static_cast<float>(Input.W));
}
rtm::vector4f VectorCast(const FVector& Input) {
    return rtm::vector_set(static_cast<float>(Input.X), static_cast<float>(Input.Y), static_cast<float>(Input.Z));
}

template <bool bUseBindPose>
struct FBindPoseDefaults : public acl::track_writer {
    FBindPose BindPose;
    rtm::vector4f DefaultScale;

    FBindPoseDefaults(const FAnimSequenceDecompressionContext& DecompContext) : BindPose(DecompContext), DefaultScale(DefaultScaleOf(DecompContext)) {}
    static constexpr acl::default_sub_track_mode get_default_rotation_mode() {
        return bUseBindPose ? acl::default_sub_track_mode::variable : acl::default_sub_track_mode::constant;
    }
    static constexpr acl::default_sub_track_mode get_default_translation_mode() {
        return bUseBindPose ? acl::default_sub_track_mode::variable : acl::default_sub_track_mode::constant;
    }
    static constexpr acl::default_sub_track_mode get_default_scale_mode() {
        return bUseBindPose ? acl::default_sub_track_mode::variable : acl::default_sub_track_mode::constant;
    }
    rtm::vector4f RTM_SIMD_CALL get_constant_default_scale() const { return DefaultScale; }
    rtm::quatf RTM_SIMD_CALL get_variable_default_rotation(uint32_t TrackIndex) const { return QuatCast(BindPose.Of(TrackIndex).GetRotation()); }
    rtm::vector4f RTM_SIMD_CALL get_variable_default_translation(uint32_t TrackIndex) const {
        return VectorCast(BindPose.Of(TrackIndex).GetTranslation());
    }
    rtm::vector4f RTM_SIMD_CALL get_variable_default_scale(uint32_t TrackIndex) const { return VectorCast(BindPose.Of(TrackIndex).GetScale3D()); }
};

template <bool bUseBindPose>
struct FUE4OutputWriter final : public FBindPoseDefaults<bUseBindPose> {
    FTransform* Atoms;
    const FAtomIndices* TrackToAtomsMap;

    FUE4OutputWriter(const FAnimSequenceDecompressionContext& DecompContext, std::span<FTransform> InAtoms, const FAtomIndices* InTrackToAtomsMap)
        : FBindPoseDefaults<bUseBindPose>(DecompContext), Atoms(InAtoms.data()), TrackToAtomsMap(InTrackToAtomsMap) {}
    bool skip_track_rotation(uint32_t BoneIndex) const { return TrackToAtomsMap[BoneIndex].Rotation == 0xFFFF; }
    bool skip_track_translation(uint32_t BoneIndex) const { return TrackToAtomsMap[BoneIndex].Translation == 0xFFFF; }
    bool skip_track_scale(uint32_t BoneIndex) const { return TrackToAtomsMap[BoneIndex].Scale == 0xFFFF; }
    void RTM_SIMD_CALL write_rotation(uint32_t BoneIndex, rtm::quatf_arg0 Rotation) {
        Atoms[TrackToAtomsMap[BoneIndex].Rotation].SetRotation(QuatCast(Rotation));
    }
    void RTM_SIMD_CALL write_translation(uint32_t BoneIndex, rtm::vector4f_arg0 Translation) {
        Atoms[TrackToAtomsMap[BoneIndex].Translation].SetTranslation(VectorCast(Translation));
    }
    void RTM_SIMD_CALL write_scale(uint32_t BoneIndex, rtm::vector4f_arg0 Scale) { Atoms[TrackToAtomsMap[BoneIndex].Scale].SetScale3D(VectorCast(Scale)); }
};

template <bool bUseBindPose>
struct UE4OutputTrackWriter final : public FBindPoseDefaults<bUseBindPose> {
    FTransform* Atom;

    UE4OutputTrackWriter(const FAnimSequenceDecompressionContext& DecompContext, FTransform& InAtom)
        : FBindPoseDefaults<bUseBindPose>(DecompContext), Atom(&InAtom) {}
    void RTM_SIMD_CALL write_rotation(uint32_t, rtm::quatf_arg0 Rotation) { Atom->SetRotation(QuatCast(Rotation)); }
    void RTM_SIMD_CALL write_translation(uint32_t, rtm::vector4f_arg0 Translation) { Atom->SetTranslation(VectorCast(Translation)); }
    void RTM_SIMD_CALL write_scale(uint32_t, rtm::vector4f_arg0 Scale) { Atom->SetScale3D(VectorCast(Scale)); }
};

bool UsesBindPoseStripping(const acl::compressed_tracks& Tracks) {
    return Tracks.get_version() >= acl::compressed_tracks_version16::v02_01_99 && Tracks.get_default_scale() != 0;
}

const acl::compressed_tracks* TracksOf(const FAnimSequenceDecompressionContext& DecompContext) {
    const auto& AnimData = static_cast<const FACLCompressedAnimData&>(*DecompContext.CompressedAnimData.CompressedDataStructure);
    return static_cast<const acl::compressed_tracks*>(AnimData.GetCompressedTracks());
}

template <class SettingsType>
void DecompressPoseImpl(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs, const BoneTrackArray& TranslationPairs,
                        const BoneTrackArray& ScalePairs, std::span<FTransform> OutAtoms) {
    const acl::compressed_tracks* CompressedClipData = TracksOf(DecompContext);
    if (CompressedClipData == nullptr)
        return;
    acl::decompression_context<SettingsType> ACLContext;
    if (!ACLContext.initialize(*CompressedClipData)) {
        ReportLoadProblem("anim.acl_initialize", FString("ACL cannot decompress the tracks of a sequence"));
        return;
    }
    ACLContext.seek(static_cast<float>(DecompContext.GetInterpolatedEvaluationTime()), acl::sample_rounding_policy::none);
    const uint32 ACLBoneCount = CompressedClipData->get_num_tracks();
    thread_local TArray<FAtomIndices> TrackToAtomsMap;
    TrackToAtomsMap.SetNum(static_cast<int32>(ACLBoneCount));
    for (FAtomIndices& Indices : TrackToAtomsMap)
        Indices = {0xFFFF, 0xFFFF, 0xFFFF};
    const auto Valid = [&](const BoneTrackPair& Pair) { return Pair.TrackIndex >= 0 && static_cast<uint32>(Pair.TrackIndex) < ACLBoneCount; };
    for (const BoneTrackPair& Pair : RotationPairs)
        if (Valid(Pair))
            TrackToAtomsMap[Pair.TrackIndex].Rotation = static_cast<uint16>(Pair.AtomIndex);
    for (const BoneTrackPair& Pair : TranslationPairs)
        if (Valid(Pair))
            TrackToAtomsMap[Pair.TrackIndex].Translation = static_cast<uint16>(Pair.AtomIndex);
    if (acl::acl_impl::get_tracks_header(*CompressedClipData).get_has_scale())
        for (const BoneTrackPair& Pair : ScalePairs)
            if (Valid(Pair))
                TrackToAtomsMap[Pair.TrackIndex].Scale = static_cast<uint16>(Pair.AtomIndex);
    if (UsesBindPoseStripping(*CompressedClipData)) {
        FUE4OutputWriter<true> PoseWriter(DecompContext, OutAtoms, TrackToAtomsMap.GetData());
        ACLContext.decompress_tracks(PoseWriter);
        return;
    }
    FUE4OutputWriter<false> PoseWriter(DecompContext, OutAtoms, TrackToAtomsMap.GetData());
    ACLContext.decompress_tracks(PoseWriter);
}

template <class SettingsType>
void DecompressBoneImpl(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) {
    const acl::compressed_tracks* CompressedClipData = TracksOf(DecompContext);
    if (CompressedClipData == nullptr || TrackIndex < 0 || static_cast<uint32>(TrackIndex) >= CompressedClipData->get_num_tracks())
        return;
    acl::decompression_context<SettingsType> ACLContext;
    if (!ACLContext.initialize(*CompressedClipData)) {
        ReportLoadProblem("anim.acl_initialize", FString("ACL cannot decompress the tracks of a sequence"));
        return;
    }
    ACLContext.seek(static_cast<float>(DecompContext.GetInterpolatedEvaluationTime()), acl::sample_rounding_policy::none);
    if (UsesBindPoseStripping(*CompressedClipData)) {
        UE4OutputTrackWriter<true> Writer(DecompContext, OutAtom);
        ACLContext.decompress_track(static_cast<uint32>(TrackIndex), Writer);
        return;
    }
    UE4OutputTrackWriter<false> Writer(DecompContext, OutAtom);
    ACLContext.decompress_track(static_cast<uint32>(TrackIndex), Writer);
}

}

bool FACLCompressedAnimData::IsValid() const {
    if (CompressedByteStream.empty())
        return false;
    const auto* Tracks = reinterpret_cast<const acl::compressed_tracks*>(CompressedByteStream.data());
    return Tracks->is_valid(false).empty();
}

UAnimBoneCompressionCodec* UAnimBoneCompressionCodec_ACL::GetCodec(const FString& DDCHandle) {
    if (UAnimBoneCompressionCodec* Found = Super::GetCodec(DDCHandle))
        return Found;
    UAnimBoneCompressionCodec* Fallback = nullptr;
    if (GetPropertyValue(this, FName("SafetyFallbackCodec"), Fallback) && Fallback != nullptr)
        return Fallback->GetCodec(DDCHandle);
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
void UAnimBoneCompressionCodec_ACLDatabase::DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const {
    DecompressBoneImpl<UE4DefaultDecompressionSettings>(DecompContext, TrackIndex, OutAtom);
}

}

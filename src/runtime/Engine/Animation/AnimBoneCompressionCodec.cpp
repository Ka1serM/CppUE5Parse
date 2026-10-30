#include "ue5parse/runtime/Engine/Animation/AnimBoneCompressionCodec.h"

#include "ue5parse/runtime/Engine/Animation/AnimBoneCompressionSettings.h"

namespace ue5parse::runtime {

FString UAnimBoneCompressionCodec::GetCodecDDCHandle() const {
    // The codec's name, then its outers below the settings object.
    FString Handle = GetName();
    for (const UObject* Outer = GetOuter(); Outer != nullptr && !Outer->IsA<UAnimBoneCompressionSettings>(); Outer = Outer->GetOuter()) {
        if (Outer->GetOuter() == nullptr) break;
        Handle += ".";
        Handle += Outer->GetName();
    }
    return Handle;
}

UAnimBoneCompressionCodec* UAnimBoneCompressionCodec::GetCodec(const FString& DDCHandle) {
    return GetCodecDDCHandle().Equals(DDCHandle) ? this : nullptr;
}

UAnimBoneCompressionCodec* UAnimBoneCompressionSettings::GetCodec(const FString& DDCHandle) const {
    for (UAnimBoneCompressionCodec* Codec : Codecs)
        if (Codec != nullptr)
            if (UAnimBoneCompressionCodec* Match = Codec->GetCodec(DDCHandle)) return Match;
    return nullptr;
}

}

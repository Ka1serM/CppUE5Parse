#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimSequenceDecompressionContext.h"

// The key formats of the engine's own bone codecs (UE's AnimEncoding.h, AnimEncoding_ConstantKeyLerp.h,
// AnimEncoding_VariableKeyLerp.h, AnimEncoding_PerTrackCompression.h).

namespace ue5parse::runtime {

enum class EAnimInterpolationType : uint8;

namespace AnimEncoding {
// The two keys around a relative position and the weight of the second: evenly spread keys, or keys at the frames of a
// frame table (UE's TimeToIndex).
float TimeToIndex(float SequenceLength, float RelativePos, int32 NumKeys, EAnimInterpolationType Interpolation, int32& PosIndex0Out,
                  int32& PosIndex1Out);
float TimeToIndex(EAnimInterpolationType Interpolation, int32 NumberOfFrames, const uint8* FrameTable, float RelativePos, int32 NumKeys,
                  int32& PosIndex0Out, int32& PosIndex1Out);

void GetBoneAtomRotation(FTransform& OutAtom, FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex);
void GetBoneAtomTranslation(FTransform& OutAtom, FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex);
void GetBoneAtomScale(FTransform& OutAtom, FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex);
}

}

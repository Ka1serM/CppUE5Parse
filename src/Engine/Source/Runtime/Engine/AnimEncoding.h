#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimSequenceDecompressionContext.h"

namespace ue5parse::runtime {

enum class EAnimInterpolationType : uint8;

namespace AnimEncoding {

float TimeToIndex(float SequenceLength, float RelativePos, int32 NumKeys, EAnimInterpolationType Interpolation, int32& PosIndex0Out,
                  int32& PosIndex1Out);
float TimeToIndex(EAnimInterpolationType Interpolation, int32 NumberOfFrames, const uint8* FrameTable, float RelativePos, int32 NumKeys,
                  int32& PosIndex0Out, int32& PosIndex1Out);

void GetBoneAtomRotation(FTransform& OutAtom, FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex);
void GetBoneAtomTranslation(FTransform& OutAtom, FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex);
void GetBoneAtomScale(FTransform& OutAtom, FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex);

}

}

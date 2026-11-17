#include "Engine/Source/Runtime/Engine/Animation/AnimCompress_RemoveTrivialKeys.h"

namespace ue5parse::runtime {

UAnimCompress_RemoveTrivialKeys::UAnimCompress_RemoveTrivialKeys(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    MaxPosDiff = 0.0001f;
    MaxAngleDiff = 0.0003f;
    MaxScaleDiff = 0.000001f;
}

}

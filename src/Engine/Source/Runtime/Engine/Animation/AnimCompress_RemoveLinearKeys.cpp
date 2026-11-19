#include "Engine/Source/Runtime/Engine/Animation/AnimCompress_RemoveLinearKeys.h"

namespace ue5parse::runtime {

UAnimCompress_RemoveLinearKeys::UAnimCompress_RemoveLinearKeys(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bNeedsSkeleton = true;
    MaxPosDiff = 0.001f;
    MaxAngleDiff = 0.00075f;
    MaxScaleDiff = 0.000001f;
    MaxEffectorDiff = 0.001f;
    MinEffectorDiff = 0.001f;
    EffectorDiffSocket = 0.001f;
    ParentKeyScale = 2.0f;
    bRetarget = true;
    bActuallyFilterLinearKeys = true;
}

}

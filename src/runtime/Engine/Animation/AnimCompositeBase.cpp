#include "ue5parse/runtime/Engine/Animation/AnimCompositeBase.h"

#include <cmath>

namespace ue5parse::runtime {

float FAnimSegment::GetLength() const {
    return static_cast<float>(LoopingCount) * (AnimEndTime - AnimStartTime) / std::abs(AnimPlayRate);
}

float FAnimTrack::GetLength() const {
    float TotalLength = 0;
    for (const FAnimSegment& Segment : AnimSegments) TotalLength = std::max(TotalLength, Segment.GetEndPos());
    return TotalLength;
}

}

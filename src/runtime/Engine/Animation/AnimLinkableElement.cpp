#include "ue5parse/runtime/Engine/Animation/AnimLinkableElement.h"

#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"

namespace ue5parse::runtime {

float FAnimLinkableElement::GetTime(EAnimLinkMethod::Type ReferenceFrame) const {
    // The absolute time of the link (UE's GetTime with an absolute reference frame).
    float Absolute = LinkValue;
    if (LinkMethod == EAnimLinkMethod::Relative) Absolute = SegmentBeginTime + LinkValue;
    else if (LinkMethod == EAnimLinkMethod::Proportional) Absolute = SegmentBeginTime + LinkValue * SegmentLength;
    switch (ReferenceFrame) {
    case EAnimLinkMethod::Relative: return Absolute - SegmentBeginTime;
    case EAnimLinkMethod::Proportional: return SegmentLength > 0 ? (Absolute - SegmentBeginTime) / SegmentLength : 0.0f;
    default: return Absolute;
    }
}

float FAnimNotifyEvent::GetEndTriggerTime() const {
    if (NotifyStateClass == nullptr) return GetTriggerTime();
    return GetTriggerTime() + GetDuration() + EndTriggerTimeOffset;
}

}

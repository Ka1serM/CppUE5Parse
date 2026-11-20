#include "Engine/Source/Runtime/Engine/Animation/AnimLinkableElement.h"

#include "Engine/Source/Runtime/Engine/Animation/AnimMontage.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimTypes.h"

namespace ue5parse::runtime {

float FAnimLinkableElement::GetTime(EAnimLinkMethod::Type ReferenceFrame) const {
    float Absolute = LinkValue;
    if (LinkMethod == EAnimLinkMethod::Relative)
        Absolute = SegmentBeginTime + LinkValue;
    else if (LinkMethod == EAnimLinkMethod::Proportional)
        Absolute = SegmentBeginTime + LinkValue * SegmentLength;
    switch (ReferenceFrame) {
    case EAnimLinkMethod::Relative:
        return Absolute - SegmentBeginTime;
    case EAnimLinkMethod::Proportional:
        return SegmentLength > 0 ? (Absolute - SegmentBeginTime) / SegmentLength : 0.0f;
    default:
        return Absolute;
    }
}

void FAnimLinkableElement::LinkMontage(UAnimMontage* Montage, float AbsMontageTime, int32 InSlotIndex) {
    if (Montage == nullptr || Montage->SlotAnimTracks.IsEmpty())
        return;
    LinkedMontage = Montage;
    SlotIndex = InSlotIndex;
    const FAnimTrack& Track = Montage->SlotAnimTracks[SlotIndex].AnimTrack;
    SegmentIndex = Track.GetSegmentIndexAtTime(AbsMontageTime);
    if (SegmentIndex == INDEX_NONE) {
        LinkValue = AbsMontageTime;
        LinkedSequence = nullptr;
        SegmentBeginTime = -1.0f;
        SegmentLength = -1.0f;
        LinkMethod = EAnimLinkMethod::Absolute;
        CachedLinkMethod = LinkMethod;
        return;
    }
    const FAnimSegment& Segment = Track.AnimSegments[SegmentIndex];
    LinkedSequence = Segment.AnimReference;
    SegmentBeginTime = Segment.StartPos;
    SegmentLength = Segment.GetLength();
    SetTime_Internal(AbsMontageTime);
}

void FAnimLinkableElement::Clear() {
    LinkValue = GetTime();
    LinkMethod = EAnimLinkMethod::Absolute;
    CachedLinkMethod = EAnimLinkMethod::Absolute;
    LinkedSequence = nullptr;
    SegmentBeginTime = -1.0f;
    SegmentLength = -1.0f;
    SegmentIndex = INDEX_NONE;
}

void FAnimLinkableElement::SetTime_Internal(float NewTime) {
    if (CachedLinkMethod == EAnimLinkMethod::Relative)
        LinkValue = NewTime - SegmentBeginTime;
    else if (CachedLinkMethod == EAnimLinkMethod::Proportional)
        LinkValue = SegmentLength > 0 ? (NewTime - SegmentBeginTime) / SegmentLength : 0.0f;
    else
        LinkValue = NewTime;
}

FName FAnimNotifyEvent::GetNotifyEventName() const {
    return FName(std::string("AnimNotify_") + NotifyName.toString());
}

float FAnimNotifyEvent::GetEndTriggerTime() const {
    if (NotifyStateClass == nullptr)
        return GetTriggerTime();
    return GetTriggerTime() + GetDuration() + EndTriggerTimeOffset;
}

}

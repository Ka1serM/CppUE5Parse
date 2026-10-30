#include "ue5parse/runtime/Engine/Animation/AnimMontage.h"

#include "ue5parse/runtime/Engine/Animation/Skeleton.h"

namespace ue5parse::runtime {

int32 UAnimMontage::GetSectionIndex(FName InSectionName) const {
    for (int32 i = 0; i < CompositeSections.Num(); i++)
        if (CompositeSections[i].SectionName == InSectionName) return i;
    return INDEX_NONE;
}

FName UAnimMontage::GetSectionName(int32 SectionIndex) const {
    return CompositeSections.IsValidIndex(SectionIndex) ? CompositeSections[SectionIndex].SectionName : FName();
}

const FSlotAnimationTrack* UAnimMontage::FindSlotTrack(FName InSlotName) const {
    for (const FSlotAnimationTrack& Track : SlotAnimTracks)
        if (Track.SlotName == InSlotName) return &Track;
    return nullptr;
}

FName UAnimMontage::GetGroupName() const {
    // The group of the first slot (UE's GetGroupName).
    if (Skeleton != nullptr && SlotAnimTracks.Num() > 0) return Skeleton->GetSlotGroupName(SlotAnimTracks[0].SlotName);
    return FAnimSlotGroup::DefaultGroupName;
}

}

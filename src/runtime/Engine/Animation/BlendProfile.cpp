#include "ue5parse/runtime/Engine/Animation/BlendProfile.h"

#include "ue5parse/runtime/Engine/Animation/Skeleton.h"

namespace ue5parse::runtime {

float UBlendProfile::GetBoneBlendScale(int32 InBoneIdx) const {
    if (OwningSkeleton == nullptr || !OwningSkeleton->GetReferenceSkeleton().IsValidIndex(InBoneIdx)) return GetDefaultBlendScale();
    return GetBoneBlendScale(OwningSkeleton->GetReferenceSkeleton().GetBoneName(InBoneIdx));
}

float UBlendProfile::GetBoneBlendScale(FName InBoneName) const {
    for (const FBlendProfileBoneEntry& Entry : ProfileEntries)
        if (Entry.BoneReference.BoneName == InBoneName) return Entry.BlendScale;
    return GetDefaultBlendScale();
}

}

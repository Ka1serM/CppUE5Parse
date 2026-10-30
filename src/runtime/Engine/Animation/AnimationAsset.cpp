#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"

namespace ue5parse::runtime {

void UAnimationAsset::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    // The skeleton's virtual bone guid when the asset was saved.
    Ar << SkeletonGuid;
}

}

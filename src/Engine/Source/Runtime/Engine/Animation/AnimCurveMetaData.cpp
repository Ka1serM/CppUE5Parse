#include "Engine/Source/Runtime/Engine/Animation/AnimCurveMetaData.h"

namespace ue5parse::runtime {

UE::Anim::ECurveElementFlags FCurveMetaData::GetFlags() const {
    UE::Anim::ECurveElementFlags Flags = UE::Anim::ECurveElementFlags::None;
    if (Type.bMaterial)
        Flags |= UE::Anim::ECurveElementFlags::Material;
    if (Type.bMorphtarget)
        Flags |= UE::Anim::ECurveElementFlags::MorphTarget;
    return Flags;
}

}

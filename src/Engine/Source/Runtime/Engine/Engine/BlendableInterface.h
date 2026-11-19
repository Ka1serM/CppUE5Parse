#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "BlendableInterface.generated.h"

namespace ue5parse::runtime {

UENUM()
enum EBlendableLocation : int32 {
    BL_SceneColorAfterTonemapping = 0,
    BL_SceneColorAfterDOF = 1,
    BL_TranslucencyAfterDOF = 5,
    BL_SSRInput = 4,
    BL_SceneColorBeforeDOF = 2,
    BL_SceneColorBeforeBloom = 6,
    BL_ReplacingTonemapper = 3,
    BL_MAX = 7,
    BL_AfterTonemapping = BL_SceneColorAfterTonemapping,
    BL_BeforeTonemapping = BL_SceneColorAfterDOF,
    BL_BeforeTranslucency = BL_SceneColorBeforeDOF,
};

}

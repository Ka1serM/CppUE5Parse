#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "RuntimeVirtualTextureEnum.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ERuntimeVirtualTextureMaterialType : uint8 {
    BaseColor,
    Mask4,
    BaseColor_Normal_Roughness,
    BaseColor_Normal_Specular,
    BaseColor_Normal_Specular_YCoCg,
    BaseColor_Normal_Specular_Mask_YCoCg,
    WorldHeight,
    Displacement,
    Count,
};

UENUM()
enum class ERuntimeVirtualTextureMainPassType : uint8 { Never, Exclusive, Always };

}

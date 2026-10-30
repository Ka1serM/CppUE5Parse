#pragma once

#include "ue5parse/runtime/Core/Containers/Array.h"

// The texture enums (UE's Engine/TextureDefines.h and Engine/Texture.h), with UE's values.

namespace ue5parse::runtime {

enum TextureCompressionSettings : int32 {
    TC_Default,
    TC_Normalmap,
    TC_Masks,
    TC_Grayscale,
    TC_Displacementmap,
    TC_VectorDisplacementmap,
    TC_HDR,
    TC_EditorIcon,
    TC_Alpha,
    TC_DistanceFieldFont,
    TC_HDR_Compressed,
    TC_BC7,
    TC_HalfFloat,
    TC_LQ,
    TC_EncodedReflectionCapture,
    TC_SingleFloat,
    TC_HDR_F32,
    TC_MAX,
};

enum TextureFilter : int32 { TF_Nearest, TF_Bilinear, TF_Trilinear, TF_Default, TF_MAX };
enum TextureAddress : int32 { TA_Wrap, TA_Clamp, TA_Mirror, TA_MAX };

enum TextureGroup : int32 {
    TEXTUREGROUP_World,
    TEXTUREGROUP_WorldNormalMap,
    TEXTUREGROUP_WorldSpecular,
    TEXTUREGROUP_Character,
    TEXTUREGROUP_CharacterNormalMap,
    TEXTUREGROUP_CharacterSpecular,
    TEXTUREGROUP_Weapon,
    TEXTUREGROUP_WeaponNormalMap,
    TEXTUREGROUP_WeaponSpecular,
    TEXTUREGROUP_Vehicle,
    TEXTUREGROUP_VehicleNormalMap,
    TEXTUREGROUP_VehicleSpecular,
    TEXTUREGROUP_Cinematic,
    TEXTUREGROUP_Effects,
    TEXTUREGROUP_EffectsNotFiltered,
    TEXTUREGROUP_Skybox,
    TEXTUREGROUP_UI,
    TEXTUREGROUP_Lightmap,
    TEXTUREGROUP_RenderTarget,
    TEXTUREGROUP_MobileFlattened,
    TEXTUREGROUP_ProcBuilding_Face,
    TEXTUREGROUP_ProcBuilding_LightMap,
    TEXTUREGROUP_Shadowmap,
    TEXTUREGROUP_ColorLookupTable,
    TEXTUREGROUP_Terrain_Heightmap,
    TEXTUREGROUP_Terrain_Weightmap,
    TEXTUREGROUP_Bokeh,
    TEXTUREGROUP_IESLightProfile,
    TEXTUREGROUP_Pixels2D,
    TEXTUREGROUP_HierarchicalLOD,
    TEXTUREGROUP_Impostor,
    TEXTUREGROUP_ImpostorNormalDepth,
    TEXTUREGROUP_8BitData,
    TEXTUREGROUP_16BitData,
    TEXTUREGROUP_Project01,
    TEXTUREGROUP_MAX = TEXTUREGROUP_16BitData + 33,
};

}

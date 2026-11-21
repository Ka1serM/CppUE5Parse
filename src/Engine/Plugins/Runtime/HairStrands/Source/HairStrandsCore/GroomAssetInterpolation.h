#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "GroomAssetInterpolation.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EGroomGeometryType : uint8 { Strands, Cards, Meshes };

UENUM()
enum class EGroomBindingType : uint8 { Rigid, Skinning };

UENUM()
enum class EGroomOverrideType : uint8 { Auto, Enable, Disable };

UENUM()
enum class EGroomInterpolationType : uint8 { None = 0, RigidTransform = 0x02, OffsetTransform = 0x04, SmoothTransform = 0x08 };

UENUM()
enum class EGroomLODMode : uint8 { Default, Manual, Auto };

UENUM()
enum class EGroomLODType : uint8 { Manual, Auto };

UENUM()
enum class EGroomGuideType : uint8 { Imported, Generated, Rigged };

UENUM()
enum class EHairInterpolationQuality : uint8 { Low, Medium, High, Unknown };

UENUM()
enum class EHairInterpolationWeight : uint8 { Parametric, Root, Index, Distance, Unknown };

USTRUCT()
struct FHairLODSettings {
    GENERATED_BODY()
    UPROPERTY()
    float CurveDecimation = 1.0f;
    UPROPERTY()
    float VertexDecimation = 1.0f;
    UPROPERTY()
    float AngularThreshold = 1.0f;
    UPROPERTY()
    float ScreenSize = 1.0f;
    UPROPERTY()
    float ThicknessScale = 1.0f;
    UPROPERTY()
    bool bVisible = true;
    UPROPERTY()
    EGroomGeometryType GeometryType = EGroomGeometryType::Strands;
    UPROPERTY()
    EGroomBindingType BindingType = EGroomBindingType::Skinning;
    UPROPERTY()
    EGroomOverrideType Simulation = EGroomOverrideType::Auto;
    UPROPERTY()
    EGroomOverrideType GlobalInterpolation = EGroomOverrideType::Auto;
};

USTRUCT()
struct FHairDecimationSettings {
    GENERATED_BODY()
    UPROPERTY()
    float CurveDecimation = 1.0f;
    UPROPERTY()
    float VertexDecimation = 1.0f;
};

USTRUCT()
struct FHairInterpolationSettings {
    GENERATED_BODY()
    UPROPERTY()
    EGroomGuideType GuideType = EGroomGuideType::Imported;
    UPROPERTY()
    bool bOverrideGuides_DEPRECATED = false;
    UPROPERTY()
    float HairToGuideDensity = 0.1f;
    UPROPERTY()
    EHairInterpolationQuality InterpolationQuality = EHairInterpolationQuality::High;
    UPROPERTY()
    EHairInterpolationWeight InterpolationDistance = EHairInterpolationWeight::Parametric;
    UPROPERTY()
    bool bRandomizeGuide = false;
    UPROPERTY()
    bool bUseUniqueGuide = false;
    UPROPERTY()
    int32 RiggedGuideNumCurves = 10;
    UPROPERTY()
    int32 RiggedGuideNumPoints = 4;
};

USTRUCT()
struct FHairDeformationSettings {
    GENERATED_BODY()
    UPROPERTY()
    bool bCanEditRigging = false;
    UPROPERTY()
    bool bEnableRigging_DEPRECATED = false;
    UPROPERTY()
    int32 NumCurves_DEPRECATED = 10;
    UPROPERTY()
    int32 NumPoints_DEPRECATED = 4;
};

USTRUCT()
struct FHairGroupsInterpolation {
    GENERATED_BODY()
    UPROPERTY()
    FHairDecimationSettings DecimationSettings;
    UPROPERTY()
    FHairInterpolationSettings InterpolationSettings;
    UPROPERTY()
    FHairDeformationSettings RiggingSettings;
};

USTRUCT()
struct FHairGroupsLOD {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FHairLODSettings> LODs;
    UPROPERTY()
    float ClusterWorldSize = 0.0f;
    UPROPERTY()
    float ClusterScreenSizeScale = 1.0f;
    UPROPERTY()
    EGroomLODType LODType = EGroomLODType::Manual;
    UPROPERTY()
    float AutoLODBias = 0.0f;
};

}

#pragma once

#include <array>

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyKey.h"
#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyPose.h"

namespace ue5parse::runtime {

struct FRigParentConstraint {
    FRigElementKey Parent;
    FVector3f InitialWeight;
    FVector3f Weight;
    FName DisplayLabel;
};

struct FRigMultiParentElement {
    FRigCurrentAndInitialTransform Pose;
    FRigCurrentAndInitialTransform LegacyParent;
    std::vector<FRigParentConstraint> Constraints;

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& Settings);
    void LoadParents(FArchive& Ar);
};

struct FRigControlSettings {
    FName AnimationType, ControlType, DisplayName, PrimaryAxis;
    bool bIsCurve = false;
    bool bAnimatable = false;
    std::array<bool, 3> LegacyLimits{};
    TArray<TPair<bool, bool>> Limits;
    bool bDrawLimits = false;
    std::array<float, 32> MinimumValue{}, MaximumValue{};
    FTransform LegacyMinimum, LegacyMaximum;
    bool bShapeEnabled = false;
    bool bShapeVisible = false;
    FName ShapeVisibility, ShapeName;
    FLinearColor ShapeColor;
    bool bTransient = false;
    FString ControlEnumPath;
    TArray<TPair<FRigElementKey, FName>> AvailableSpaces;
    TArray<FRigElementKey> DrivenControls;
    bool bGroupWithParent = false;
    bool bRestrictSpaceSwitching = false;
    TArray<uint8> FilteredChannels;
    uint8 PreferredRotationOrder = 0;
    bool bUsePreferredRotationOrder = false;

    void Load(FArchive& Ar);
};

struct FRigControlElement : FRigMultiParentElement {
    FRigControlSettings Control;
    FRigCurrentAndInitialTransform Offset, Shape;
    FName EulerRotationOrder;
    FVector CurrentEuler, InitialEuler;

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& Settings);
};

}

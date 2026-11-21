#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyControl.h"

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigObjectVersion.h"

namespace ue5parse::runtime {

using Version = FControlRigObjectVersion;

void FRigMultiParentElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& Settings) {
    Pose.Load(Ar, Settings);
    if (!Ar.CustomVerAtLeast(Version::GUID, Version::RemovedMultiParentParentCache, EGame::GAME_UE5_1))
        LegacyParent.Load(Ar, Settings);
    Constraints.resize(Ar.ReadCount(8));
}

void FRigMultiParentElement::LoadParents(FArchive& Ar) {
    for (auto& Constraint : Constraints) {
        Ar << Constraint.Parent;
        if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyMultiParentConstraints, EGame::GAME_UE5_0))
            Ar << Constraint.InitialWeight << Constraint.Weight;
        else {
            float Initial = 0, Current = 0;
            Ar << Initial << Current;
            Constraint.InitialWeight = {Initial, Initial, Initial};
            Constraint.Weight = {Current, Current, Current};
        }
        if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyParentContraintWithLabel, EGame::GAME_UE5_6))
            Ar << Constraint.DisplayLabel;
        if (Ar.IsError())
            return;
    }
}

void FRigControlSettings::Load(FArchive& Ar) {
    const bool HasAnimationType = Ar.CustomVerAtLeast(Version::GUID, Version::ControlAnimationType, EGame::GAME_UE5_1);
    if (HasAnimationType)
        Ar << AnimationType;
    Ar << ControlType << DisplayName << PrimaryAxis << bIsCurve;
    if (!HasAnimationType)
        Ar << bAnimatable;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::PerChannelLimits, EGame::GAME_UE5_0))
        Ar << Limits;
    else
        for (bool& Limit : LegacyLimits)
            Ar << Limit;
    Ar << bDrawLimits;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::StorageMinMaxValuesAsFloatStorage, EGame::GAME_UE5_0)) {
        for (auto& Value : MinimumValue)
            Ar << Value;
        for (auto& Value : MaximumValue)
            Ar << Value;
    } else
        Ar << LegacyMinimum << LegacyMaximum;
    if (!HasAnimationType)
        Ar << bShapeEnabled;
    Ar << bShapeVisible;
    if (HasAnimationType)
        Ar << ShapeVisibility;
    Ar << ShapeName << ShapeColor << bTransient << ControlEnumPath;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyParentContraintWithLabel, EGame::GAME_UE5_6))
        Ar << AvailableSpaces;
    else if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyControlSpaceFavorites, EGame::GAME_UE5_0)) {
        const int32 Count = Ar.ReadCount(8);
        for (int32 Index = 0; Index < Count && !Ar.IsError(); ++Index)
            Ar << AvailableSpaces[AvailableSpaces.AddDefaulted()].Key;
    }
    if (HasAnimationType)
        Ar << DrivenControls << bGroupWithParent;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::RestrictSpaceSwitchingForControls, EGame::GAME_UE5_2))
        Ar << bRestrictSpaceSwitching;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::ControlTransformChannelFiltering, EGame::GAME_UE5_2))
        Ar << FilteredChannels;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyControlPreferredRotationOrder, EGame::GAME_UE5_3))
        Ar << PreferredRotationOrder;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyControlPreferredRotationOrderFlag, EGame::GAME_UE5_3))
        Ar << bUsePreferredRotationOrder;
}

void FRigControlElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& Settings) {
    FRigMultiParentElement::Load(Ar, Settings);
    Control.Load(Ar);
    Offset.Load(Ar, Settings);
    Shape.Load(Ar, Settings);
    if (Ar.CustomVerAtLeast(Version::GUID, Version::PreferredEulerAnglesForControls, EGame::GAME_UE5_1))
        Ar << EulerRotationOrder << CurrentEuler << InitialEuler;
}

}

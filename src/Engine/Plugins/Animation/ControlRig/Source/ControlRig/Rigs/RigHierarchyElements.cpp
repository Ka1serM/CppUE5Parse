#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyElements.h"

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchy.h"
#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyMetadata.h"

namespace ue5parse::runtime {

using Version = FControlRigObjectVersion;

namespace {

template <class TEnum>
bool LoadEnumByName(FArchive& Ar, TEnum& OutValue, const char* Where) {
    FName ValueName;
    Ar << ValueName;
    if (Ar.IsError())
        return false;
    const int64 Value = StaticEnum<TEnum>()->GetValueByName(ValueName);
    if (Value == INDEX_NONE) {
        Ar.SetError(ErrorCode::InvalidData, Where);
        return false;
    }
    OutValue = static_cast<TEnum>(Value);
    return true;
}

FRigTransformElement* FindParent(FArchive& Ar, URigHierarchy* Owner, const FRigElementKey& ParentKey, const char* Where) {
    FRigTransformElement* Parent = Owner->Find<FRigTransformElement>(ParentKey);
    if (Parent == nullptr)
        Ar.SetError(ErrorCode::InvalidData, Where);
    return Parent;
}

}

void FRigBaseElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) {
    if (InSettings.SerializationPhase != FRigHierarchySerializationSettings::StaticData)
        return;

    FRigElementKey LoadedKey;
    Ar << LoadedKey;
    if (Ar.IsError())
        return;
    if (LoadedKey.Type != Key.Type) {
        Ar.SetError(ErrorCode::InvalidData, "FRigBaseElement.Key");
        return;
    }
    Key = LoadedKey;

    if (!Version::AtLeast(Ar, Version::HierarchyElementMetadata) || Version::AtLeast(Ar, Version::RigHierarchyStoresElementMetadata))
        return;

    int32 MetadataNum = 0;
    Ar << MetadataNum;
    for (int32 MetadataIndex = 0; MetadataIndex < MetadataNum && !Ar.IsError(); MetadataIndex++) {
        FName MetadataName;
        Ar << MetadataName;
        ERigMetadataType MetadataType = ERigMetadataType::Invalid;
        if (!LoadEnumByName(Ar, MetadataType, "FRigBaseElement.MetadataType"))
            return;
        FRigBaseMetadata* Md = Owner->GetMetadataForElement(this, MetadataName, MetadataType);
        if (Md == nullptr) {
            Ar.SetError(ErrorCode::InvalidData, "FRigBaseElement.Metadata");
            return;
        }
        Md->Serialize(Ar);
    }
}

void FRigCompactTransform::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings, ERepresentation* OutRepresentation) {
    if (InSettings.ControlRigVersion < Version::RigHierarchyCompactTransformSerialization) {
        Ar << Transform;
        return;
    }

    uint8 State = Float_Zero_Identity_One;
    Ar << State;

    if (OutRepresentation)
        *OutRepresentation = static_cast<ERepresentation>(State);

    if (State == Double_Complete) {
        Ar << Transform;
        return;
    }

    Transform = FTransform::Identity;
    FVector3f Position = FVector3f::ZeroVector;
    FQuat4f Rotation = FQuat4f::Identity;
    FVector3f Scale = FVector3f::OneVector;
    float ScaleX = 1.f;

    switch (State) {
    case Float_Zero_Identity_One:
        break;
    case Float_Zero_Identity_Uniform:
        Ar << ScaleX;
        Transform.SetScale3D(FVector(ScaleX, ScaleX, ScaleX));
        break;
    case Float_Zero_Identity_NonUniform:
        Ar << Scale;
        Transform.SetScale3D(FVector(Scale));
        break;
    case Float_Zero_Quat_One:
        Ar << Rotation;
        Transform.SetRotation(FQuat(Rotation));
        break;
    case Float_Zero_Quat_Uniform:
        Ar << Rotation;
        Ar << ScaleX;
        Transform.SetRotation(FQuat(Rotation));
        Transform.SetScale3D(FVector(ScaleX, ScaleX, ScaleX));
        break;
    case Float_Zero_Quat_NonUniform:
        Ar << Rotation;
        Ar << Scale;
        Transform.SetRotation(FQuat(Rotation));
        Transform.SetScale3D(FVector(Scale));
        break;
    case Float_Position_Identity_One:
        Ar << Position;
        Transform.SetLocation(FVector(Position));
        break;
    case Float_Position_Identity_Uniform:
        Ar << Position;
        Ar << ScaleX;
        Transform.SetLocation(FVector(Position));
        Transform.SetScale3D(FVector(ScaleX, ScaleX, ScaleX));
        break;
    case Float_Position_Identity_NonUniform:
        Ar << Position;
        Ar << Scale;
        Transform.SetLocation(FVector(Position));
        Transform.SetScale3D(FVector(Scale));
        break;
    case Float_Position_Quat_One:
        Ar << Position;
        Ar << Rotation;
        Transform.SetLocation(FVector(Position));
        Transform.SetRotation(FQuat(Rotation));
        break;
    case Float_Position_Quat_Uniform:
        Ar << Position;
        Ar << Rotation;
        Ar << ScaleX;
        Transform.SetLocation(FVector(Position));
        Transform.SetRotation(FQuat(Rotation));
        Transform.SetScale3D(FVector(ScaleX, ScaleX, ScaleX));
        break;
    case Float_Position_Quat_NonUniform:
        Ar << Position;
        Ar << Rotation;
        Ar << Scale;
        Transform.SetLocation(FVector(Position));
        Transform.SetRotation(FQuat(Rotation));
        Transform.SetScale3D(FVector(Scale));
        break;
    default:
        Ar << Transform;
        break;
    }
}

void FRigComputedTransform::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings, FRigTransformDirtyState& InDirtyState) {
    FTransform LoadedTransform = FTransform::Identity;
    bool bDirty = false;

    if (InSettings.ControlRigVersion < Version::RigHierarchyCompactTransformSerialization) {
        Ar << LoadedTransform;
        Ar << bDirty;
    } else {
        Ar << bDirty;
        if (!bDirty)
            FRigCompactTransform(LoadedTransform).Load(Ar, InSettings);
    }

    Set(LoadedTransform);
    (void)InDirtyState.Set(bDirty);
}

void FRigLocalAndGlobalTransform::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings, FRigLocalAndGlobalDirtyState& OutDirtyState) {
    if (InSettings.bSerializeLocalTransform) {
        Local.Load(Ar, InSettings, OutDirtyState.Local);
        if (!InSettings.bSerializeGlobalTransform)
            OutDirtyState.Global.Set(true);
    }
    if (InSettings.bSerializeGlobalTransform) {
        Global.Load(Ar, InSettings, OutDirtyState.Global);
        if (!InSettings.bSerializeLocalTransform)
            OutDirtyState.Local.Set(true);
    }
}

void FRigCurrentAndInitialTransform::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings, FRigCurrentAndInitialDirtyState& OutDirtyState) {
    if (InSettings.bSerializeCurrentTransform)
        Current.Load(Ar, InSettings, OutDirtyState.Current);
    if (!InSettings.bSerializeInitialTransform)
        return;
    Initial.Load(Ar, InSettings, OutDirtyState.Initial);
    if (!InSettings.bSerializeCurrentTransform) {
        Current = Initial;
        OutDirtyState.Current = OutDirtyState.Initial;
    }
}

void FRigPreferredEulerAngles::Load(FArchive& Ar) {
    if (!LoadEnumByName(Ar, RotationOrder, "FRigPreferredEulerAngles.RotationOrder"))
        return;
    Ar << Current;
    Ar << Initial;
}

void FRigPreferredEulerAngles::Reset() {
    RotationOrder = DefaultRotationOrder;
    Initial = Current = FVector::ZeroVector;
}

void FRigTransformElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) {
    FRigBaseElement::Load(Ar, InSettings);

    if (InSettings.SerializationPhase == FRigHierarchySerializationSettings::StaticData)
        GetTransform().Load(Ar, InSettings, GetDirtyState());
}

void FRigSingleParentElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) {
    FRigTransformElement::Load(Ar, InSettings);

    if (InSettings.SerializationPhase != FRigHierarchySerializationSettings::InterElementData)
        return;

    FRigElementKey ParentKey;
    Ar << ParentKey;
    if (!Ar.IsError() && ParentKey.IsValid())
        ParentElement = FindParent(Ar, Owner, ParentKey, "FRigSingleParentElement.ParentElement");
}

void FRigMultiParentElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) {
    FRigTransformElement::Load(Ar, InSettings);

    if (InSettings.SerializationPhase == FRigHierarchySerializationSettings::StaticData) {
        if (!Version::AtLeast(Ar, Version::RemovedMultiParentParentCache)) {
            FRigCurrentAndInitialTransform Parent;
            FRigCurrentAndInitialDirtyState DirtyState;
            Parent.Load(Ar, InSettings, DirtyState);
        }

        const int32 NumParents = Ar.ReadCount(4);
        ParentConstraints.SetNum(NumParents);
        return;
    }

    for (int32 ParentIndex = 0; ParentIndex < ParentConstraints.Num() && !Ar.IsError(); ParentIndex++) {
        FRigElementKey ParentKey;
        Ar << ParentKey;
        if (Ar.IsError())
            return;

        FRigElementParentConstraint& Constraint = ParentConstraints[ParentIndex];
        Constraint.ParentElement = FindParent(Ar, Owner, ParentKey, "FRigMultiParentElement.ParentConstraints");
        Constraint.bCacheIsDirty = true;

        if (Version::AtLeast(Ar, Version::RigHierarchyMultiParentConstraints)) {
            Ar << Constraint.InitialWeight;
            Ar << Constraint.Weight;
        } else {
            float InitialWeight = 0.f;
            Ar << InitialWeight;
            Constraint.InitialWeight = FRigElementWeight(InitialWeight);

            float Weight = 0.f;
            Ar << Weight;
            Constraint.Weight = FRigElementWeight(Weight);
        }

        if (Version::AtLeast(Ar, Version::RigHierarchyParentContraintWithLabel))
            Ar << Constraint.DisplayLabel;
        else
            Constraint.DisplayLabel = FName();

        IndexLookup.Add(ParentKey, ParentIndex);
    }
}

void FRigBoneElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) {
    FRigSingleParentElement::Load(Ar, InSettings);

    if (InSettings.SerializationPhase == FRigHierarchySerializationSettings::StaticData)
        LoadEnumByName(Ar, BoneType, "FRigBoneElement.BoneType");
}

void FRigControlSettings::SetAnimationTypeFromDeprecatedData(bool bAnimatable, bool bShapeEnabled) {
    if (!bAnimatable) {
        AnimationType = ERigControlAnimationType::ProxyControl;
        return;
    }
    if (bShapeEnabled && ControlType != ERigControlType::Bool)
        AnimationType = ERigControlAnimationType::AnimationControl;
    else
        AnimationType = ERigControlAnimationType::AnimationChannel;
}

void FRigControlSettings::Load(FArchive& Ar) {
    bool bLimitTranslation_DEPRECATED = false;
    bool bLimitRotation_DEPRECATED = false;
    bool bLimitScale_DEPRECATED = false;
    bool bAnimatableDeprecated = false;
    bool bShapeEnabledDeprecated = false;

    const bool bHasAnimationType = Version::AtLeast(Ar, Version::ControlAnimationType);
    if (bHasAnimationType && !LoadEnumByName(Ar, AnimationType, "FRigControlSettings.AnimationType"))
        return;
    if (!LoadEnumByName(Ar, ControlType, "FRigControlSettings.ControlType"))
        return;
    Ar << DisplayName;
    if (!LoadEnumByName(Ar, PrimaryAxis, "FRigControlSettings.PrimaryAxis"))
        return;
    Ar << bIsCurve;
    if (!bHasAnimationType)
        Ar << bAnimatableDeprecated;
    const bool bHasPerChannelLimits = Version::AtLeast(Ar, Version::PerChannelLimits);
    if (!bHasPerChannelLimits) {
        Ar << bLimitTranslation_DEPRECATED;
        Ar << bLimitRotation_DEPRECATED;
        Ar << bLimitScale_DEPRECATED;
    } else {
        Ar << LimitEnabled;
    }
    Ar << bDrawLimits;

    if (Version::AtLeast(Ar, Version::StorageMinMaxValuesAsFloatStorage)) {
        Ar << MinimumValue;
        Ar << MaximumValue;
    } else {
        Ar << MinimumTransform_DEPRECATED;
        Ar << MaximumTransform_DEPRECATED;
    }

    if (!bHasAnimationType) {
        Ar << bShapeEnabledDeprecated;
        SetAnimationTypeFromDeprecatedData(bAnimatableDeprecated, bShapeEnabledDeprecated);
    }

    Ar << bShapeVisible;

    if (!bHasAnimationType)
        ShapeVisibility = ERigControlVisibility::UserDefined;
    else if (!LoadEnumByName(Ar, ShapeVisibility, "FRigControlSettings.ShapeVisibility"))
        return;
    Ar << ShapeName;

    //! UE before RenameGizmoToShape renames the default gizmo Gizmo to the default shape Default.
    if (!Version::AtLeast(Ar, Version::RenameGizmoToShape) && ShapeName == FName("Gizmo"))
        ShapeName = FName("Default");

    Ar << ShapeColor;
    Ar << bIsTransientControl;
    Ar << ControlEnumPathName;

    if (Version::AtLeast(Ar, Version::RigHierarchyParentContraintWithLabel)) {
        Ar << Customization.AvailableSpaces;
    } else if (Version::AtLeast(Ar, Version::RigHierarchyControlSpaceFavorites)) {
        TArray<FRigElementKey> AvailableSpaceKeys;
        Ar << AvailableSpaceKeys;
        Customization.AvailableSpaces.Reset();
        for (const FRigElementKey& AvailableSpaceKey : AvailableSpaceKeys)
            Customization.AvailableSpaces.Add(FRigElementKeyWithLabel(AvailableSpaceKey, FName()));
    } else {
        Customization.AvailableSpaces.Reset();
    }

    if (bHasAnimationType)
        Ar << DrivenControls;
    else
        DrivenControls.Reset();

    PreviouslyDrivenControls.Reset();

    if (!bHasPerChannelLimits)
        SetupLimitArrayForType(bLimitTranslation_DEPRECATED, bLimitRotation_DEPRECATED, bLimitScale_DEPRECATED);

    if (bHasAnimationType) {
        Ar << bGroupWithParentControl;
    } else {
        bGroupWithParentControl = IsAnimatable() && (ControlType == ERigControlType::Bool || ControlType == ERigControlType::Float ||
                                                     ControlType == ERigControlType::ScaleFloat || ControlType == ERigControlType::Integer ||
                                                     ControlType == ERigControlType::Vector2D);
    }

    if (Version::AtLeast(Ar, Version::RestrictSpaceSwitchingForControls))
        Ar << bRestrictSpaceSwitching;
    else
        bRestrictSpaceSwitching = false;

    if (Version::AtLeast(Ar, Version::ControlTransformChannelFiltering))
        Ar << FilteredChannels;
    else
        FilteredChannels.Reset();

    if (Version::AtLeast(Ar, Version::RigHierarchyControlPreferredRotationOrder))
        Ar << PreferredRotationOrder;
    else
        PreferredRotationOrder = FRigPreferredEulerAngles::DefaultRotationOrder;

    if (Version::AtLeast(Ar, Version::RigHierarchyControlPreferredRotationOrderFlag))
        Ar << bUsePreferredRotationOrder;
    else
        bUsePreferredRotationOrder = false;

    ShapeTransform = FTransform::Identity;
}

void FRigControlSettings::SetupLimitArrayForType(bool bLimitTranslation, bool bLimitRotation, bool bLimitScale) {
    const auto Fill = [this](int32 Start, int32 Count, bool bValue) {
        for (int32 Index = Start; Index < Start + Count; Index++)
            LimitEnabled[Index].Set(bValue);
    };
    switch (ControlType) {
    case ERigControlType::Integer:
    case ERigControlType::Float:
        LimitEnabled.SetNum(1);
        Fill(0, 1, bLimitTranslation);
        break;
    case ERigControlType::ScaleFloat:
        LimitEnabled.SetNum(1);
        Fill(0, 1, bLimitScale);
        break;
    case ERigControlType::Vector2D:
        LimitEnabled.SetNum(2);
        Fill(0, 2, bLimitTranslation);
        break;
    case ERigControlType::Position:
        LimitEnabled.SetNum(3);
        Fill(0, 3, bLimitTranslation);
        break;
    case ERigControlType::Scale:
        LimitEnabled.SetNum(3);
        Fill(0, 3, bLimitScale);
        break;
    case ERigControlType::Rotator:
        LimitEnabled.SetNum(3);
        Fill(0, 3, bLimitRotation);
        break;
    case ERigControlType::TransformNoScale:
        LimitEnabled.SetNum(6);
        Fill(0, 3, bLimitTranslation);
        Fill(3, 3, bLimitRotation);
        break;
    case ERigControlType::EulerTransform:
    case ERigControlType::Transform:
        LimitEnabled.SetNum(9);
        Fill(0, 3, bLimitTranslation);
        Fill(3, 3, bLimitRotation);
        Fill(6, 3, bLimitScale);
        break;
    case ERigControlType::Bool:
        LimitEnabled.Reset();
        break;
    }
}

void FRigControlElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) {
    FRigMultiParentElement::Load(Ar, InSettings);

    if (InSettings.SerializationPhase != FRigHierarchySerializationSettings::StaticData)
        return;

    Settings.Load(Ar);
    GetOffsetTransform().Load(Ar, InSettings, GetOffsetDirtyState());
    GetShapeTransform().Load(Ar, InSettings, GetShapeDirtyState());

    if (Version::AtLeast(Ar, Version::PreferredEulerAnglesForControls))
        PreferredEulerAngles.Load(Ar);
    else
        PreferredEulerAngles.Reset();
    PreferredEulerAngles.SetRotationOrder(Settings.PreferredRotationOrder);
}

void FRigCurveElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) {
    FRigBaseElement::Load(Ar, InSettings);

    if (InSettings.SerializationPhase != FRigHierarchySerializationSettings::StaticData)
        return;

    bool bLoadedValueSet = true;
    if (Version::AtLeast(Ar, Version::CurveElementValueStateFlag))
        Ar << bLoadedValueSet;

    float LoadedValue = 0.f;
    Ar << LoadedValue;

    Set(LoadedValue, bLoadedValueSet);
}

void FRigRigidBodySettings::Load(FArchive& Ar) {
    Ar << Mass;
}

void FRigRigidBodyElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) {
    FRigSingleParentElement::Load(Ar, InSettings);

    if (InSettings.SerializationPhase == FRigHierarchySerializationSettings::StaticData)
        Settings.Load(Ar);
}

void FRigPhysicsSettings::Load(FArchive& Ar) {
    Ar << Mass;
}

void FRigPhysicsElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) {
    FRigSingleParentElement::Load(Ar, InSettings);

    if (InSettings.SerializationPhase != FRigHierarchySerializationSettings::StaticData)
        return;

    Ar << Solver;
    Settings.Load(Ar);
}

void FRigConnectorSettings::Load(FArchive& Ar) {
    Ar << Description;

    if (Version::AtLeast(Ar, Version::ConnectorsWithType)) {
        Ar << Type;
        Ar << bOptional;
    }

    if (Version::AtLeast(Ar, Version::RigHierarchyArrayConnectors))
        Ar << bIsArray;
    else
        bIsArray = false;

    if (Version::AtLeast(Ar, Version::RigHierarchyPostConstructionConnectors))
        Ar << bPostConstruction;
    else
        bPostConstruction = false;

    const int32 NumRules = Ar.ReadCount(8);
    Rules.SetNum(NumRules);
    for (int32 Index = 0; Index < NumRules && !Ar.IsError(); Index++)
        Rules[Index].Load(Ar);
}

void FRigConnectorElement::Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) {
    FRigBaseElement::Load(Ar, InSettings);

    if (InSettings.SerializationPhase == FRigHierarchySerializationSettings::StaticData)
        Settings.Load(Ar);
}

}

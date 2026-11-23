#pragma once

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigConnectionRules.h"
#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyDefines.h"
#include "Engine/Source/Runtime/AnimationCore/EulerTransform.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"

#include "RigHierarchyElements.generated.h"

namespace ue5parse::runtime {

class URigHierarchy;

#define DECLARE_RIG_ELEMENT_METHODS(ElementType)                                                                                                               \
    UScriptStruct* GetScriptStruct() const override { return ElementType::StaticStruct(); }                                                                    \
    static bool IsClassOf(const FRigBaseElement* InElement) { return InElement->GetScriptStruct()->IsChildOf(StaticStruct()); }

USTRUCT()
struct FRigBaseElement {
    GENERATED_BODY()

    virtual ~FRigBaseElement() = default;

    virtual UScriptStruct* GetScriptStruct() const { return FRigBaseElement::StaticStruct(); }
    static bool IsClassOf(const FRigBaseElement*) { return true; }
    template <typename T>
    bool IsA() const {
        return T::IsClassOf(this);
    }
    template <typename T>
    friend const T* Cast(const FRigBaseElement* InElement) {
        return InElement != nullptr && InElement->IsA<T>() ? static_cast<const T*>(InElement) : nullptr;
    }
    template <typename T>
    friend T* Cast(FRigBaseElement* InElement) {
        return InElement != nullptr && InElement->IsA<T>() ? static_cast<T*>(InElement) : nullptr;
    }

    virtual void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings);

    const FRigElementKey& GetKey() const { return Key; }
    ERigElementType GetType() const { return Key.Type; }
    int32 GetIndex() const { return Index; }

    UPROPERTY()
    FRigElementKey Key;
    UPROPERTY()
    int32 Index = INDEX_NONE;
    UPROPERTY()
    int32 SubIndex = INDEX_NONE;
    int32 MetadataStorageIndex = INDEX_NONE;
    URigHierarchy* Owner = nullptr;
};

struct FRigCompactTransform {
    enum ERepresentation : uint8 {
        Float_Zero_Identity_One = 0,
        Float_Zero_Identity_Uniform,
        Float_Zero_Identity_NonUniform,
        Float_Zero_Quat_One,
        Float_Zero_Quat_Uniform,
        Float_Zero_Quat_NonUniform,
        Float_Position_Identity_One,
        Float_Position_Identity_Uniform,
        Float_Position_Identity_NonUniform,
        Float_Position_Quat_One,
        Float_Position_Quat_Uniform,
        Float_Position_Quat_NonUniform,
        Double_Complete,
    };

    explicit FRigCompactTransform(FTransform& InTransform) : Transform(InTransform) {}

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings, ERepresentation* OutRepresentation = nullptr);

    FTransform& Transform;
};

USTRUCT()
struct FRigTransformDirtyState {
    GENERATED_BODY()

    bool Get() const { return bDirty; }
    bool Set(bool InDirty) {
        if (bDirty == InDirty)
            return false;
        bDirty = InDirty;
        return true;
    }

    //! UE keeps the flag in the shared storage of the hierarchy; the port keeps it in the element.
    bool bDirty = false;
};

USTRUCT()
struct FRigLocalAndGlobalDirtyState {
    GENERATED_BODY()
    FRigTransformDirtyState Global;
    FRigTransformDirtyState Local;
};

USTRUCT()
struct FRigCurrentAndInitialDirtyState {
    GENERATED_BODY()
    FRigLocalAndGlobalDirtyState Current;
    FRigLocalAndGlobalDirtyState Initial;
};

USTRUCT()
struct FRigComputedTransform {
    GENERATED_BODY()

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings, FRigTransformDirtyState& InDirtyState);

    const FTransform& Get() const { return Transform; }
    void Set(const FTransform& InTransform) { Transform = InTransform; }

    //! UE keeps the transform in the shared storage of the hierarchy; the port keeps it in the element.
    FTransform Transform = FTransform::Identity;
};

USTRUCT()
struct FRigLocalAndGlobalTransform {
    GENERATED_BODY()

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings, FRigLocalAndGlobalDirtyState& OutDirtyState);

    FRigComputedTransform Local;
    FRigComputedTransform Global;
};

USTRUCT()
struct FRigCurrentAndInitialTransform {
    GENERATED_BODY()

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings, FRigCurrentAndInitialDirtyState& OutDirtyState);

    FRigLocalAndGlobalTransform Current;
    FRigLocalAndGlobalTransform Initial;
};

USTRUCT()
struct FRigPreferredEulerAngles {
    GENERATED_BODY()

    static constexpr EEulerRotationOrder DefaultRotationOrder = EEulerRotationOrder::YZX;

    void Load(FArchive& Ar);
    void Reset();
    void SetRotationOrder(EEulerRotationOrder InRotationOrder) { RotationOrder = InRotationOrder; }

    UPROPERTY()
    EEulerRotationOrder RotationOrder = DefaultRotationOrder;
    UPROPERTY()
    FVector Current = FVector::ZeroVector;
    UPROPERTY()
    FVector Initial = FVector::ZeroVector;
};

USTRUCT()
struct FRigTransformElement : public FRigBaseElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigTransformElement)

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) override;

    FRigCurrentAndInitialTransform& GetTransform() { return PoseStorage; }
    FRigCurrentAndInitialDirtyState& GetDirtyState() { return PoseDirtyState; }

    FRigCurrentAndInitialTransform PoseStorage;
    FRigCurrentAndInitialDirtyState PoseDirtyState;
};

USTRUCT()
struct FRigSingleParentElement : public FRigTransformElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigSingleParentElement)

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) override;

    FRigTransformElement* ParentElement = nullptr;
};

USTRUCT()
struct FRigElementWeight {
    GENERATED_BODY()

    FRigElementWeight() = default;
    explicit FRigElementWeight(float InWeight) : Location(InWeight), Rotation(InWeight), Scale(InWeight) {}

    UPROPERTY()
    float Location = 1.f;
    UPROPERTY()
    float Rotation = 1.f;
    UPROPERTY()
    float Scale = 1.f;

    friend FArchive& operator<<(FArchive& Ar, FRigElementWeight& Weight) {
        Ar << Weight.Location;
        Ar << Weight.Rotation;
        Ar << Weight.Scale;
        return Ar;
    }
};

USTRUCT()
struct FRigElementParentConstraint {
    GENERATED_BODY()

    FRigTransformElement* ParentElement = nullptr;
    FRigElementWeight Weight;
    FRigElementWeight InitialWeight;
    FName DisplayLabel;
    bool bCacheIsDirty = true;
};

USTRUCT()
struct FRigMultiParentElement : public FRigTransformElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigMultiParentElement)

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) override;

    TArray<FRigElementParentConstraint> ParentConstraints;
    TMap<FRigElementKey, int32> IndexLookup;
};

USTRUCT()
struct FRigBoneElement : public FRigSingleParentElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigBoneElement)

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) override;

    UPROPERTY()
    ERigBoneType BoneType = ERigBoneType::User;
};

USTRUCT()
struct FRigNullElement final : public FRigMultiParentElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigNullElement)
};

USTRUCT()
struct FRigElementKeyWithLabel {
    GENERATED_BODY()

    FRigElementKeyWithLabel() = default;
    explicit FRigElementKeyWithLabel(const FRigElementKey& InKey, const FName& InLabel = FName()) : Key(InKey), Label(InLabel) {}

    UPROPERTY()
    FRigElementKey Key;
    UPROPERTY()
    FName Label;

    friend FArchive& operator<<(FArchive& Ar, FRigElementKeyWithLabel& KeyWithLabel) {
        Ar << KeyWithLabel.Key;
        Ar << KeyWithLabel.Label;
        return Ar;
    }
};

USTRUCT()
struct FRigControlElementCustomization {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FRigElementKeyWithLabel> AvailableSpaces;
    UPROPERTY()
    TArray<FRigElementKey> RemovedSpaces;
};

UENUM()
enum class ERigControlTransformChannel : uint8 { TranslationX, TranslationY, TranslationZ, Pitch, Yaw, Roll, ScaleX, ScaleY, ScaleZ };

USTRUCT()
struct FRigControlSettings {
    GENERATED_BODY()

    void Load(FArchive& Ar);
    void SetupLimitArrayForType(bool bLimitTranslation = false, bool bLimitRotation = false, bool bLimitScale = false);
    bool IsAnimatable() const {
        return AnimationType == ERigControlAnimationType::AnimationControl || AnimationType == ERigControlAnimationType::AnimationChannel;
    }
    void SetAnimationTypeFromDeprecatedData(bool bAnimatable, bool bShapeEnabled);

    UPROPERTY()
    ERigControlAnimationType AnimationType = ERigControlAnimationType::AnimationControl;
    UPROPERTY()
    ERigControlType ControlType = ERigControlType::EulerTransform;
    UPROPERTY()
    FName DisplayName;
    UPROPERTY()
    ERigControlAxis PrimaryAxis = ERigControlAxis::X;
    UPROPERTY()
    bool bIsCurve = false;
    UPROPERTY()
    TArray<FRigControlLimitEnabled> LimitEnabled;
    UPROPERTY()
    bool bDrawLimits = true;
    UPROPERTY()
    FRigControlValue MinimumValue;
    UPROPERTY()
    FRigControlValue MaximumValue;
    //! UE converts these transforms to the minimum and maximum values; the port keeps the stored transforms.
    FTransform MinimumTransform_DEPRECATED;
    FTransform MaximumTransform_DEPRECATED;
    UPROPERTY()
    bool bShapeVisible = true;
    UPROPERTY()
    ERigControlVisibility ShapeVisibility = ERigControlVisibility::UserDefined;
    UPROPERTY()
    FName ShapeName;
    UPROPERTY()
    FLinearColor ShapeColor = FLinearColor::Red;
    UPROPERTY()
    bool bIsTransientControl = false;
    //! UE loads the enum from this path; the port keeps the path.
    FString ControlEnumPathName;
    UPROPERTY()
    FRigControlElementCustomization Customization;
    UPROPERTY()
    TArray<FRigElementKey> DrivenControls;
    TArray<FRigElementKey> PreviouslyDrivenControls;
    UPROPERTY()
    bool bGroupWithParentControl = false;
    UPROPERTY()
    bool bRestrictSpaceSwitching = false;
    UPROPERTY()
    TArray<ERigControlTransformChannel> FilteredChannels;
    UPROPERTY()
    EEulerRotationOrder PreferredRotationOrder = FRigPreferredEulerAngles::DefaultRotationOrder;
    UPROPERTY()
    bool bUsePreferredRotationOrder = false;
    FTransform ShapeTransform = FTransform::Identity;
};

USTRUCT()
struct FRigControlElement final : public FRigMultiParentElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigControlElement)

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) override;

    FRigCurrentAndInitialTransform& GetOffsetTransform() { return OffsetStorage; }
    FRigCurrentAndInitialDirtyState& GetOffsetDirtyState() { return OffsetDirtyState; }
    FRigCurrentAndInitialTransform& GetShapeTransform() { return ShapeStorage; }
    FRigCurrentAndInitialDirtyState& GetShapeDirtyState() { return ShapeDirtyState; }

    UPROPERTY()
    FRigControlSettings Settings;
    FRigCurrentAndInitialTransform OffsetStorage;
    FRigCurrentAndInitialDirtyState OffsetDirtyState;
    FRigCurrentAndInitialTransform ShapeStorage;
    FRigCurrentAndInitialDirtyState ShapeDirtyState;
    UPROPERTY()
    FRigPreferredEulerAngles PreferredEulerAngles;
};

USTRUCT()
struct FRigCurveElement final : public FRigBaseElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigCurveElement)

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) override;

    float Get() const { return Value; }
    void Set(float InValue, bool InValueIsSet = true) {
        Value = InValue;
        bIsValueSet = InValueIsSet;
    }

    bool bIsValueSet = true;
    //! UE keeps the value in the shared storage of the hierarchy; the port keeps it in the element.
    float Value = 0.f;
};

USTRUCT()
struct FRigRigidBodySettings {
    GENERATED_BODY()

    void Load(FArchive& Ar);

    UPROPERTY()
    float Mass = 1.f;
};

//! UE 5.3 and 5.4 store a rigid body element in place of the physics element of UE 5.5.
USTRUCT()
struct FRigRigidBodyElement final : public FRigSingleParentElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigRigidBodyElement)

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) override;

    UPROPERTY()
    FRigRigidBodySettings Settings;
};

USTRUCT()
struct FRigPhysicsSolverID {
    GENERATED_BODY()
    UPROPERTY()
    FGuid Guid;

    friend FArchive& operator<<(FArchive& Ar, FRigPhysicsSolverID& ID) {
        Ar << ID.Guid;
        return Ar;
    }
};

USTRUCT()
struct FRigPhysicsSettings {
    GENERATED_BODY()

    void Load(FArchive& Ar);

    UPROPERTY()
    float Mass = 1.f;
};

//! Only UE 5.5 has the physics element; UE 5.6 drops it.
USTRUCT()
struct FRigPhysicsElement final : public FRigSingleParentElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigPhysicsElement)

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) override;

    UPROPERTY()
    FRigPhysicsSolverID Solver;
    UPROPERTY()
    FRigPhysicsSettings Settings;
};

USTRUCT()
struct FRigReferenceElement final : public FRigSingleParentElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigReferenceElement)
};

UENUM()
enum class EConnectorType : uint8 { Primary, Secondary };

USTRUCT()
struct FRigConnectorSettings {
    GENERATED_BODY()

    void Load(FArchive& Ar);

    UPROPERTY()
    FString Description;
    UPROPERTY()
    EConnectorType Type = EConnectorType::Primary;
    UPROPERTY()
    bool bOptional = false;
    UPROPERTY()
    bool bIsArray = false;
    UPROPERTY()
    bool bPostConstruction = false;
    UPROPERTY()
    TArray<FRigConnectionRuleStash> Rules;
};

USTRUCT()
struct FRigConnectorElement final : public FRigBaseElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigConnectorElement)

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& InSettings) override;

    UPROPERTY()
    FRigConnectorSettings Settings;
};

USTRUCT()
struct FRigSocketElement final : public FRigSingleParentElement {
    GENERATED_BODY()
    DECLARE_RIG_ELEMENT_METHODS(FRigSocketElement)
};

}

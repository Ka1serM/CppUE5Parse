#pragma once

#include "ue5parse/runtime/AnimationCore/CommonAnimTypes.h"
#include "ue5parse/runtime/Core/Containers/Array.h"
#include "ue5parse/runtime/Core/Math/UnrealMath.h"

#include "Constraint.generated.h"

// The constraint data of UE's AnimationCore Constraint.h: the per axis filter, the transform and aim descriptions, and
// the weighted constraint data that AnimationCore::SolveConstraints blends.
namespace ue5parse::runtime {

struct FMultiTransformBlendHelper;

USTRUCT()
struct FFilterOptionPerAxis {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bX = true;
    UPROPERTY()
    bool bY = true;
    UPROPERTY()
    bool bZ = true;

    void FilterVector(FVector& Input, const FVector& ResetValue = FVector::ZeroVector) const {
        if (!bX) Input.X = ResetValue.X;
        if (!bY) Input.Y = ResetValue.Y;
        if (!bZ) Input.Z = ResetValue.Z;
    }
    void FilterQuat(FQuat& Input, const FQuat& ResetValue = FQuat::Identity) const {
        FRotator Rotator = Input.Rotator();
        FilterRotator(Rotator, ResetValue.Rotator());
        Input = Rotator.Quaternion();
    }
    void FilterRotator(FRotator& Input, const FRotator& ResetValue = FRotator::ZeroRotator) const {
        if (!bX) Input.Roll = ResetValue.Roll;
        if (!bY) Input.Pitch = ResetValue.Pitch;
        if (!bZ) Input.Yaw = ResetValue.Yaw;
    }
    bool IsValid() const { return bX || bY || bZ; }
    bool HasNoEffect() const { return bX && bY && bZ; }
};

enum class EConstraintType : uint8 { Transform, Aim, MAX };

enum class ETransformConstraintType : uint8 { Translation, Rotation, Scale, Parent, LookAt };

struct FConstraintDescriptionEx {
    FFilterOptionPerAxis AxesFilterOption;

    virtual ~FConstraintDescriptionEx() = default;
    virtual void AccumulateConstraintTransform(const FTransform& TargetTransform, const FTransform& CurrentTransform,
                                               const FTransform& CurrentParentTransform, float Weight,
                                               FMultiTransformBlendHelper& BlendHelperInLocalSpace) const = 0;
    virtual bool DoesAffectRotation() const { return false; }
    virtual bool DoesAffectTranslation() const { return false; }
    virtual bool DoesAffectScale() const { return false; }
    virtual bool DoesAffectTransform() const { return false; }
};

struct FTransformConstraintDescription : public FConstraintDescriptionEx {
    ETransformConstraintType TransformType;

    FTransformConstraintDescription(const ETransformConstraintType InType = ETransformConstraintType::Translation) : TransformType(InType) {}
    void AccumulateConstraintTransform(const FTransform& TargetTransform, const FTransform& CurrentTransform, const FTransform& CurrentParentTransform,
                                       float Weight, FMultiTransformBlendHelper& BlendHelperInLocalSpace) const override;
    bool DoesAffectRotation() const override { return TransformType == ETransformConstraintType::Rotation; }
    bool DoesAffectTranslation() const override { return TransformType == ETransformConstraintType::Translation; }
    bool DoesAffectScale() const override { return TransformType == ETransformConstraintType::Scale; }
    bool DoesAffectTransform() const override { return TransformType == ETransformConstraintType::Parent; }
};

struct FAimConstraintDescription : public FConstraintDescriptionEx {
    FAxis LookAt_Axis;
    FAxis LookUp_Axis;
    bool bUseLookUp = false;
    FVector LookUpTarget = FVector::ZeroVector;

    FAimConstraintDescription() { LookUp_Axis.Axis = FVector::UpVector; }
    void AccumulateConstraintTransform(const FTransform& TargetTransform, const FTransform& CurrentTransform, const FTransform& CurrentParentTransform,
                                       float Weight, FMultiTransformBlendHelper& BlendHelperInLocalSpace) const override;
    bool DoesAffectRotation() const override { return true; }
};

// Owns a copy of a transform or an aim description.
struct FConstraintDescriptor {
    EConstraintType Type = EConstraintType::MAX;
    FConstraintDescriptionEx* ConstraintDescription = nullptr;

    FConstraintDescriptor() = default;
    FConstraintDescriptor(const FTransformConstraintDescription& InT) : Type(EConstraintType::Transform) { Set(InT); }
    FConstraintDescriptor(const FAimConstraintDescription& InA) : Type(EConstraintType::Aim) { Set(InA); }
    FConstraintDescriptor(const FConstraintDescriptor& InOther) { *this = InOther; }
    FConstraintDescriptor& operator=(const FConstraintDescriptor& Other) {
        if (this == &Other) return *this;
        Clear();
        Type = Other.Type;
        if (Other.IsValid()) {
            if (Other.Type == EConstraintType::Transform) Set(*Other.GetTypedConstraint<FTransformConstraintDescription>());
            else if (Other.Type == EConstraintType::Aim) Set(*Other.GetTypedConstraint<FAimConstraintDescription>());
        }
        return *this;
    }
    ~FConstraintDescriptor() { Clear(); }

    template <typename T>
    T* GetTypedConstraint() const {
        return static_cast<T*>(ConstraintDescription);
    }
    bool IsValid() const { return ConstraintDescription != nullptr; }
    bool DoesAffectRotation() const { return ConstraintDescription && ConstraintDescription->DoesAffectRotation(); }
    bool DoesAffectTranslation() const { return ConstraintDescription && ConstraintDescription->DoesAffectTranslation(); }
    bool DoesAffectScale() const { return ConstraintDescription && ConstraintDescription->DoesAffectScale(); }
    bool DoesAffectTransform() const { return ConstraintDescription && ConstraintDescription->DoesAffectTransform(); }
    void ApplyConstraintTransform(const FTransform& TargetTransform, const FTransform& CurrentTransform, const FTransform& CurrentParentTransform,
                                  float Weight, FMultiTransformBlendHelper& BlendHelperInLocalSpace) const {
        if (ConstraintDescription)
            ConstraintDescription->AccumulateConstraintTransform(TargetTransform, CurrentTransform, CurrentParentTransform, Weight, BlendHelperInLocalSpace);
    }

private:
    void Set(const FAimConstraintDescription& InA) {
        Clear();
        ConstraintDescription = new FAimConstraintDescription(InA);
    }
    void Set(const FTransformConstraintDescription& InT) {
        Clear();
        ConstraintDescription = new FTransformConstraintDescription(InT);
    }
    void Clear() {
        delete ConstraintDescription;
        ConstraintDescription = nullptr;
    }
};

struct FConstraintData {
    FConstraintDescriptor Constraint;
    float Weight = 1.f;
    bool bMaintainOffset = true;
    FTransform Offset = FTransform::Identity;
    FTransform CurrentTransform;

    FConstraintData() = default;
    FConstraintData(const FTransformConstraintDescription& InTrans, FName InTargetNode = NAME_None, float InWeight = 1.f, bool bInMaintainOffset = true,
                    const FTransform& InOffset = FTransform::Identity)
        : Constraint(InTrans), Weight(InWeight), bMaintainOffset(bInMaintainOffset), Offset(InOffset) {}
    FConstraintData(const FAimConstraintDescription& InAim, FName InTargetNode = NAME_None, float InWeight = 1.f, bool bInMaintainOffset = true,
                    const FTransform& InOffset = FTransform::Identity)
        : Constraint(InAim), Weight(InWeight), bMaintainOffset(bInMaintainOffset), Offset(InOffset) {}

    void ApplyInverseOffset(const FTransform& InTarget, FTransform& OutSource, const FTransform& InBaseTransform) const;
    void SaveInverseOffset(const FTransform& Source, const FTransform& Target, const FTransform& InBaseTransform);
    void ResetOffset() { Offset = FTransform::Identity; }
    void ApplyConstraintTransform(const FTransform& TargetTransform, const FTransform& InCurrentTransform, const FTransform& CurrentParentTransform,
                                  FMultiTransformBlendHelper& BlendHelperInLocalSpace) const;
};

namespace AnimationCore {

// The current transform with the weighted constraints applied in its parent's space.
FTransform SolveConstraints(const FTransform& CurrentTransform, const FTransform& CurrentParentTransform, const TArray<FConstraintData>& Constraints);

}

}

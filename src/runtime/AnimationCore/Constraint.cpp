#include "ue5parse/runtime/AnimationCore/Constraint.h"

#include "ue5parse/runtime/AnimationCore/AnimationCoreLibrary.h"

namespace ue5parse::runtime {

// UE's AnimationCoreUtil.h: the weighted sums of the parent transforms, translations, rotations and scales.
struct FMultiTransformBlendHelper {
    void AddParent(const FTransform& InTransform, float Weight) {
        Transforms.Add(InTransform);
        ParentWeights.Add(Weight);
    }
    void AddTranslation(const FVector& Translation, float Weight) {
        Translations.Add(Translation);
        TranslationWeights.Add(Weight);
    }
    void AddRotation(const FQuat& Rotation, float Weight) {
        Rotations.Add(Rotation);
        RotationWeights.Add(Weight);
    }
    void AddScale(const FVector& Scale, float Weight) {
        Scales.Add(Scale);
        ScaleWeights.Add(Weight);
    }
    bool GetBlendedParent(FTransform& OutTransform) {
        if (Transforms.Num() == 0) return false;
        const float TotalWeight = GetTotalWeight(ParentWeights);
        if (TotalWeight <= ZERO_ANIMWEIGHT_THRESH) return false;
        const float MultiplyWeight = TotalWeight > 1.f ? 1.f / TotalWeight : 1.f;
        OutTransform = Transforms[0] * (ParentWeights[0] * MultiplyWeight);
        for (int32 Index = 1; Index < Transforms.Num(); ++Index)
            OutTransform.AccumulateWithShortestRotation(Transforms[Index], ParentWeights[Index] * MultiplyWeight);
        OutTransform.NormalizeRotation();
        return true;
    }
    bool GetBlendedTranslation(FVector& Output) {
        if (Translations.Num() == 0) return false;
        const float TotalWeight = GetTotalWeight(TranslationWeights);
        if (TotalWeight <= ZERO_ANIMWEIGHT_THRESH) return false;
        const float MultiplyWeight = TotalWeight > 1.f ? 1.f / TotalWeight : 1.f;
        Output = Translations[0] * (TranslationWeights[0] * MultiplyWeight);
        for (int32 Index = 1; Index < Translations.Num(); ++Index) Output += Translations[Index] * (TranslationWeights[Index] * MultiplyWeight);
        return true;
    }
    bool GetBlendedRotation(FQuat& Output) {
        if (Rotations.Num() == 0) return false;
        const float TotalWeight = GetTotalWeight(RotationWeights);
        if (TotalWeight <= ZERO_ANIMWEIGHT_THRESH) return false;
        const float MultiplyWeight = TotalWeight > 1.f ? 1.f / TotalWeight : 1.f;
        Output = Rotations[0] * (RotationWeights[0] * MultiplyWeight);
        for (int32 Index = 1; Index < Rotations.Num(); ++Index) {
            const FQuat BlendRotation = Rotations[Index] * (RotationWeights[Index] * MultiplyWeight);
            if ((Output | BlendRotation) < 0) {
                Output.X -= BlendRotation.X;
                Output.Y -= BlendRotation.Y;
                Output.Z -= BlendRotation.Z;
                Output.W -= BlendRotation.W;
            } else {
                Output.X += BlendRotation.X;
                Output.Y += BlendRotation.Y;
                Output.Z += BlendRotation.Z;
                Output.W += BlendRotation.W;
            }
        }
        Output.Normalize();
        return true;
    }
    bool GetBlendedScale(FVector& Output) {
        if (Scales.Num() == 0) return false;
        const float TotalWeight = GetTotalWeight(ScaleWeights);
        if (TotalWeight <= ZERO_ANIMWEIGHT_THRESH) return false;
        const float MultiplyWeight = TotalWeight > 1.f ? 1.f / TotalWeight : 1.f;
        Output = FVector::ZeroVector;
        for (int32 Index = 0; Index < Scales.Num(); ++Index) Output += Scales[Index] * (ScaleWeights[Index] * MultiplyWeight);
        return true;
    }

private:
    TArray<FTransform> Transforms;
    TArray<FVector> Translations;
    TArray<FQuat> Rotations;
    TArray<FVector> Scales;
    TArray<float> ParentWeights;
    TArray<float> TranslationWeights;
    TArray<float> RotationWeights;
    TArray<float> ScaleWeights;

    static float GetTotalWeight(const TArray<float>& Weights) {
        float TotalWeight = 0.f;
        for (float Weight : Weights) TotalWeight += Weight;
        return TotalWeight;
    }
};

// The offset is (Source - Target) - BaseTransform in component space; applying it gives
// Offset + NewBaseTransform + NewTargetTransform = NewSourceTransform.
void FConstraintData::ApplyInverseOffset(const FTransform& InTarget, FTransform& OutSource, const FTransform& InBaseTransform) const {
    if (!bMaintainOffset) {
        OutSource = InTarget;
        return;
    }
    if (Constraint.DoesAffectTransform()) {
        OutSource = (Offset * InBaseTransform) * InTarget;
        return;
    }
    if (Constraint.DoesAffectTranslation())
        OutSource.SetTranslation(InTarget.GetTranslation() + InBaseTransform.TransformVectorNoScale(Offset.GetTranslation()));
    if (Constraint.DoesAffectRotation()) {
        OutSource.SetRotation(InTarget.GetRotation() * InBaseTransform.GetRotation() * Offset.GetRotation());
        OutSource.NormalizeRotation();
    }
    if (Constraint.DoesAffectScale()) OutSource.SetScale3D(InTarget.GetScale3D() * InBaseTransform.GetScale3D() * Offset.GetScale3D());
}

void FConstraintData::SaveInverseOffset(const FTransform& Source, const FTransform& Target, const FTransform& InBaseTransform) {
    ResetOffset();
    if (!bMaintainOffset) return;
    if (Constraint.DoesAffectTransform()) {
        const FTransform ToSource = Source.GetRelativeTransform(Target);
        Offset = ToSource.GetRelativeTransform(InBaseTransform);
        return;
    }
    if (Constraint.DoesAffectTranslation()) {
        const FVector DeltaLocation = Source.GetTranslation() - Target.GetTranslation();
        Offset.SetLocation(InBaseTransform.InverseTransformVectorNoScale(DeltaLocation));
    }
    if (Constraint.DoesAffectRotation()) {
        const FQuat DeltaRotation = Target.GetRotation().Inverse() * Source.GetRotation();
        Offset.SetRotation(InBaseTransform.GetRotation().Inverse() * DeltaRotation);
        Offset.NormalizeRotation();
    }
    if (Constraint.DoesAffectScale()) {
        const FVector RecipTarget = FTransform::GetSafeScaleReciprocal(Target.GetScale3D());
        const FVector DeltaScale = Source.GetScale3D() * RecipTarget;
        const FVector RecipBase = FTransform::GetSafeScaleReciprocal(InBaseTransform.GetScale3D());
        Offset.SetScale3D(DeltaScale * RecipBase);
    }
}

void FConstraintData::ApplyConstraintTransform(const FTransform& TargetTransform, const FTransform& InCurrentTransform,
                                               const FTransform& CurrentParentTransform, FMultiTransformBlendHelper& BlendHelperInLocalSpace) const {
    FTransform OffsetTargetTransform;
    ApplyInverseOffset(TargetTransform, OffsetTargetTransform, CurrentParentTransform);
    Constraint.ApplyConstraintTransform(OffsetTargetTransform, InCurrentTransform, CurrentParentTransform, Weight, BlendHelperInLocalSpace);
}

void FTransformConstraintDescription::AccumulateConstraintTransform(const FTransform& TargetTransform, const FTransform& CurrentTransform,
                                                                    const FTransform& CurrentParentTransform, float Weight,
                                                                    FMultiTransformBlendHelper& BlendHelperInLocalSpace) const {
    const FTransform TargetLocalTransform = TargetTransform.GetRelativeTransform(CurrentParentTransform);
    if (DoesAffectTransform()) {
        BlendHelperInLocalSpace.AddParent(TargetLocalTransform, Weight);
        return;
    }
    if (DoesAffectTranslation()) {
        FVector Translation = TargetLocalTransform.GetTranslation();
        AxesFilterOption.FilterVector(Translation);
        BlendHelperInLocalSpace.AddTranslation(Translation, Weight);
    }
    if (DoesAffectRotation()) {
        FQuat DeltaRotation = TargetLocalTransform.GetRotation();
        AxesFilterOption.FilterQuat(DeltaRotation);
        BlendHelperInLocalSpace.AddRotation(DeltaRotation, Weight);
    }
    if (DoesAffectScale()) {
        FVector Scale = TargetLocalTransform.GetScale3D();
        AxesFilterOption.FilterVector(Scale);
        BlendHelperInLocalSpace.AddScale(Scale, Weight);
    }
}

void FAimConstraintDescription::AccumulateConstraintTransform(const FTransform& TargetTransform, const FTransform& CurrentTransform,
                                                              const FTransform& CurrentParentTransform, float Weight,
                                                              FMultiTransformBlendHelper& BlendHelperInLocalSpace) const {
    FTransform NewTransform = CurrentTransform;
    if (bUseLookUp) {
        const FQuat DeltaRotation =
            AnimationCore::SolveAim(NewTransform, LookUpTarget, LookUp_Axis.GetTransformedAxis(NewTransform), false, FVector::ZeroVector);
        NewTransform.SetRotation(DeltaRotation * NewTransform.GetRotation());
    }
    const FQuat DeltaRotation =
        AnimationCore::SolveAim(NewTransform, TargetTransform.GetLocation(), LookAt_Axis.GetTransformedAxis(NewTransform), false, FVector::ZeroVector);
    NewTransform.SetRotation(DeltaRotation * NewTransform.GetRotation());
    const FTransform LocalTransform = NewTransform.GetRelativeTransform(CurrentParentTransform);
    FQuat LocalRotation = LocalTransform.GetRotation();
    AxesFilterOption.FilterQuat(LocalRotation);
    BlendHelperInLocalSpace.AddRotation(LocalRotation, Weight);
}

namespace AnimationCore {

FTransform SolveConstraints(const FTransform& CurrentTransform, const FTransform& CurrentParentTransform, const TArray<FConstraintData>& Constraints) {
    FMultiTransformBlendHelper BlendHelperInLocalSpace;
    FTransform BlendedLocalTransform = CurrentTransform.GetRelativeTransform(CurrentParentTransform);
    for (const FConstraintData& Constraint : Constraints) {
        if (Constraint.Weight <= ZERO_ANIMWEIGHT_THRESH) continue;
        // In the parent's space, which keeps the hierarchy.
        Constraint.ApplyConstraintTransform(Constraint.CurrentTransform, CurrentTransform, CurrentParentTransform, BlendHelperInLocalSpace);
    }
    // A parent constraint does not combine with the others.
    FTransform ParentTransform;
    if (BlendHelperInLocalSpace.GetBlendedParent(ParentTransform)) {
        BlendedLocalTransform = ParentTransform;
    } else {
        FVector BlendedTranslation;
        if (BlendHelperInLocalSpace.GetBlendedTranslation(BlendedTranslation)) BlendedLocalTransform.SetTranslation(BlendedTranslation);
        FQuat BlendedRotation;
        if (BlendHelperInLocalSpace.GetBlendedRotation(BlendedRotation)) BlendedLocalTransform.SetRotation(BlendedRotation);
        FVector BlendedScale;
        if (BlendHelperInLocalSpace.GetBlendedScale(BlendedScale)) BlendedLocalTransform.SetScale3D(BlendedScale);
    }
    return BlendedLocalTransform * CurrentParentTransform;
}

}

}

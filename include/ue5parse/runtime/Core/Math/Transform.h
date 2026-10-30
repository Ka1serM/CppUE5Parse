#pragma once

#include "ue5parse/runtime/Core/Math/Matrix.h"

namespace ue5parse::runtime {

// TTransform: rotation, translation and 3D scale, as UE's TransformNonVectorized.h. A * B applies A first, then B.
template <class T>
struct TTransform {
    TQuat<T> Rotation;
    TVector<T> Translation;
    TVector<T> Scale3D{1, 1, 1};

    constexpr TTransform() = default;
    explicit TTransform(const TVector<T>& InTranslation) : Translation(InTranslation) {}
    explicit TTransform(const TQuat<T>& InRotation) : Rotation(InRotation) {}
    explicit TTransform(const TRotator<T>& InRotation) : Rotation(InRotation.Quaternion()) {}
    TTransform(const TQuat<T>& InRotation, const TVector<T>& InTranslation, const TVector<T>& InScale3D = TVector<T>(1))
        : Rotation(InRotation), Translation(InTranslation), Scale3D(InScale3D) {}
    TTransform(const TRotator<T>& InRotation, const TVector<T>& InTranslation, const TVector<T>& InScale3D = TVector<T>(1))
        : Rotation(InRotation.Quaternion()), Translation(InTranslation), Scale3D(InScale3D) {}
    explicit TTransform(const TMatrix<T>& InMatrix) { SetFromMatrix(InMatrix); }
    template <class U>
    explicit TTransform(const TTransform<U>& Other) : Rotation(Other.Rotation), Translation(Other.Translation), Scale3D(Other.Scale3D) {}

    static const TTransform Identity;

    // The transform that applies this one, then Other.
    TTransform operator*(const TTransform& Other) const {
        TTransform Output;
        Multiply(&Output, this, &Other);
        return Output;
    }
    TTransform& operator*=(const TTransform& Other) {
        Multiply(this, this, &Other);
        return *this;
    }
    TTransform operator*(const TQuat<T>& Other) const {
        TTransform Output;
        Output.Rotation = Other * Rotation;
        Output.Scale3D = Scale3D;
        Output.Translation = Other.RotateVector(Translation);
        return Output;
    }
    // Every component times the weight (UE's operator*(ScalarRegister)), for blending.
    TTransform operator*(T Mult) const { return TTransform(Rotation * Mult, Translation * Mult, Scale3D * Mult); }
    TTransform& operator*=(T Mult) {
        Translation *= Mult;
        Rotation *= Mult;
        Scale3D *= Mult;
        return *this;
    }
    TTransform operator+(const TTransform& Atom) const {
        return TTransform(Rotation + Atom.Rotation, Translation + Atom.Translation, Scale3D + Atom.Scale3D);
    }
    TTransform& operator+=(const TTransform& Atom) {
        Translation += Atom.Translation;
        Rotation.X += Atom.Rotation.X;
        Rotation.Y += Atom.Rotation.Y;
        Rotation.Z += Atom.Rotation.Z;
        Rotation.W += Atom.Rotation.W;
        Scale3D += Atom.Scale3D;
        return *this;
    }

    static void Multiply(TTransform* OutTransform, const TTransform* A, const TTransform* B) {
        // The scalar path of UE's Multiply (no special case for negative scale).
        const TQuat<T> QuatA = A->Rotation;
        const TQuat<T> QuatB = B->Rotation;
        const TVector<T> TranslateA = A->Translation;
        const TVector<T> TranslateB = B->Translation;
        const TVector<T> ScaleA = A->Scale3D;
        const TVector<T> ScaleB = B->Scale3D;
        OutTransform->Rotation = QuatB * QuatA;
        OutTransform->Scale3D = ScaleA * ScaleB;
        OutTransform->Translation = QuatB.RotateVector(ScaleB * TranslateA) + TranslateB;
    }

    TTransform Inverse() const {
        const TVector<T> InvScale3D = GetSafeScaleReciprocal(Scale3D);
        const TQuat<T> InvRotation = Rotation.Inverse();
        const TVector<T> ScaledTranslation = InvScale3D * Translation;
        const TVector<T> T2 = InvRotation.RotateVector(ScaledTranslation);
        return TTransform(InvRotation, -T2, InvScale3D);
    }
    // This transform relative to Other: this * Other.Inverse().
    TTransform GetRelativeTransform(const TTransform& Other) const;
    // Other relative to this: this.Inverse() * Other.
    TTransform GetRelativeTransformReverse(const TTransform& Other) const;
    void SetToRelativeTransform(const TTransform& ParentTransform) { *this = GetRelativeTransform(ParentTransform); }

    TVector<T> TransformPosition(const TVector<T>& V) const { return Rotation.RotateVector(Scale3D * V) + Translation; }
    TVector<T> TransformPositionNoScale(const TVector<T>& V) const { return Rotation.RotateVector(V) + Translation; }
    TVector<T> TransformVector(const TVector<T>& V) const { return Rotation.RotateVector(Scale3D * V); }
    TVector<T> TransformVectorNoScale(const TVector<T>& V) const { return Rotation.RotateVector(V); }
    TVector<T> InverseTransformPosition(const TVector<T>& V) const {
        return Rotation.UnrotateVector(V - Translation) * GetSafeScaleReciprocal(Scale3D);
    }
    TVector<T> InverseTransformPositionNoScale(const TVector<T>& V) const { return Rotation.UnrotateVector(V - Translation); }
    TVector<T> InverseTransformVector(const TVector<T>& V) const { return Rotation.UnrotateVector(V) * GetSafeScaleReciprocal(Scale3D); }
    TVector<T> InverseTransformVectorNoScale(const TVector<T>& V) const { return Rotation.UnrotateVector(V); }
    TQuat<T> TransformRotation(const TQuat<T>& Q) const { return Rotation * Q; }
    TQuat<T> InverseTransformRotation(const TQuat<T>& Q) const { return Rotation.Inverse() * Q; }

    TMatrix<T> ToMatrixWithScale() const;
    TMatrix<T> ToMatrixNoScale() const;
    TMatrix<T> ToInverseMatrixWithScale() const { return ToMatrixWithScale().Inverse(); }
    void SetFromMatrix(const TMatrix<T>& InMatrix);

    TVector<T> GetLocation() const { return Translation; }
    TRotator<T> Rotator() const { return Rotation.Rotator(); }
    const TQuat<T>& GetRotation() const { return Rotation; }
    const TVector<T>& GetTranslation() const { return Translation; }
    const TVector<T>& GetScale3D() const { return Scale3D; }
    void SetLocation(const TVector<T>& V) { Translation = V; }
    void SetRotation(const TQuat<T>& Q) { Rotation = Q; }
    void SetTranslation(const TVector<T>& V) { Translation = V; }
    void SetScale3D(const TVector<T>& V) { Scale3D = V; }
    void SetComponents(const TQuat<T>& InRotation, const TVector<T>& InTranslation, const TVector<T>& InScale3D) {
        Rotation = InRotation;
        Translation = InTranslation;
        Scale3D = InScale3D;
    }
    void SetIdentity() { *this = TTransform(); }
    // The identity of an additive pose: no rotation, translation or scale delta (UE's SetIdentityZeroScale).
    void SetIdentityZeroScale() {
        Rotation = TQuat<T>::Identity;
        Translation = TVector<T>(0);
        Scale3D = TVector<T>(0);
    }
    void SetTranslationAndScale3D(const TVector<T>& NewTranslation, const TVector<T>& NewScale3D) {
        Translation = NewTranslation;
        Scale3D = NewScale3D;
    }
    void AddToTranslation(const TVector<T>& Delta) { Translation += Delta; }
    void ConcatenateRotation(const TQuat<T>& DeltaRotation) { Rotation = Rotation * DeltaRotation; }
    void MultiplyScale3D(const TVector<T>& Scale3DMultiplier) { Scale3D *= Scale3DMultiplier; }
    void ScaleTranslation(const TVector<T>& InScale3D) { Translation *= InScale3D; }
    void ScaleTranslation(T Scale) { Translation *= Scale; }
    void RemoveScaling(T Tolerance = T(UE_SMALL_NUMBER)) {
        Scale3D = TVector<T>(1);
        Rotation.Normalize();
    }
    T GetMaximumAxisScale() const { return Scale3D.GetAbsMax(); }
    T GetMinimumAxisScale() const { return Scale3D.GetAbsMin(); }
    TVector<T> GetScaledAxis(int32 Axis) const;
    TVector<T> GetUnitAxis(int32 Axis) const;
    void Mirror(int32 MirrorAxis, int32 FlipAxis);
    void NormalizeRotation() { Rotation.Normalize(); }
    bool IsRotationNormalized() const { return Rotation.IsNormalized(); }
    bool ContainsNaN() const { return Rotation.ContainsNaN() || Translation.ContainsNaN() || Scale3D.ContainsNaN(); }
    bool IsValid() const { return !ContainsNaN() && Rotation.IsNormalized(); }

    bool RotationEquals(const TTransform& Other, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const { return Rotation.Equals(Other.Rotation, Tolerance); }
    bool TranslationEquals(const TTransform& Other, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return Translation.Equals(Other.Translation, Tolerance);
    }
    bool Scale3DEquals(const TTransform& Other, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const { return Scale3D.Equals(Other.Scale3D, Tolerance); }
    bool Equals(const TTransform& Other, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return RotationEquals(Other, Tolerance) && TranslationEquals(Other, Tolerance) && Scale3DEquals(Other, Tolerance);
    }
    bool EqualsNoScale(const TTransform& Other, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return RotationEquals(Other, Tolerance) && TranslationEquals(Other, Tolerance);
    }
    static bool AreRotationsEqual(const TTransform& A, const TTransform& B, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) {
        return A.Rotation.Equals(B.Rotation, Tolerance);
    }
    static bool AreTranslationsEqual(const TTransform& A, const TTransform& B, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) {
        return A.Translation.Equals(B.Translation, Tolerance);
    }
    static bool AreScale3DsEqual(const TTransform& A, const TTransform& B, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) {
        return A.Scale3D.Equals(B.Scale3D, Tolerance);
    }

    // The blending operations of the animation system.
    void Blend(const TTransform& Atom1, const TTransform& Atom2, float Alpha) {
        if (Alpha <= ZERO_ANIMWEIGHT_THRESH) {
            *this = Atom1;
        } else if (Alpha >= 1.f - ZERO_ANIMWEIGHT_THRESH) {
            *this = Atom2;
        } else {
            Translation = FMath::Lerp(Atom1.Translation, Atom2.Translation, T(Alpha));
            Scale3D = FMath::Lerp(Atom1.Scale3D, Atom2.Scale3D, T(Alpha));
            Rotation = TQuat<T>::FastLerp(Atom1.Rotation, Atom2.Rotation, T(Alpha));
            Rotation.Normalize();
        }
    }
    void BlendWith(const TTransform& OtherAtom, float Alpha) {
        if (Alpha > ZERO_ANIMWEIGHT_THRESH) {
            if (Alpha >= 1.f - ZERO_ANIMWEIGHT_THRESH) {
                *this = OtherAtom;
            } else {
                Translation = FMath::Lerp(Translation, OtherAtom.Translation, T(Alpha));
                Scale3D = FMath::Lerp(Scale3D, OtherAtom.Scale3D, T(Alpha));
                Rotation = TQuat<T>::FastLerp(Rotation, OtherAtom.Rotation, T(Alpha));
                Rotation.Normalize();
            }
        }
    }
    // Adds an additive delta (UE's Accumulate(SourceAtom)).
    void Accumulate(const TTransform& SourceAtom) {
        if (FMath::Square(SourceAtom.Rotation.W) < T(1) - T(UE_DELTA) * T(UE_DELTA)) Rotation = SourceAtom.Rotation * Rotation;
        Translation += SourceAtom.Translation;
        Scale3D *= SourceAtom.Scale3D;
    }
    void Accumulate(const TTransform& Atom, float BlendWeight) {
        TTransform SourceAtom(Atom * T(BlendWeight));
        if (FMath::Square(SourceAtom.Rotation.W) < T(1) - T(UE_DELTA) * T(UE_DELTA)) Rotation = SourceAtom.Rotation * Rotation;
        Translation += SourceAtom.Translation;
        Scale3D *= SourceAtom.Scale3D;
    }
    // Adds a weighted atom to a weighted sum of atoms, flipping the rotation for the shorter arc.
    void AccumulateWithShortestRotation(const TTransform& DeltaAtom, float BlendWeight) {
        TTransform Atom(DeltaAtom * T(BlendWeight));
        if ((Atom.Rotation | Rotation) < T(0)) {
            Rotation.X -= Atom.Rotation.X;
            Rotation.Y -= Atom.Rotation.Y;
            Rotation.Z -= Atom.Rotation.Z;
            Rotation.W -= Atom.Rotation.W;
        } else {
            Rotation.X += Atom.Rotation.X;
            Rotation.Y += Atom.Rotation.Y;
            Rotation.Z += Atom.Rotation.Z;
            Rotation.W += Atom.Rotation.W;
        }
        Translation += Atom.Translation;
        Scale3D += Atom.Scale3D;
    }
    void AccumulateWithAdditiveScale(const TTransform& Atom, float BlendWeight) {
        const TVector<T> DefaultScale(1);
        TTransform SourceAtom(Atom * T(BlendWeight));
        if (FMath::Square(SourceAtom.Rotation.W) < T(1) - T(UE_DELTA) * T(UE_DELTA)) Rotation = SourceAtom.Rotation * Rotation;
        Translation += SourceAtom.Translation;
        Scale3D *= (DefaultScale + SourceAtom.Scale3D);
    }
    void LerpTranslationScale3D(const TTransform& SourceAtom1, const TTransform& SourceAtom2, float Alpha) {
        Translation = FMath::Lerp(SourceAtom1.Translation, SourceAtom2.Translation, T(Alpha));
        Scale3D = FMath::Lerp(SourceAtom1.Scale3D, SourceAtom2.Scale3D, T(Alpha));
    }
    // UE's BlendFromIdentityAndAccumulate: FinalAtom += SourceAtom (an additive delta with scale stored minus one)
    // scaled by BlendWeight.
    static void BlendFromIdentityAndAccumulate(TTransform& FinalAtom, const TTransform& SourceAtom, float BlendWeight) {
        const TTransform AdditiveIdentity(TQuat<T>::Identity, TVector<T>(0), TVector<T>(0));
        TTransform DeltaAtom = SourceAtom;
        if (BlendWeight < 1.f - ZERO_ANIMWEIGHT_THRESH) DeltaAtom.Blend(AdditiveIdentity, SourceAtom, BlendWeight);
        if (FMath::Square(DeltaAtom.Rotation.W) < T(1) - T(UE_DELTA) * T(UE_DELTA))
            FinalAtom.Rotation = DeltaAtom.Rotation * FinalAtom.Rotation;
        FinalAtom.Translation += DeltaAtom.Translation;
        FinalAtom.Scale3D *= (TVector<T>(1) + DeltaAtom.Scale3D);
    }

    static TVector<T> GetSafeScaleReciprocal(const TVector<T>& InScale, T Tolerance = T(UE_SMALL_NUMBER)) {
        TVector<T> SafeReciprocalScale;
        SafeReciprocalScale.X = FMath::Abs(InScale.X) <= Tolerance ? T(0) : T(1) / InScale.X;
        SafeReciprocalScale.Y = FMath::Abs(InScale.Y) <= Tolerance ? T(0) : T(1) / InScale.Y;
        SafeReciprocalScale.Z = FMath::Abs(InScale.Z) <= Tolerance ? T(0) : T(1) / InScale.Z;
        return SafeReciprocalScale;
    }
    static TVector<T> AddTranslations(const TTransform& A, const TTransform& B) { return A.Translation + B.Translation; }
    static TVector<T> SubtractTranslations(const TTransform& A, const TTransform& B) { return A.Translation - B.Translation; }
};

template <class T>
inline const TTransform<T> TTransform<T>::Identity;

template <class T>
TTransform<T> TTransform<T>::GetRelativeTransform(const TTransform& Other) const {
    TTransform Result;
    const TVector<T> SafeRecipScale3D = GetSafeScaleReciprocal(Other.Scale3D, T(UE_SMALL_NUMBER));
    Result.Scale3D = Scale3D * SafeRecipScale3D;
    if (!Other.Rotation.IsNormalized()) return TTransform::Identity;
    const TQuat<T> Inverse = Other.Rotation.Inverse();
    Result.Rotation = Inverse * Rotation;
    Result.Translation = Inverse.RotateVector(Translation - Other.Translation) * SafeRecipScale3D;
    return Result;
}

template <class T>
TTransform<T> TTransform<T>::GetRelativeTransformReverse(const TTransform& Other) const {
    TTransform Result;
    const TVector<T> SafeRecipScale3D = GetSafeScaleReciprocal(Scale3D);
    Result.Scale3D = Other.Scale3D * SafeRecipScale3D;
    Result.Rotation = Rotation.Inverse() * Other.Rotation;
    Result.Translation = Other.Translation - Result.Scale3D * Result.Rotation.RotateVector(Translation);
    return Result;
}

template <class T>
TMatrix<T> TTransform<T>::ToMatrixWithScale() const {
    TMatrix<T> OutMatrix;
    OutMatrix.M[3][0] = Translation.X;
    OutMatrix.M[3][1] = Translation.Y;
    OutMatrix.M[3][2] = Translation.Z;
    const T x2 = Rotation.X + Rotation.X, y2 = Rotation.Y + Rotation.Y, z2 = Rotation.Z + Rotation.Z;
    {
        const T xx2 = Rotation.X * x2, yy2 = Rotation.Y * y2, zz2 = Rotation.Z * z2;
        OutMatrix.M[0][0] = (T(1) - (yy2 + zz2)) * Scale3D.X;
        OutMatrix.M[1][1] = (T(1) - (xx2 + zz2)) * Scale3D.Y;
        OutMatrix.M[2][2] = (T(1) - (xx2 + yy2)) * Scale3D.Z;
    }
    {
        const T yz2 = Rotation.Y * z2, wx2 = Rotation.W * x2;
        OutMatrix.M[2][1] = (yz2 - wx2) * Scale3D.Z;
        OutMatrix.M[1][2] = (yz2 + wx2) * Scale3D.Y;
    }
    {
        const T xy2 = Rotation.X * y2, wz2 = Rotation.W * z2;
        OutMatrix.M[1][0] = (xy2 - wz2) * Scale3D.Y;
        OutMatrix.M[0][1] = (xy2 + wz2) * Scale3D.X;
    }
    {
        const T xz2 = Rotation.X * z2, wy2 = Rotation.W * y2;
        OutMatrix.M[2][0] = (xz2 + wy2) * Scale3D.Z;
        OutMatrix.M[0][2] = (xz2 - wy2) * Scale3D.X;
    }
    OutMatrix.M[0][3] = 0;
    OutMatrix.M[1][3] = 0;
    OutMatrix.M[2][3] = 0;
    OutMatrix.M[3][3] = 1;
    return OutMatrix;
}

template <class T>
TMatrix<T> TTransform<T>::ToMatrixNoScale() const {
    TTransform NoScale(Rotation, Translation, TVector<T>(1));
    return NoScale.ToMatrixWithScale();
}

template <class T>
void TTransform<T>::SetFromMatrix(const TMatrix<T>& InMatrix) {
    TMatrix<T> M = InMatrix;
    Scale3D = M.ExtractScaling();
    if (InMatrix.Determinant() < T(0)) {
        Scale3D.X *= T(-1);
        M.SetAxis(0, -M.GetScaledAxis(0));
    }
    Rotation = M.ToQuat();
    Translation = InMatrix.GetOrigin();
    Rotation.Normalize();
}

template <class T>
TVector<T> TTransform<T>::GetScaledAxis(int32 Axis) const {
    TVector<T> Unit = Axis == 0 ? TVector<T>(1, 0, 0) : (Axis == 1 ? TVector<T>(0, 1, 0) : TVector<T>(0, 0, 1));
    return TransformVector(Unit);
}

template <class T>
TVector<T> TTransform<T>::GetUnitAxis(int32 Axis) const {
    TVector<T> Unit = Axis == 0 ? TVector<T>(1, 0, 0) : (Axis == 1 ? TVector<T>(0, 1, 0) : TVector<T>(0, 0, 1));
    return TransformVectorNoScale(Unit);
}

template <class T>
void TTransform<T>::Mirror(int32 MirrorAxis, int32 FlipAxis) {
    // MirrorAxis and FlipAxis are EAxis values: 1 X, 2 Y, 3 Z.
    TMatrix<T> M = ToMatrixWithScale();
    auto Mirror = [&](int32 Axis) {
        if (Axis < 1 || Axis > 3) return;
        int32 Col = Axis - 1;
        for (int32 Row = 0; Row < 4; Row++) M.M[Row][Col] *= T(-1);
    };
    Mirror(MirrorAxis);
    if (FlipAxis >= 1 && FlipAxis <= 3) M.SetAxis(FlipAxis - 1, -M.GetScaledAxis(FlipAxis - 1));
    SetFromMatrix(M);
}

using FTransform = TTransform<double>;
using FTransform3d = TTransform<double>;
using FTransform3f = TTransform<float>;

}

#pragma once

#include "Engine/Source/Runtime/Core/Math/Quat.h"

namespace ue5parse::runtime {

template <class T>
struct TPlane : TVector<T> {
    T W = 0;
    constexpr TPlane() = default;
    constexpr TPlane(T InX, T InY, T InZ, T InW) : TVector<T>(InX, InY, InZ), W(InW) {}
    TPlane(const TVector<T>& InBase, const TVector<T>& InNormal) : TVector<T>(InNormal), W(InBase | InNormal) {}
    TVector<T> GetNormal() const { return {this->X, this->Y, this->Z}; }
    TVector<T> GetOrigin() const { return GetNormal() * W; }
    constexpr bool operator==(const TPlane& P) const { return this->X == P.X && this->Y == P.Y && this->Z == P.Z && W == P.W; }
};
using FPlane = TPlane<double>;
using FPlane4f = TPlane<float>;

template <class T>
struct alignas(16) TMatrix {
    T M[4][4];

    constexpr TMatrix() : M{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}} {}
    TMatrix(const TPlane<T>& InX, const TPlane<T>& InY, const TPlane<T>& InZ, const TPlane<T>& InW) {
        const TPlane<T>* Rows[4] = {&InX, &InY, &InZ, &InW};
        for (int32 i = 0; i < 4; i++) {
            M[i][0] = Rows[i]->X;
            M[i][1] = Rows[i]->Y;
            M[i][2] = Rows[i]->Z;
            M[i][3] = Rows[i]->W;
        }
    }
    TMatrix(const TVector<T>& InX, const TVector<T>& InY, const TVector<T>& InZ, const TVector<T>& InW) {
        M[0][0] = InX.X; M[0][1] = InX.Y; M[0][2] = InX.Z; M[0][3] = 0;
        M[1][0] = InY.X; M[1][1] = InY.Y; M[1][2] = InY.Z; M[1][3] = 0;
        M[2][0] = InZ.X; M[2][1] = InZ.Y; M[2][2] = InZ.Z; M[2][3] = 0;
        M[3][0] = InW.X; M[3][1] = InW.Y; M[3][2] = InW.Z; M[3][3] = 1;
    }

    static const TMatrix Identity;

    template <class U>
    explicit TMatrix(const TMatrix<U>& Other) {
        for (int32 i = 0; i < 4; i++)
            for (int32 j = 0; j < 4; j++) M[i][j] = T(Other.M[i][j]);
    }
    void SetIdentity() { *this = TMatrix(); }
    TMatrix operator*(const TMatrix& Other) const {
        TMatrix Result;
        for (int32 i = 0; i < 4; i++)
            for (int32 j = 0; j < 4; j++)
                Result.M[i][j] = M[i][0] * Other.M[0][j] + M[i][1] * Other.M[1][j] + M[i][2] * Other.M[2][j] + M[i][3] * Other.M[3][j];
        return Result;
    }
    TMatrix& operator*=(const TMatrix& Other) {
        *this = *this * Other;
        return *this;
    }
    TMatrix operator+(const TMatrix& Other) const {
        TMatrix Result;
        for (int32 i = 0; i < 4; i++)
            for (int32 j = 0; j < 4; j++) Result.M[i][j] = M[i][j] + Other.M[i][j];
        return Result;
    }
    TMatrix operator*(T Other) const {
        TMatrix Result;
        for (int32 i = 0; i < 4; i++)
            for (int32 j = 0; j < 4; j++) Result.M[i][j] = M[i][j] * Other;
        return Result;
    }
    bool operator==(const TMatrix& Other) const {
        for (int32 i = 0; i < 4; i++)
            for (int32 j = 0; j < 4; j++)
                if (M[i][j] != Other.M[i][j]) return false;
        return true;
    }
    bool Equals(const TMatrix& Other, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        for (int32 i = 0; i < 4; i++)
            for (int32 j = 0; j < 4; j++)
                if (FMath::Abs(M[i][j] - Other.M[i][j]) > Tolerance) return false;
        return true;
    }
    TVector4<T> TransformFVector4(const TVector4<T>& P) const {
        return {P.X * M[0][0] + P.Y * M[1][0] + P.Z * M[2][0] + P.W * M[3][0],
                P.X * M[0][1] + P.Y * M[1][1] + P.Z * M[2][1] + P.W * M[3][1],
                P.X * M[0][2] + P.Y * M[1][2] + P.Z * M[2][2] + P.W * M[3][2],
                P.X * M[0][3] + P.Y * M[1][3] + P.Z * M[2][3] + P.W * M[3][3]};
    }
    TVector<T> TransformPosition(const TVector<T>& V) const { return TransformFVector4(TVector4<T>(V, 1)).XYZ(); }
    TVector<T> TransformVector(const TVector<T>& V) const { return TransformFVector4(TVector4<T>(V, 0)).XYZ(); }
    TVector<T> InverseTransformPosition(const TVector<T>& V) const { return InverseFast().TransformPosition(V); }
    TVector<T> InverseTransformVector(const TVector<T>& V) const { return InverseFast().TransformVector(V); }
    T Determinant() const;
    TMatrix InverseFast() const;
    TMatrix Inverse() const;
    void RemoveScaling(T Tolerance = T(UE_SMALL_NUMBER));
    TVector<T> ExtractScaling(T Tolerance = T(UE_SMALL_NUMBER));
    void ScaleTranslation(const TVector<T>& Scale3D) {
        M[3][0] *= Scale3D.X;
        M[3][1] *= Scale3D.Y;
        M[3][2] *= Scale3D.Z;
    }
    TVector<T> GetOrigin() const { return {M[3][0], M[3][1], M[3][2]}; }
    void SetOrigin(const TVector<T>& NewOrigin) {
        M[3][0] = NewOrigin.X;
        M[3][1] = NewOrigin.Y;
        M[3][2] = NewOrigin.Z;
    }
    TVector<T> GetScaledAxis(int32 Axis) const { return {M[Axis][0], M[Axis][1], M[Axis][2]}; }
    TVector<T> GetUnitAxis(int32 Axis) const { return GetScaledAxis(Axis).GetSafeNormal(); }
    void SetAxis(int32 Axis, const TVector<T>& V) {
        M[Axis][0] = V.X;
        M[Axis][1] = V.Y;
        M[Axis][2] = V.Z;
    }
    TRotator<T> Rotator() const;
    TQuat<T> ToQuat() const;
    T GetMaximumAxisScale() const {
        const T MaxRowScaleSquared = FMath::Max3(GetScaledAxis(0).SizeSquared(), GetScaledAxis(1).SizeSquared(), GetScaledAxis(2).SizeSquared());
        return std::sqrt(MaxRowScaleSquared);
    }
    bool ContainsNaN() const {
        for (int32 i = 0; i < 4; i++)
            for (int32 j = 0; j < 4; j++)
                if (!std::isfinite(M[i][j])) return true;
        return false;
    }
};

template <class T>
inline const TMatrix<T> TMatrix<T>::Identity;

using FMatrix = TMatrix<double>;
using FMatrix44d = TMatrix<double>;
using FMatrix44f = TMatrix<float>;

template <class T>
TMatrix<T> MakeRotationTranslationMatrix(const TRotator<T>& Rot, const TVector<T>& Origin);
template <class T>
TMatrix<T> MakeQuatRotationTranslationMatrix(const TQuat<T>& Q, const TVector<T>& Origin);

struct FRotationMatrix : FMatrix {
    explicit FRotationMatrix(const FRotator& Rot) : FMatrix(MakeRotationTranslationMatrix(Rot, FVector::ZeroVector)) {}
    static FMatrix Make(const FRotator& Rot) { return FRotationMatrix(Rot); }
    static FMatrix Make(const FQuat& Rot) { return MakeQuatRotationTranslationMatrix(Rot, FVector::ZeroVector); }
    static FMatrix MakeFromX(const FVector& XAxis);
    static FMatrix MakeFromZ(const FVector& ZAxis);
    static FMatrix MakeFromXY(const FVector& XAxis, const FVector& YAxis);
    static FMatrix MakeFromXZ(const FVector& XAxis, const FVector& ZAxis);
    static FMatrix MakeFromZX(const FVector& ZAxis, const FVector& XAxis);
};

struct FRotationTranslationMatrix : FMatrix {
    FRotationTranslationMatrix(const FRotator& Rot, const FVector& Origin) : FMatrix(MakeRotationTranslationMatrix(Rot, Origin)) {}
};

struct FQuatRotationMatrix : FMatrix {
    explicit FQuatRotationMatrix(const FQuat& Q) : FMatrix(MakeQuatRotationTranslationMatrix(Q, FVector::ZeroVector)) {}
};

struct FQuatRotationTranslationMatrix : FMatrix {
    FQuatRotationTranslationMatrix(const FQuat& Q, const FVector& Origin) : FMatrix(MakeQuatRotationTranslationMatrix(Q, Origin)) {}
};

struct FScaleMatrix : FMatrix {
    explicit FScaleMatrix(const FVector& Scale) {
        M[0][0] = Scale.X;
        M[1][1] = Scale.Y;
        M[2][2] = Scale.Z;
    }
    explicit FScaleMatrix(double Scale) : FScaleMatrix(FVector(Scale)) {}
};

struct FTranslationMatrix : FMatrix {
    explicit FTranslationMatrix(const FVector& Delta) { SetOrigin(Delta); }
};

struct FLookAtMatrix : FMatrix {
    FLookAtMatrix(const FVector& EyePosition, const FVector& LookAtPosition, const FVector& UpVector);
};

}

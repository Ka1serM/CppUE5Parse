#pragma once

#include "ue5parse/runtime/Core/Math/UnrealMathUtility.h"

// TVector, TVector2, TVector4 and the integer points and vectors (UE's Vector.h, Vector2D.h, Vector4.h, IntPoint.h,
// IntVector.h). FVector is double since UE 5 (large world coordinates); FVector3f is float.

namespace ue5parse::runtime {

template <class T>
struct TRotator;
template <class T>
struct TQuat;

template <class T>
struct TVector {
    using FReal = T;
    T X = 0, Y = 0, Z = 0;

    constexpr TVector() = default;
    constexpr explicit TVector(T InF) : X(InF), Y(InF), Z(InF) {}
    constexpr TVector(T InX, T InY, T InZ) : X(InX), Y(InY), Z(InZ) {}
    template <class U>
    constexpr explicit TVector(const TVector<U>& V) : X(T(V.X)), Y(T(V.Y)), Z(T(V.Z)) {}

    static const TVector ZeroVector;
    static const TVector OneVector;
    static const TVector UpVector;
    static const TVector DownVector;
    static const TVector ForwardVector;
    static const TVector BackwardVector;
    static const TVector RightVector;
    static const TVector LeftVector;
    static const TVector XAxisVector;
    static const TVector YAxisVector;
    static const TVector ZAxisVector;
    static constexpr TVector Zero() { return TVector(); }
    static constexpr TVector One() { return TVector(1); }

    constexpr TVector operator+(const TVector& V) const { return {X + V.X, Y + V.Y, Z + V.Z}; }
    constexpr TVector operator-(const TVector& V) const { return {X - V.X, Y - V.Y, Z - V.Z}; }
    constexpr TVector operator*(const TVector& V) const { return {X * V.X, Y * V.Y, Z * V.Z}; }
    constexpr TVector operator/(const TVector& V) const { return {X / V.X, Y / V.Y, Z / V.Z}; }
    constexpr TVector operator+(T Bias) const { return {X + Bias, Y + Bias, Z + Bias}; }
    constexpr TVector operator-(T Bias) const { return {X - Bias, Y - Bias, Z - Bias}; }
    constexpr TVector operator*(T Scale) const { return {X * Scale, Y * Scale, Z * Scale}; }
    TVector operator/(T Scale) const {
        const T RScale = T(1) / Scale;
        return {X * RScale, Y * RScale, Z * RScale};
    }
    constexpr TVector operator-() const { return {-X, -Y, -Z}; }
    TVector& operator+=(const TVector& V) { X += V.X; Y += V.Y; Z += V.Z; return *this; }
    TVector& operator-=(const TVector& V) { X -= V.X; Y -= V.Y; Z -= V.Z; return *this; }
    TVector& operator*=(const TVector& V) { X *= V.X; Y *= V.Y; Z *= V.Z; return *this; }
    TVector& operator/=(const TVector& V) { X /= V.X; Y /= V.Y; Z /= V.Z; return *this; }
    TVector& operator*=(T Scale) { X *= Scale; Y *= Scale; Z *= Scale; return *this; }
    TVector& operator/=(T V) { const T RV = T(1) / V; X *= RV; Y *= RV; Z *= RV; return *this; }
    // The cross product (UE's operator^) and the dot product (UE's operator|).
    constexpr TVector operator^(const TVector& V) const { return {Y * V.Z - Z * V.Y, Z * V.X - X * V.Z, X * V.Y - Y * V.X}; }
    constexpr T operator|(const TVector& V) const { return X * V.X + Y * V.Y + Z * V.Z; }
    constexpr bool operator==(const TVector& V) const { return X == V.X && Y == V.Y && Z == V.Z; }
    T& operator[](int32 Index) { return (&X)[Index]; }
    T operator[](int32 Index) const { return (&X)[Index]; }
    T& Component(int32 Index) { return (&X)[Index]; }
    T Component(int32 Index) const { return (&X)[Index]; }

    static constexpr TVector CrossProduct(const TVector& A, const TVector& B) { return A ^ B; }
    static constexpr T DotProduct(const TVector& A, const TVector& B) { return A | B; }
    static T Dist(const TVector& A, const TVector& B) { return std::sqrt(DistSquared(A, B)); }
    static T Distance(const TVector& A, const TVector& B) { return Dist(A, B); }
    static constexpr T DistSquared(const TVector& A, const TVector& B) {
        return FMath::Square(B.X - A.X) + FMath::Square(B.Y - A.Y) + FMath::Square(B.Z - A.Z);
    }
    static T Dist2D(const TVector& A, const TVector& B) { return std::sqrt(DistSquared2D(A, B)); }
    static constexpr T DistSquared2D(const TVector& A, const TVector& B) { return FMath::Square(B.X - A.X) + FMath::Square(B.Y - A.Y); }
    static T DistXY(const TVector& A, const TVector& B) { return Dist2D(A, B); }
    static constexpr T DistSquaredXY(const TVector& A, const TVector& B) { return DistSquared2D(A, B); }

    bool Equals(const TVector& V, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return FMath::Abs(X - V.X) <= Tolerance && FMath::Abs(Y - V.Y) <= Tolerance && FMath::Abs(Z - V.Z) <= Tolerance;
    }
    bool AllComponentsEqual(T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return FMath::Abs(X - Y) <= Tolerance && FMath::Abs(X - Z) <= Tolerance && FMath::Abs(Y - Z) <= Tolerance;
    }
    T GetMax() const { return FMath::Max3(X, Y, Z); }
    T GetAbsMax() const { return FMath::Max3(FMath::Abs(X), FMath::Abs(Y), FMath::Abs(Z)); }
    T GetMin() const { return FMath::Min3(X, Y, Z); }
    T GetAbsMin() const { return FMath::Min3(FMath::Abs(X), FMath::Abs(Y), FMath::Abs(Z)); }
    TVector ComponentMin(const TVector& O) const { return {FMath::Min(X, O.X), FMath::Min(Y, O.Y), FMath::Min(Z, O.Z)}; }
    TVector ComponentMax(const TVector& O) const { return {FMath::Max(X, O.X), FMath::Max(Y, O.Y), FMath::Max(Z, O.Z)}; }
    TVector GetAbs() const { return {FMath::Abs(X), FMath::Abs(Y), FMath::Abs(Z)}; }
    T Size() const { return std::sqrt(X * X + Y * Y + Z * Z); }
    T Length() const { return Size(); }
    constexpr T SizeSquared() const { return X * X + Y * Y + Z * Z; }
    constexpr T SquaredLength() const { return SizeSquared(); }
    T Size2D() const { return std::sqrt(X * X + Y * Y); }
    constexpr T SizeSquared2D() const { return X * X + Y * Y; }
    bool IsNearlyZero(T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return FMath::Abs(X) <= Tolerance && FMath::Abs(Y) <= Tolerance && FMath::Abs(Z) <= Tolerance;
    }
    constexpr bool IsZero() const { return X == 0 && Y == 0 && Z == 0; }
    bool IsUnit(T LengthSquaredTolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return FMath::Abs(T(1) - SizeSquared()) < LengthSquaredTolerance;
    }
    bool IsNormalized() const { return FMath::Abs(T(1) - SizeSquared()) < T(UE_THRESH_VECTOR_NORMALIZED); }
    bool Normalize(T Tolerance = T(UE_SMALL_NUMBER)) {
        const T SquareSum = X * X + Y * Y + Z * Z;
        if (SquareSum > Tolerance) {
            const T Scale = T(1) / std::sqrt(SquareSum);
            X *= Scale;
            Y *= Scale;
            Z *= Scale;
            return true;
        }
        return false;
    }
    TVector GetSafeNormal(T Tolerance = T(UE_SMALL_NUMBER), const TVector& ResultIfZero = TVector()) const {
        const T SquareSum = X * X + Y * Y + Z * Z;
        if (SquareSum == T(1)) return *this;
        if (SquareSum < Tolerance) return ResultIfZero;
        const T Scale = T(1) / std::sqrt(SquareSum);
        return {X * Scale, Y * Scale, Z * Scale};
    }
    TVector GetSafeNormal2D(T Tolerance = T(UE_SMALL_NUMBER), const TVector& ResultIfZero = TVector()) const {
        const T SquareSum = X * X + Y * Y;
        if (SquareSum == T(1)) return Z == 0 ? *this : TVector(X, Y, 0);
        if (SquareSum < Tolerance) return ResultIfZero;
        const T Scale = T(1) / std::sqrt(SquareSum);
        return {X * Scale, Y * Scale, 0};
    }
    TVector GetUnsafeNormal() const {
        const T Scale = T(1) / std::sqrt(X * X + Y * Y + Z * Z);
        return {X * Scale, Y * Scale, Z * Scale};
    }
    void ToDirectionAndLength(TVector& OutDir, T& OutLength) const {
        OutLength = Size();
        if (OutLength > T(UE_SMALL_NUMBER)) {
            T OneOverLength = T(1) / OutLength;
            OutDir = TVector(X * OneOverLength, Y * OneOverLength, Z * OneOverLength);
        } else {
            OutDir = TVector();
        }
    }
    TVector GetClampedToSize(T Min, T Max) const {
        T VecSize = Size();
        const TVector VecDir = VecSize > T(UE_SMALL_NUMBER) ? (*this / VecSize) : TVector();
        VecSize = FMath::Clamp(VecSize, Min, Max);
        return VecDir * VecSize;
    }
    TVector GetClampedToMaxSize(T MaxSize) const {
        if (MaxSize < T(UE_KINDA_SMALL_NUMBER)) return TVector();
        const T VSq = SizeSquared();
        if (VSq > FMath::Square(MaxSize)) {
            const T Scale = MaxSize * (T(1) / std::sqrt(VSq));
            return {X * Scale, Y * Scale, Z * Scale};
        }
        return *this;
    }
    TVector GetSignVector() const { return {X >= 0 ? T(1) : T(-1), Y >= 0 ? T(1) : T(-1), Z >= 0 ? T(1) : T(-1)}; }
    TVector Projection() const {
        const T RZ = T(1) / Z;
        return {X * RZ, Y * RZ, 1};
    }
    TVector ProjectOnTo(const TVector& A) const { return A * ((*this | A) / (A | A)); }
    TVector ProjectOnToNormal(const TVector& Normal) const { return Normal * (*this | Normal); }
    static TVector VectorPlaneProject(const TVector& V, const TVector& PlaneNormal) { return V - V.ProjectOnToNormal(PlaneNormal); }
    TVector MirrorByVector(const TVector& MirrorNormal) const { return *this - MirrorNormal * (T(2) * (*this | MirrorNormal)); }
    TVector RotateAngleAxis(T AngleDeg, const TVector& Axis) const;
    T CosineAngle2D(TVector B) const {
        TVector A(*this);
        A.Z = 0;
        B.Z = 0;
        A.Normalize();
        B.Normalize();
        return A | B;
    }
    bool ContainsNaN() const { return !std::isfinite(X) || !std::isfinite(Y) || !std::isfinite(Z); }
    // The rotation that points +X along this vector (UE's ToOrientationRotator / Rotation).
    TRotator<T> ToOrientationRotator() const;
    TRotator<T> Rotation() const;
    TQuat<T> ToOrientationQuat() const;
    TVector BoundToCube(T Radius) const {
        return {FMath::Clamp(X, -Radius, Radius), FMath::Clamp(Y, -Radius, Radius), FMath::Clamp(Z, -Radius, Radius)};
    }
    T HeadingAngle() const {
        TVector PlaneDir = *this;
        PlaneDir.Z = 0;
        PlaneDir = PlaneDir.GetSafeNormal();
        T Angle = std::acos(PlaneDir.X);
        if (PlaneDir.Y < 0) Angle *= T(-1);
        return Angle;
    }
    void FindBestAxisVectors(TVector& Axis1, TVector& Axis2) const {
        const T NX = FMath::Abs(X), NY = FMath::Abs(Y), NZ = FMath::Abs(Z);
        if (NZ > NX && NZ > NY) Axis1 = TVector(1, 0, 0);
        else Axis1 = TVector(0, 0, 1);
        TVector Tmp = Axis1 - *this * (Axis1 | *this);
        Axis1 = Tmp.GetSafeNormal();
        Axis2 = Axis1 ^ *this;
    }
    static TVector PointPlaneProject(const TVector& Point, const TVector& PlaneBase, const TVector& PlaneNormal) {
        return Point - PlaneNormal * ((Point - PlaneBase) | PlaneNormal);
    }

    static constexpr T UE_THRESH_VECTOR_NORMALIZED = T(0.01);
};

template <class T>
constexpr TVector<T> operator*(T Scale, const TVector<T>& V) {
    return V * Scale;
}

template <class T>
inline const TVector<T> TVector<T>::ZeroVector(0, 0, 0);
template <class T>
inline const TVector<T> TVector<T>::OneVector(1, 1, 1);
template <class T>
inline const TVector<T> TVector<T>::UpVector(0, 0, 1);
template <class T>
inline const TVector<T> TVector<T>::DownVector(0, 0, -1);
template <class T>
inline const TVector<T> TVector<T>::ForwardVector(1, 0, 0);
template <class T>
inline const TVector<T> TVector<T>::BackwardVector(-1, 0, 0);
template <class T>
inline const TVector<T> TVector<T>::RightVector(0, 1, 0);
template <class T>
inline const TVector<T> TVector<T>::LeftVector(0, -1, 0);
template <class T>
inline const TVector<T> TVector<T>::XAxisVector(1, 0, 0);
template <class T>
inline const TVector<T> TVector<T>::YAxisVector(0, 1, 0);
template <class T>
inline const TVector<T> TVector<T>::ZAxisVector(0, 0, 1);

template <class T>
TVector<T> TVector<T>::RotateAngleAxis(T AngleDeg, const TVector& Axis) const {
    T S, C;
    FMath::SinCos(&S, &C, FMath::DegreesToRadians(AngleDeg));
    const T XX = Axis.X * Axis.X, YY = Axis.Y * Axis.Y, ZZ = Axis.Z * Axis.Z;
    const T XY = Axis.X * Axis.Y, YZ = Axis.Y * Axis.Z, ZX = Axis.Z * Axis.X;
    const T XS = Axis.X * S, YS = Axis.Y * S, ZS = Axis.Z * S;
    const T OMC = T(1) - C;
    return TVector((OMC * XX + C) * X + (OMC * XY - ZS) * Y + (OMC * ZX + YS) * Z,
                   (OMC * XY + ZS) * X + (OMC * YY + C) * Y + (OMC * YZ - XS) * Z,
                   (OMC * ZX - YS) * X + (OMC * YZ + XS) * Y + (OMC * ZZ + C) * Z);
}

using FVector = TVector<double>;
using FVector3d = TVector<double>;
using FVector3f = TVector<float>;

template <class T>
struct TVector2 {
    T X = 0, Y = 0;

    constexpr TVector2() = default;
    constexpr explicit TVector2(T InF) : X(InF), Y(InF) {}
    constexpr TVector2(T InX, T InY) : X(InX), Y(InY) {}
    template <class U>
    constexpr explicit TVector2(const TVector2<U>& V) : X(T(V.X)), Y(T(V.Y)) {}
    constexpr explicit TVector2(const TVector<T>& V) : X(V.X), Y(V.Y) {}

    static const TVector2 ZeroVector;
    static const TVector2 UnitVector;

    constexpr TVector2 operator+(const TVector2& V) const { return {X + V.X, Y + V.Y}; }
    constexpr TVector2 operator-(const TVector2& V) const { return {X - V.X, Y - V.Y}; }
    constexpr TVector2 operator*(const TVector2& V) const { return {X * V.X, Y * V.Y}; }
    constexpr TVector2 operator/(const TVector2& V) const { return {X / V.X, Y / V.Y}; }
    constexpr TVector2 operator*(T Scale) const { return {X * Scale, Y * Scale}; }
    constexpr TVector2 operator/(T Scale) const { return {X / Scale, Y / Scale}; }
    constexpr TVector2 operator+(T A) const { return {X + A, Y + A}; }
    constexpr TVector2 operator-(T A) const { return {X - A, Y - A}; }
    constexpr TVector2 operator-() const { return {-X, -Y}; }
    TVector2& operator+=(const TVector2& V) { X += V.X; Y += V.Y; return *this; }
    TVector2& operator-=(const TVector2& V) { X -= V.X; Y -= V.Y; return *this; }
    TVector2& operator*=(T Scale) { X *= Scale; Y *= Scale; return *this; }
    TVector2& operator/=(T V) { X /= V; Y /= V; return *this; }
    constexpr T operator|(const TVector2& V) const { return X * V.X + Y * V.Y; }
    constexpr T operator^(const TVector2& V) const { return X * V.Y - Y * V.X; }
    constexpr bool operator==(const TVector2& V) const { return X == V.X && Y == V.Y; }
    T& operator[](int32 Index) { return (&X)[Index]; }
    T operator[](int32 Index) const { return (&X)[Index]; }

    static constexpr T DotProduct(const TVector2& A, const TVector2& B) { return A | B; }
    static constexpr T CrossProduct(const TVector2& A, const TVector2& B) { return A ^ B; }
    static T Distance(const TVector2& A, const TVector2& B) { return std::sqrt(DistSquared(A, B)); }
    static constexpr T DistSquared(const TVector2& A, const TVector2& B) { return FMath::Square(B.X - A.X) + FMath::Square(B.Y - A.Y); }
    static TVector2 Max(const TVector2& A, const TVector2& B) { return {FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y)}; }
    static TVector2 Min(const TVector2& A, const TVector2& B) { return {FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y)}; }
    bool Equals(const TVector2& V, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return FMath::Abs(X - V.X) <= Tolerance && FMath::Abs(Y - V.Y) <= Tolerance;
    }
    T Size() const { return std::sqrt(X * X + Y * Y); }
    T Length() const { return Size(); }
    constexpr T SizeSquared() const { return X * X + Y * Y; }
    T GetMax() const { return FMath::Max(X, Y); }
    T GetMin() const { return FMath::Min(X, Y); }
    T GetAbsMax() const { return FMath::Max(FMath::Abs(X), FMath::Abs(Y)); }
    TVector2 GetAbs() const { return {FMath::Abs(X), FMath::Abs(Y)}; }
    bool IsNearlyZero(T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const { return FMath::Abs(X) <= Tolerance && FMath::Abs(Y) <= Tolerance; }
    constexpr bool IsZero() const { return X == 0 && Y == 0; }
    TVector2 GetSafeNormal(T Tolerance = T(UE_SMALL_NUMBER)) const {
        const T SquareSum = X * X + Y * Y;
        if (SquareSum > Tolerance) {
            const T Scale = T(1) / std::sqrt(SquareSum);
            return {X * Scale, Y * Scale};
        }
        return {};
    }
    void Normalize(T Tolerance = T(UE_SMALL_NUMBER)) {
        const T SquareSum = X * X + Y * Y;
        if (SquareSum > Tolerance) {
            const T Scale = T(1) / std::sqrt(SquareSum);
            X *= Scale;
            Y *= Scale;
            return;
        }
        X = 0;
        Y = 0;
    }
    TVector2 GetRotated(T AngleDeg) const {
        T S, C;
        FMath::SinCos(&S, &C, FMath::DegreesToRadians(AngleDeg));
        return {C * X - S * Y, S * X + C * Y};
    }
    TVector2 ClampAxes(T MinAxisVal, T MaxAxisVal) const {
        return {FMath::Clamp(X, MinAxisVal, MaxAxisVal), FMath::Clamp(Y, MinAxisVal, MaxAxisVal)};
    }
    TVector<T> SphericalToUnitCartesian() const {
        const T SinTheta = std::sin(X);
        return {std::cos(Y) * SinTheta, std::sin(Y) * SinTheta, std::cos(X)};
    }
};

template <class T>
inline const TVector2<T> TVector2<T>::ZeroVector(0, 0);
template <class T>
inline const TVector2<T> TVector2<T>::UnitVector(1, 1);
template <class T>
constexpr TVector2<T> operator*(T Scale, const TVector2<T>& V) {
    return V * Scale;
}

using FVector2D = TVector2<double>;
using FVector2f = TVector2<float>;

template <class T>
struct TVector4 {
    T X = 0, Y = 0, Z = 0, W = 0;

    constexpr TVector4() = default;
    constexpr TVector4(T InX, T InY, T InZ, T InW) : X(InX), Y(InY), Z(InZ), W(InW) {}
    constexpr TVector4(const TVector<T>& V, T InW = 1) : X(V.X), Y(V.Y), Z(V.Z), W(InW) {}
    template <class U>
    constexpr explicit TVector4(const TVector4<U>& V) : X(T(V.X)), Y(T(V.Y)), Z(T(V.Z)), W(T(V.W)) {}

    constexpr TVector4 operator+(const TVector4& V) const { return {X + V.X, Y + V.Y, Z + V.Z, W + V.W}; }
    constexpr TVector4 operator-(const TVector4& V) const { return {X - V.X, Y - V.Y, Z - V.Z, W - V.W}; }
    constexpr TVector4 operator*(const TVector4& V) const { return {X * V.X, Y * V.Y, Z * V.Z, W * V.W}; }
    constexpr TVector4 operator*(T S) const { return {X * S, Y * S, Z * S, W * S}; }
    constexpr TVector4 operator/(T S) const { return {X / S, Y / S, Z / S, W / S}; }
    constexpr TVector4 operator-() const { return {-X, -Y, -Z, -W}; }
    TVector4& operator+=(const TVector4& V) { X += V.X; Y += V.Y; Z += V.Z; W += V.W; return *this; }
    constexpr bool operator==(const TVector4& V) const { return X == V.X && Y == V.Y && Z == V.Z && W == V.W; }
    T& operator[](int32 Index) { return (&X)[Index]; }
    T operator[](int32 Index) const { return (&X)[Index]; }
    TVector<T> XYZ() const { return {X, Y, Z}; }
    bool Equals(const TVector4& V, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return FMath::Abs(X - V.X) <= Tolerance && FMath::Abs(Y - V.Y) <= Tolerance && FMath::Abs(Z - V.Z) <= Tolerance &&
               FMath::Abs(W - V.W) <= Tolerance;
    }
    T Size3() const { return std::sqrt(X * X + Y * Y + Z * Z); }
    T Size() const { return std::sqrt(X * X + Y * Y + Z * Z + W * W); }
};

using FVector4 = TVector4<double>;
using FVector4d = TVector4<double>;
using FVector4f = TVector4<float>;

template <class T>
struct TIntPoint {
    T X = 0, Y = 0;
    constexpr TIntPoint() = default;
    constexpr TIntPoint(T InX, T InY) : X(InX), Y(InY) {}
    constexpr bool operator==(const TIntPoint& O) const { return X == O.X && Y == O.Y; }
    constexpr TIntPoint operator+(const TIntPoint& O) const { return {X + O.X, Y + O.Y}; }
    constexpr TIntPoint operator-(const TIntPoint& O) const { return {X - O.X, Y - O.Y}; }
    constexpr TIntPoint operator*(T S) const { return {X * S, Y * S}; }
    constexpr TIntPoint operator/(T S) const { return {X / S, Y / S}; }
    T& operator[](int32 Index) { return (&X)[Index]; }
};
using FIntPoint = TIntPoint<int32>;
using FUintPoint = TIntPoint<uint32>;
using FInt64Point = TIntPoint<int64>;

template <class T>
struct TIntVector3 {
    T X = 0, Y = 0, Z = 0;
    constexpr TIntVector3() = default;
    constexpr TIntVector3(T InX, T InY, T InZ) : X(InX), Y(InY), Z(InZ) {}
    constexpr bool operator==(const TIntVector3& O) const { return X == O.X && Y == O.Y && Z == O.Z; }
    constexpr TIntVector3 operator+(const TIntVector3& O) const { return {X + O.X, Y + O.Y, Z + O.Z}; }
    constexpr TIntVector3 operator-(const TIntVector3& O) const { return {X - O.X, Y - O.Y, Z - O.Z}; }
    constexpr TIntVector3 operator*(T S) const { return {X * S, Y * S, Z * S}; }
    constexpr TIntVector3 operator/(T S) const { return {X / S, Y / S, Z / S}; }
    T& operator[](int32 Index) { return (&X)[Index]; }
};
using FIntVector = TIntVector3<int32>;
using FUintVector = TIntVector3<uint32>;
using FInt64Vector = TIntVector3<int64>;

template <class T>
struct TIntVector4 {
    T X = 0, Y = 0, Z = 0, W = 0;
    constexpr bool operator==(const TIntVector4& O) const { return X == O.X && Y == O.Y && Z == O.Z && W == O.W; }
};
using FIntVector4 = TIntVector4<int32>;
using FUintVector4 = TIntVector4<uint32>;

template <class T>
struct TIntVector2 {
    T X = 0, Y = 0;
    constexpr bool operator==(const TIntVector2& O) const { return X == O.X && Y == O.Y; }
};
using FIntVector2 = TIntVector2<int32>;
using FUintVector2 = TIntVector2<uint32>;

}

#pragma once

#include "Engine/Source/Runtime/Core/Math/Rotator.h"

namespace ue5parse::runtime {

template <class T>
struct TQuat {
    T X = 0, Y = 0, Z = 0, W = 1;

    constexpr TQuat() = default;
    constexpr TQuat(T InX, T InY, T InZ, T InW) : X(InX), Y(InY), Z(InZ), W(InW) {}
    template <class U>
    constexpr explicit TQuat(const TQuat<U>& Q) : X(T(Q.X)), Y(T(Q.Y)), Z(T(Q.Z)), W(T(Q.W)) {}
    explicit TQuat(const TRotator<T>& R) { *this = R.Quaternion(); }
    TQuat(const TVector<T>& Axis, T AngleRad) {
        const T HalfA = T(0.5) * AngleRad;
        T S, C;
        FMath::SinCos(&S, &C, HalfA);
        X = S * Axis.X;
        Y = S * Axis.Y;
        Z = S * Axis.Z;
        W = C;
    }

    static const TQuat Identity;

    constexpr TQuat operator+(const TQuat& Q) const { return {X + Q.X, Y + Q.Y, Z + Q.Z, W + Q.W}; }
    constexpr TQuat operator-(const TQuat& Q) const { return {X - Q.X, Y - Q.Y, Z - Q.Z, W - Q.W}; }
    constexpr TQuat operator-() const { return {-X, -Y, -Z, -W}; }
    TQuat& operator+=(const TQuat& Q) {
        X += Q.X;
        Y += Q.Y;
        Z += Q.Z;
        W += Q.W;
        return *this;
    }
    TQuat& operator-=(const TQuat& Q) {
        X -= Q.X;
        Y -= Q.Y;
        Z -= Q.Z;
        W -= Q.W;
        return *this;
    }
    constexpr TQuat operator*(T Scale) const { return {X * Scale, Y * Scale, Z * Scale, W * Scale}; }
    TQuat& operator*=(T Scale) {
        X *= Scale;
        Y *= Scale;
        Z *= Scale;
        W *= Scale;
        return *this;
    }
    constexpr TQuat operator/(T Scale) const { return {X / Scale, Y / Scale, Z / Scale, W / Scale}; }
    constexpr TQuat operator*(const TQuat& Q) const {
        return {W * Q.X + X * Q.W + Y * Q.Z - Z * Q.Y, W * Q.Y - X * Q.Z + Y * Q.W + Z * Q.X, W * Q.Z + X * Q.Y - Y * Q.X + Z * Q.W,
                W * Q.W - X * Q.X - Y * Q.Y - Z * Q.Z};
    }
    TQuat& operator*=(const TQuat& Q) {
        *this = *this * Q;
        return *this;
    }
    TVector<T> operator*(const TVector<T>& V) const { return RotateVector(V); }
    constexpr T operator|(const TQuat& Q) const { return X * Q.X + Y * Q.Y + Z * Q.Z + W * Q.W; }
    constexpr bool operator==(const TQuat& Q) const { return X == Q.X && Y == Q.Y && Z == Q.Z && W == Q.W; }

    bool Equals(const TQuat& Q, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return (FMath::Abs(X - Q.X) <= Tolerance && FMath::Abs(Y - Q.Y) <= Tolerance && FMath::Abs(Z - Q.Z) <= Tolerance && FMath::Abs(W - Q.W) <= Tolerance)
               || (FMath::Abs(X + Q.X) <= Tolerance && FMath::Abs(Y + Q.Y) <= Tolerance && FMath::Abs(Z + Q.Z) <= Tolerance
                   && FMath::Abs(W + Q.W) <= Tolerance);
    }
    T Size() const { return std::sqrt(X * X + Y * Y + Z * Z + W * W); }
    constexpr T SizeSquared() const { return X * X + Y * Y + Z * Z + W * W; }
    bool IsNormalized() const { return FMath::Abs(T(1) - SizeSquared()) < T(UE_THRESH_QUAT_NORMALIZED); }
    void Normalize(T Tolerance = T(UE_SMALL_NUMBER)) {
        const T SquareSum = X * X + Y * Y + Z * Z + W * W;
        if (SquareSum >= Tolerance) {
            const T Scale = T(1) / std::sqrt(SquareSum);
            X *= Scale;
            Y *= Scale;
            Z *= Scale;
            W *= Scale;
        } else {
            *this = Identity;
        }
    }
    TQuat GetNormalized(T Tolerance = T(UE_SMALL_NUMBER)) const {
        TQuat Result(*this);
        Result.Normalize(Tolerance);
        return Result;
    }
    constexpr TQuat Inverse() const { return {-X, -Y, -Z, W}; }
    TVector<T> RotateVector(const TVector<T>& V) const {
        const TVector<T> Q(X, Y, Z);
        const TVector<T> TT = TVector<T>::CrossProduct(Q, V) * T(2);
        return V + (TT * W) + TVector<T>::CrossProduct(Q, TT);
    }
    TVector<T> UnrotateVector(const TVector<T>& V) const {
        const TVector<T> Q(-X, -Y, -Z);
        const TVector<T> TT = TVector<T>::CrossProduct(Q, V) * T(2);
        return V + (TT * W) + TVector<T>::CrossProduct(Q, TT);
    }
    TVector<T> GetAxisX() const { return RotateVector(TVector<T>(1, 0, 0)); }
    TVector<T> GetAxisY() const { return RotateVector(TVector<T>(0, 1, 0)); }
    TVector<T> GetAxisZ() const { return RotateVector(TVector<T>(0, 0, 1)); }
    TVector<T> GetForwardVector() const { return GetAxisX(); }
    TVector<T> GetRightVector() const { return GetAxisY(); }
    TVector<T> GetUpVector() const { return GetAxisZ(); }
    TVector<T> Vector() const { return GetAxisX(); }
    TRotator<T> Rotator() const;
    TVector<T> Euler() const { return Rotator().Euler(); }
    static TQuat MakeFromEuler(const TVector<T>& Euler) { return TRotator<T>::MakeFromEuler(Euler).Quaternion(); }
    T GetAngle() const { return T(2) * std::acos(FMath::Clamp(W, T(-1), T(1))); }
    TVector<T> GetRotationAxis() const {
        const T SquareSum = X * X + Y * Y + Z * Z;
        if (SquareSum < T(UE_SMALL_NUMBER))
            return TVector<T>(1, 0, 0);
        const T Scale = T(1) / std::sqrt(SquareSum);
        return {X * Scale, Y * Scale, Z * Scale};
    }
    void ToAxisAndAngle(TVector<T>& Axis, T& Angle) const {
        Angle = GetAngle();
        Axis = GetRotationAxis();
    }
    T AngularDistance(const TQuat& Q) const {
        T InnerProd = X * Q.X + Y * Q.Y + Z * Q.Z + W * Q.W;
        return std::acos(FMath::Clamp(T(2) * InnerProd * InnerProd - T(1), T(-1), T(1)));
    }
    TQuat Log() const;
    TQuat Exp() const;
    void ToSwingTwist(const TVector<T>& InTwistAxis, TQuat& OutSwing, TQuat& OutTwist) const {
        TVector<T> Projection = InTwistAxis * (TVector<T>(X, Y, Z) | InTwistAxis);
        OutTwist = TQuat(Projection.X, Projection.Y, Projection.Z, W);
        if (OutTwist.SizeSquared() == T(0))
            OutTwist = Identity;
        else
            OutTwist.Normalize();
        OutSwing = *this * OutTwist.Inverse();
    }
    T GetTwistAngle(const TVector<T>& TwistAxis) const {
        T XYZ = TVector<T>(X, Y, Z) | TwistAxis;
        return static_cast<T>(FMath::UnwindRadians(T(2) * std::atan2(XYZ, W)));
    }
    bool ContainsNaN() const { return !std::isfinite(X) || !std::isfinite(Y) || !std::isfinite(Z) || !std::isfinite(W); }

    static TQuat FindBetweenNormals(const TVector<T>& A, const TVector<T>& B) { return FindBetweenHelper(A, B, T(1)); }
    static TQuat FastLerp(const TQuat& A, const TQuat& B, T Alpha) {
        const T DotResult = (A | B);
        const T Bias = DotResult >= T(0) ? T(1) : T(-1);
        return (B * Alpha) + (A * (Bias * (T(1) - Alpha)));
    }
    static TQuat Slerp_NotNormalized(const TQuat& Quat1, const TQuat& Quat2, T Slerp);
    static TQuat Slerp(const TQuat& Quat1, const TQuat& Quat2, T Slerp) { return Slerp_NotNormalized(Quat1, Quat2, Slerp).GetNormalized(); }

private:
    static TQuat FindBetweenHelper(const TVector<T>& A, const TVector<T>& B, T NormAB);
};

template <class T>
inline const TQuat<T> TQuat<T>::Identity(0, 0, 0, 1);

using FQuat = TQuat<double>;
using FQuat4d = TQuat<double>;
using FQuat4f = TQuat<float>;

template <class T>
TRotator<T>::TRotator(const TQuat<T>& Quat) {
    *this = Quat.Rotator();
}
template <class T>
TVector<T> TRotator<T>::RotateVector(const TVector<T>& V) const {
    return Quaternion().RotateVector(V);
}
template <class T>
TVector<T> TRotator<T>::UnrotateVector(const TVector<T>& V) const {
    return Quaternion().UnrotateVector(V);
}
template <class T>
TRotator<T> TRotator<T>::GetInverse() const {
    return Quaternion().Inverse().Rotator();
}
template <class T>
TQuat<T> TVector<T>::ToOrientationQuat() const {
    const T YawRad = std::atan2(Y, X);
    const T PitchRad = std::atan2(Z, std::sqrt(X * X + Y * Y));
    T SP, SY, CP, CY;
    FMath::SinCos(&SP, &CP, PitchRad * T(0.5));
    FMath::SinCos(&SY, &CY, YawRad * T(0.5));
    return TQuat<T>(SP * SY, -SP * CY, CP * SY, CP * CY);
}

}

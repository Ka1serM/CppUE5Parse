#pragma once

#include "ue5parse/runtime/Core/Math/Vector.h"

namespace ue5parse::runtime {

// TRotator: pitch (around Y), yaw (around Z) and roll (around X) in degrees, as UE's Rotator.h.
template <class T>
struct TRotator {
    T Pitch = 0, Yaw = 0, Roll = 0;

    constexpr TRotator() = default;
    constexpr explicit TRotator(T InF) : Pitch(InF), Yaw(InF), Roll(InF) {}
    constexpr TRotator(T InPitch, T InYaw, T InRoll) : Pitch(InPitch), Yaw(InYaw), Roll(InRoll) {}
    template <class U>
    constexpr explicit TRotator(const TRotator<U>& R) : Pitch(T(R.Pitch)), Yaw(T(R.Yaw)), Roll(T(R.Roll)) {}
    explicit TRotator(const TQuat<T>& Quat);

    static const TRotator ZeroRotator;

    constexpr TRotator operator+(const TRotator& R) const { return {Pitch + R.Pitch, Yaw + R.Yaw, Roll + R.Roll}; }
    constexpr TRotator operator-(const TRotator& R) const { return {Pitch - R.Pitch, Yaw - R.Yaw, Roll - R.Roll}; }
    constexpr TRotator operator*(T Scale) const { return {Pitch * Scale, Yaw * Scale, Roll * Scale}; }
    constexpr TRotator operator-() const { return {-Pitch, -Yaw, -Roll}; }
    TRotator& operator+=(const TRotator& R) { Pitch += R.Pitch; Yaw += R.Yaw; Roll += R.Roll; return *this; }
    TRotator& operator-=(const TRotator& R) { Pitch -= R.Pitch; Yaw -= R.Yaw; Roll -= R.Roll; return *this; }
    TRotator& operator*=(T Scale) { Pitch *= Scale; Yaw *= Scale; Roll *= Scale; return *this; }
    constexpr bool operator==(const TRotator& R) const { return Pitch == R.Pitch && Yaw == R.Yaw && Roll == R.Roll; }

    bool IsNearlyZero(T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return FMath::Abs(NormalizeAxis(Pitch)) <= Tolerance && FMath::Abs(NormalizeAxis(Yaw)) <= Tolerance &&
               FMath::Abs(NormalizeAxis(Roll)) <= Tolerance;
    }
    bool IsZero() const { return ClampAxis(Pitch) == 0 && ClampAxis(Yaw) == 0 && ClampAxis(Roll) == 0; }
    bool Equals(const TRotator& R, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return FMath::Abs(NormalizeAxis(Pitch - R.Pitch)) <= Tolerance && FMath::Abs(NormalizeAxis(Yaw - R.Yaw)) <= Tolerance &&
               FMath::Abs(NormalizeAxis(Roll - R.Roll)) <= Tolerance;
    }
    TRotator Add(T DeltaPitch, T DeltaYaw, T DeltaRoll) {
        Yaw += DeltaYaw;
        Pitch += DeltaPitch;
        Roll += DeltaRoll;
        return *this;
    }
    TRotator GetInverse() const;
    TRotator GridSnap(const TRotator& RotGrid) const;
    // The unit vector of the rotation's +X axis.
    TVector<T> Vector() const;
    TQuat<T> Quaternion() const;
    TVector<T> Euler() const { return {Roll, Pitch, Yaw}; }
    TVector<T> RotateVector(const TVector<T>& V) const;
    TVector<T> UnrotateVector(const TVector<T>& V) const;
    TRotator Clamp() const { return {ClampAxis(Pitch), ClampAxis(Yaw), ClampAxis(Roll)}; }
    TRotator GetNormalized() const {
        TRotator Rot = *this;
        Rot.Normalize();
        return Rot;
    }
    TRotator GetDenormalized() const {
        TRotator Rot = *this;
        Rot.Pitch = ClampAxis(Rot.Pitch);
        Rot.Yaw = ClampAxis(Rot.Yaw);
        Rot.Roll = ClampAxis(Rot.Roll);
        return Rot;
    }
    void Normalize() {
        Pitch = NormalizeAxis(Pitch);
        Yaw = NormalizeAxis(Yaw);
        Roll = NormalizeAxis(Roll);
    }
    TRotator GetEquivalentRotator() const { return {T(180) - Pitch, Yaw + T(180), Roll + T(180)}; }
    void SetClosestToMe(TRotator& MakeClosest) const;
    bool ContainsNaN() const { return !std::isfinite(Pitch) || !std::isfinite(Yaw) || !std::isfinite(Roll); }

    // [0, 360).
    static T ClampAxis(T Angle) {
        Angle = static_cast<T>(FMath::Fmod(static_cast<double>(Angle), 360.0));
        if (Angle < T(0)) Angle += T(360);
        return Angle;
    }
    // (-180, 180].
    static T NormalizeAxis(T Angle) {
        Angle = ClampAxis(Angle);
        if (Angle > T(180)) Angle -= T(360);
        return Angle;
    }
    static uint8 CompressAxisToByte(T Angle) { return static_cast<uint8>(FMath::RoundToInt(Angle * T(256) / T(360)) & 0xFF); }
    static T DecompressAxisFromByte(uint8 Angle) { return T(Angle) * T(360) / T(256); }
    static uint16 CompressAxisToShort(T Angle) { return static_cast<uint16>(FMath::RoundToInt(Angle * T(65536) / T(360)) & 0xFFFF); }
    static T DecompressAxisFromShort(uint16 Angle) { return T(Angle) * T(360) / T(65536); }
    static TRotator MakeFromEuler(const TVector<T>& Euler) { return {Euler.Y, Euler.Z, Euler.X}; }
};

template <class T>
inline const TRotator<T> TRotator<T>::ZeroRotator(0, 0, 0);

using FRotator = TRotator<double>;
using FRotator3d = TRotator<double>;
using FRotator3f = TRotator<float>;

}

#pragma once

#include "Engine/Source/Runtime/Core/Math/Vector.h"

namespace ue5parse::runtime {

struct FColor;

struct FLinearColor {
    float R = 0, G = 0, B = 0, A = 0;

    constexpr FLinearColor() = default;
    constexpr FLinearColor(float InR, float InG, float InB, float InA = 1.0f) : R(InR), G(InG), B(InB), A(InA) {}
    explicit FLinearColor(const FVector& V) : R(float(V.X)), G(float(V.Y)), B(float(V.Z)), A(1.0f) {}
    explicit FLinearColor(const FVector4& V) : R(float(V.X)), G(float(V.Y)), B(float(V.Z)), A(float(V.W)) {}
    FLinearColor(const FColor& Color);

    static const FLinearColor White;
    static const FLinearColor Gray;
    static const FLinearColor Black;
    static const FLinearColor Transparent;
    static const FLinearColor Red;
    static const FLinearColor Green;
    static const FLinearColor Blue;
    static const FLinearColor Yellow;

    constexpr FLinearColor operator+(const FLinearColor& C) const { return {R + C.R, G + C.G, B + C.B, A + C.A}; }
    constexpr FLinearColor operator-(const FLinearColor& C) const { return {R - C.R, G - C.G, B - C.B, A - C.A}; }
    constexpr FLinearColor operator*(const FLinearColor& C) const { return {R * C.R, G * C.G, B * C.B, A * C.A}; }
    constexpr FLinearColor operator/(const FLinearColor& C) const { return {R / C.R, G / C.G, B / C.B, A / C.A}; }
    constexpr FLinearColor operator*(float S) const { return {R * S, G * S, B * S, A * S}; }
    constexpr FLinearColor operator/(float S) const { return {R / S, G / S, B / S, A / S}; }
    FLinearColor& operator+=(const FLinearColor& C) {
        R += C.R;
        G += C.G;
        B += C.B;
        A += C.A;
        return *this;
    }
    FLinearColor& operator*=(const FLinearColor& C) {
        R *= C.R;
        G *= C.G;
        B *= C.B;
        A *= C.A;
        return *this;
    }
    FLinearColor& operator*=(float S) {
        R *= S;
        G *= S;
        B *= S;
        A *= S;
        return *this;
    }
    constexpr bool operator==(const FLinearColor& C) const { return R == C.R && G == C.G && B == C.B && A == C.A; }
    float& Component(int32 Index) { return (&R)[Index]; }
    float Component(int32 Index) const { return (&R)[Index]; }

    bool Equals(const FLinearColor& C, float Tolerance = UE_KINDA_SMALL_NUMBER) const {
        return FMath::Abs(R - C.R) < Tolerance && FMath::Abs(G - C.G) < Tolerance && FMath::Abs(B - C.B) < Tolerance && FMath::Abs(A - C.A) < Tolerance;
    }
    FLinearColor CopyWithNewOpacity(float NewOpacity) const { return {R, G, B, NewOpacity}; }
    float GetMax() const { return FMath::Max(FMath::Max(FMath::Max(R, G), B), A); }
    float GetMin() const { return FMath::Min(FMath::Min(FMath::Min(R, G), B), A); }
    FColor ToFColor(bool bSRGB) const;
    FLinearColor LinearRGBToHSV() const;
    FLinearColor HSVToLinearRGB() const;

    static FLinearColor MakeFromColorTemperature(float Temp);
    static FLinearColor LerpUsingHSV(const FLinearColor& From, const FLinearColor& To, float Progress);
    static float Dist(const FLinearColor& V1, const FLinearColor& V2) {
        return std::sqrt(FMath::Square(V2.R - V1.R) + FMath::Square(V2.G - V1.G) + FMath::Square(V2.B - V1.B) + FMath::Square(V2.A - V1.A));
    }
};

constexpr FLinearColor operator*(float S, const FLinearColor& C) {
    return C * S;
}

struct FColor {
    uint8 B = 0, G = 0, R = 0, A = 0;

    constexpr FColor() = default;
    constexpr FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255) : B(InB), G(InG), R(InR), A(InA) {}
    explicit FColor(uint32 InColor) {
        uint32 Value = InColor;
        B = uint8(Value & 0xff);
        G = uint8((Value >> 8) & 0xff);
        R = uint8((Value >> 16) & 0xff);
        A = uint8((Value >> 24) & 0xff);
    }

    static const FColor White;
    static const FColor Black;
    static const FColor Transparent;

    constexpr bool operator==(const FColor& C) const { return B == C.B && G == C.G && R == C.R && A == C.A; }
};

}

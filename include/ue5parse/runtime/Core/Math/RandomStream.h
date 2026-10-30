#pragma once

#include <bit>

#include "ue5parse/runtime/Core/Math/Rotator.h"

namespace ue5parse::runtime {

// FRandomStream: a seeded linear congruential generator, so that the same seed gives the same numbers on every
// platform (UE's Math/RandomStream.h).
struct FRandomStream {
    int32 InitialSeed = 0;
    // Mutable as in UE: drawing a number is const.
    mutable int32 Seed = 0;

    constexpr FRandomStream() = default;
    constexpr explicit FRandomStream(int32 InSeed) : InitialSeed(InSeed), Seed(InSeed) {}

    void Initialize(int32 InSeed) {
        InitialSeed = InSeed;
        Seed = InSeed;
    }
    void Reset() { Seed = InitialSeed; }
    int32 GetInitialSeed() const { return InitialSeed; }
    int32 GetCurrentSeed() const { return Seed; }
    void GenerateNewSeed() { Initialize(FMath::Rand()); }

    // [0, 1): the mantissa of a float in [1, 2) from the next seed, minus one.
    float GetFraction() const {
        MutateSeed();
        const uint32 Bits = 0x3F800000u | (static_cast<uint32>(Seed) >> 9);
        return std::bit_cast<float>(Bits) - 1.0f;
    }
    float FRand() const { return GetFraction(); }
    uint32 GetUnsignedInt() const {
        MutateSeed();
        return static_cast<uint32>(Seed);
    }
    // [0, A).
    int32 RandHelper(int32 A) const { return A > 0 ? FMath::Min(FMath::TruncToInt(GetFraction() * float(A)), A - 1) : 0; }
    // [Min, Max].
    int32 RandRange(int32 Min, int32 Max) const { return Min + RandHelper((Max - Min) + 1); }
    float FRandRange(float InMin, float InMax) const { return InMin + (InMax - InMin) * FRand(); }
    FVector GetUnitVector() const {
        FVector Result;
        double L;
        do {
            Result.X = GetFraction() * 2.f - 1.f;
            Result.Y = GetFraction() * 2.f - 1.f;
            Result.Z = GetFraction() * 2.f - 1.f;
            L = Result.SizeSquared();
        } while (L > 1.0 || L < UE_KINDA_SMALL_NUMBER);
        return Result.GetUnsafeNormal();
    }
    FVector VRand() const { return GetUnitVector(); }
    // A unit vector within the cone around Dir of the half angle in radians.
    FVector VRandCone(const FVector& Dir, float ConeHalfAngleRad) const;

    bool operator==(const FRandomStream&) const = default;

private:
    void MutateSeed() const { Seed = static_cast<int32>(static_cast<uint32>(Seed) * 196314165u + 907633515u); }
};

}

#pragma once

#include "Engine/Source/Runtime/Core/Math/UnrealMathUtility.h"

namespace ue5parse::runtime {

namespace ETimespan {

inline constexpr int64 TicksPerMicrosecond = 10;
inline constexpr int64 TicksPerMillisecond = 10000;
inline constexpr int64 TicksPerSecond = 10000000;
inline constexpr int64 TicksPerMinute = 600000000;
inline constexpr int64 TicksPerHour = 36000000000;
inline constexpr int64 TicksPerDay = 864000000000;
inline constexpr int64 TicksPerWeek = 6048000000000;
inline constexpr int64 MaxTicks = 9223372036854775807;
inline constexpr int64 MinTicks = -9223372036854775807 - 1;

}

struct FTimespan {
    int64 Ticks = 0;

    constexpr FTimespan() = default;
    constexpr explicit FTimespan(int64 InTicks) : Ticks(InTicks) {}
    FTimespan(int32 Hours, int32 Minutes, int32 Seconds) { Assign(0, Hours, Minutes, Seconds, 0); }
    FTimespan(int32 Days, int32 Hours, int32 Minutes, int32 Seconds, int32 FractionNano = 0) { Assign(Days, Hours, Minutes, Seconds, FractionNano); }

    constexpr FTimespan operator+(const FTimespan& Other) const { return FTimespan(Ticks + Other.Ticks); }
    constexpr FTimespan operator-(const FTimespan& Other) const { return FTimespan(Ticks - Other.Ticks); }
    constexpr FTimespan operator-() const { return FTimespan(-Ticks); }
    FTimespan operator*(double Scalar) const { return FTimespan(static_cast<int64>(static_cast<double>(Ticks) * Scalar)); }
    FTimespan operator/(double Scalar) const { return FTimespan(static_cast<int64>(static_cast<double>(Ticks) / Scalar)); }
    constexpr bool operator==(const FTimespan&) const = default;
    constexpr auto operator<=>(const FTimespan&) const = default;

    int64 GetTicks() const { return Ticks; }
    int32 GetDays() const { return static_cast<int32>(Ticks / ETimespan::TicksPerDay); }
    int32 GetHours() const { return static_cast<int32>((Ticks / ETimespan::TicksPerHour) % 24); }
    int32 GetMinutes() const { return static_cast<int32>((Ticks / ETimespan::TicksPerMinute) % 60); }
    int32 GetSeconds() const { return static_cast<int32>((Ticks / ETimespan::TicksPerSecond) % 60); }
    int32 GetFractionMilli() const { return static_cast<int32>((Ticks % ETimespan::TicksPerSecond) / ETimespan::TicksPerMillisecond); }
    double GetTotalSeconds() const { return static_cast<double>(Ticks) / ETimespan::TicksPerSecond; }
    FTimespan GetDuration() const { return FTimespan(Ticks >= 0 ? Ticks : -Ticks); }
    bool IsZero() const { return Ticks == 0; }

    static FTimespan FromHours(double Hours) { return FromTicks(Hours * ETimespan::TicksPerHour); }
    static FTimespan FromSeconds(double Seconds) { return FromTicks(Seconds * ETimespan::TicksPerSecond); }
    static FTimespan FromMilliseconds(double Milliseconds) { return FromTicks(Milliseconds * ETimespan::TicksPerMillisecond); }
    static FTimespan MaxValue() { return FTimespan(ETimespan::MaxTicks); }
    static FTimespan MinValue() { return FTimespan(ETimespan::MinTicks); }
    static FTimespan Zero() { return FTimespan(0); }

private:
    static FTimespan FromTicks(double InTicks) { return FTimespan(static_cast<int64>(InTicks + (InTicks >= 0 ? 0.5 : -0.5))); }
    void Assign(int32 Days, int32 Hours, int32 Minutes, int32 Seconds, int32 FractionNano) {
        const int64 TotalTicks = 1000000000ll * (60ll * 60 * 24 * Days + 60ll * 60 * Hours + 60ll * Minutes + Seconds) + FractionNano;
        Ticks = TotalTicks / 100;
    }
};

}

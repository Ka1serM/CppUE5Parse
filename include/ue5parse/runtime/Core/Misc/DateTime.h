#pragma once

#include "ue5parse/runtime/Core/Containers/UnrealString.h"
#include "ue5parse/runtime/Core/Misc/Timespan.h"

namespace ue5parse::runtime {

// FDateTime: a date and time in ticks of 100 nanoseconds since midnight of January 1st of the year 1, in the
// proleptic Gregorian calendar (UE's Misc/DateTime.h).
struct FDateTime {
    int64 Ticks = 0;

    constexpr FDateTime() = default;
    constexpr explicit FDateTime(int64 InTicks) : Ticks(InTicks) {}
    FDateTime(int32 Year, int32 Month, int32 Day, int32 Hour = 0, int32 Minute = 0, int32 Second = 0, int32 Millisecond = 0);

    constexpr FDateTime operator+(const FTimespan& Other) const { return FDateTime(Ticks + Other.Ticks); }
    constexpr FDateTime operator-(const FTimespan& Other) const { return FDateTime(Ticks - Other.Ticks); }
    constexpr FTimespan operator-(const FDateTime& Other) const { return FTimespan(Ticks - Other.Ticks); }
    constexpr bool operator==(const FDateTime&) const = default;
    constexpr auto operator<=>(const FDateTime&) const = default;

    int64 GetTicks() const { return Ticks; }
    FDateTime GetDate() const { return FDateTime(Ticks - Ticks % ETimespan::TicksPerDay); }
    void GetDate(int32& OutYear, int32& OutMonth, int32& OutDay) const;
    int32 GetYear() const;
    int32 GetMonth() const;
    int32 GetDay() const;
    int32 GetDayOfYear() const;
    int32 GetHour() const { return static_cast<int32>((Ticks / ETimespan::TicksPerHour) % 24); }
    int32 GetMinute() const { return static_cast<int32>((Ticks / ETimespan::TicksPerMinute) % 60); }
    int32 GetSecond() const { return static_cast<int32>((Ticks / ETimespan::TicksPerSecond) % 60); }
    int32 GetMillisecond() const { return static_cast<int32>((Ticks / ETimespan::TicksPerMillisecond) % 1000); }
    FTimespan GetTimeOfDay() const { return FTimespan(Ticks % ETimespan::TicksPerDay); }
    int64 ToUnixTimestamp() const { return (Ticks - UnixEpochTicks) / ETimespan::TicksPerSecond; }
    double ToUnixTimestampDecimal() const { return static_cast<double>(Ticks - UnixEpochTicks) / ETimespan::TicksPerSecond; }
    // "yyyy.mm.dd-hh.mm.ss" (UE's ToString).
    FString ToString() const;

    static int32 DaysInMonth(int32 Year, int32 Month);
    static int32 DaysInYear(int32 Year) { return IsLeapYear(Year) ? 366 : 365; }
    static bool IsLeapYear(int32 Year) { return (Year % 4 == 0) && ((Year % 100 != 0) || (Year % 400 == 0)); }
    static bool Validate(int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute, int32 Second, int32 Millisecond);
    static FDateTime FromUnixTimestamp(int64 UnixTime) { return FDateTime(UnixEpochTicks + UnixTime * ETimespan::TicksPerSecond); }
    static FDateTime MaxValue() { return FDateTime(3652059 * ETimespan::TicksPerDay - 1); }
    static FDateTime MinValue() { return FDateTime(0); }
    static FDateTime Now();
    static FDateTime UtcNow();
    static FDateTime Today() { return Now().GetDate(); }
    // "yyyy.mm.dd-hh.mm.ss[.mmm]" or "yyyy.mm.dd" (UE's Parse).
    static bool Parse(const FString& DateTimeString, FDateTime& OutDateTime);

    static constexpr int64 UnixEpochTicks = 621355968000000000;
};

}

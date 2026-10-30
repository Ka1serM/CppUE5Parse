#include "ue5parse/runtime/Core/Misc/DateTime.h"

#include <chrono>
#include <cstdio>
#include <ctime>

namespace ue5parse::runtime {

namespace {

constexpr int32 DaysToMonth[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

// The days since January 1st of the year 1 of a date of the proleptic Gregorian calendar.
int64 DaysSinceEpoch(int32 Year, int32 Month, int32 Day) {
    int64 Days = 0;
    const int64 PreviousYear = Year - 1;
    Days += PreviousYear * 365 + PreviousYear / 4 - PreviousYear / 100 + PreviousYear / 400;
    Days += DaysToMonth[Month - 1];
    if (Month > 2 && FDateTime::IsLeapYear(Year)) Days++;
    return Days + Day - 1;
}

FDateTime FromTimePoint(std::chrono::system_clock::time_point Time) {
    const int64 Ticks = std::chrono::duration_cast<std::chrono::microseconds>(Time.time_since_epoch()).count() * ETimespan::TicksPerMicrosecond;
    return FDateTime(FDateTime::UnixEpochTicks + Ticks);
}

}

FDateTime::FDateTime(int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute, int32 Second, int32 Millisecond) {
    if (!Validate(Year, Month, Day, Hour, Minute, Second, Millisecond)) return;
    Ticks = DaysSinceEpoch(Year, Month, Day) * ETimespan::TicksPerDay + Hour * ETimespan::TicksPerHour +
            Minute * ETimespan::TicksPerMinute + Second * ETimespan::TicksPerSecond + Millisecond * ETimespan::TicksPerMillisecond;
}

// The civil date of a day count: the 400-year, 100-year, 4-year and year cycles of the Gregorian calendar.
void FDateTime::GetDate(int32& OutYear, int32& OutMonth, int32& OutDay) const {
    int64 Days = Ticks / ETimespan::TicksPerDay;
    const int64 Cycles400 = Days / 146097;
    Days -= Cycles400 * 146097;
    int64 Cycles100 = Days / 36524;
    if (Cycles100 == 4) Cycles100 = 3;
    Days -= Cycles100 * 36524;
    const int64 Cycles4 = Days / 1461;
    Days -= Cycles4 * 1461;
    int64 Years = Days / 365;
    if (Years == 4) Years = 3;
    Days -= Years * 365;
    const int32 Year = static_cast<int32>(Cycles400 * 400 + Cycles100 * 100 + Cycles4 * 4 + Years + 1);
    int32 Month = 1;
    while (Month < 12) {
        const int32 MonthEnd = DaysToMonth[Month] + ((Month >= 2 && IsLeapYear(Year)) ? 1 : 0);
        if (Days < MonthEnd) break;
        Month++;
    }
    const int32 MonthStart = DaysToMonth[Month - 1] + ((Month > 2 && IsLeapYear(Year)) ? 1 : 0);
    OutYear = Year;
    OutMonth = Month;
    OutDay = static_cast<int32>(Days - MonthStart + 1);
}

int32 FDateTime::GetYear() const {
    int32 Year, Month, Day;
    GetDate(Year, Month, Day);
    return Year;
}

int32 FDateTime::GetMonth() const {
    int32 Year, Month, Day;
    GetDate(Year, Month, Day);
    return Month;
}

int32 FDateTime::GetDay() const {
    int32 Year, Month, Day;
    GetDate(Year, Month, Day);
    return Day;
}

int32 FDateTime::GetDayOfYear() const {
    int32 Year, Month, Day;
    GetDate(Year, Month, Day);
    return static_cast<int32>(DaysSinceEpoch(Year, Month, Day) - DaysSinceEpoch(Year, 1, 1) + 1);
}

FString FDateTime::ToString() const {
    int32 Year, Month, Day;
    GetDate(Year, Month, Day);
    char Buffer[32];
    std::snprintf(Buffer, sizeof(Buffer), "%04d.%02d.%02d-%02d.%02d.%02d", Year, Month, Day, GetHour(), GetMinute(), GetSecond());
    return FString(std::string_view(Buffer));
}

int32 FDateTime::DaysInMonth(int32 Year, int32 Month) {
    if (Month == 2 && IsLeapYear(Year)) return 29;
    return DaysToMonth[Month] - DaysToMonth[Month - 1];
}

bool FDateTime::Validate(int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute, int32 Second, int32 Millisecond) {
    return Year >= 1 && Year <= 9999 && Month >= 1 && Month <= 12 && Day >= 1 && Day <= DaysInMonth(Year, Month) && Hour >= 0 &&
           Hour <= 23 && Minute >= 0 && Minute <= 59 && Second >= 0 && Second <= 59 && Millisecond >= 0 && Millisecond <= 999;
}

FDateTime FDateTime::UtcNow() { return FromTimePoint(std::chrono::system_clock::now()); }

FDateTime FDateTime::Now() {
    const std::time_t Seconds = std::time(nullptr);
    std::tm Local{};
    std::tm Utc{};
#if defined(_WIN32)
    localtime_s(&Local, &Seconds);
    gmtime_s(&Utc, &Seconds);
#else
    localtime_r(&Seconds, &Local);
    gmtime_r(&Seconds, &Utc);
#endif
    Utc.tm_isdst = Local.tm_isdst;
    const int64 OffsetSeconds = static_cast<int64>(std::mktime(&Local) - std::mktime(&Utc));
    return UtcNow() + FTimespan(OffsetSeconds * ETimespan::TicksPerSecond);
}

bool FDateTime::Parse(const FString& DateTimeString, FDateTime& OutDateTime) {
    const std::string Text = DateTimeString.ToUtf8();
    int32 Year = 0, Month = 0, Day = 0, Hour = 0, Minute = 0, Second = 0, Millisecond = 0;
    int Read = 0;
    const int Fields = std::sscanf(Text.c_str(), "%d.%d.%d-%d.%d.%d%n", &Year, &Month, &Day, &Hour, &Minute, &Second, &Read);
    if (Fields == 6) {
        if (Text[static_cast<std::size_t>(Read)] == '.') {
            if (std::sscanf(Text.c_str() + Read + 1, "%d", &Millisecond) != 1) return false;
        } else if (Text[static_cast<std::size_t>(Read)] != '\0') {
            return false;
        }
    } else if (Fields == 3) {
        Hour = Minute = Second = 0;
        if (std::sscanf(Text.c_str(), "%d.%d.%d%n", &Year, &Month, &Day, &Read) != 3 || Text[static_cast<std::size_t>(Read)] != '\0') return false;
    } else {
        return false;
    }
    if (!Validate(Year, Month, Day, Hour, Minute, Second, Millisecond)) return false;
    OutDateTime = FDateTime(Year, Month, Day, Hour, Minute, Second, Millisecond);
    return true;
}

}

#pragma once

#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Containers/Set.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"
#include "Engine/Source/Runtime/Core/Misc/DateTime.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class UScriptStruct;

inline FArchive& operator<<(FArchive& Ar, FDateTime& DateTime) {
    return Ar << DateTime.Ticks;
}
inline FArchive& operator<<(FArchive& Ar, FTimespan& Timespan) {
    return Ar << Timespan.Ticks;
}

struct FFrameNumber {
    int32 Value = 0;
    bool operator==(const FFrameNumber&) const = default;
};
inline FArchive& operator<<(FArchive& Ar, FFrameNumber& FrameNumber) {
    return Ar << FrameNumber.Value;
}

struct FFrameRate {
    int32 Numerator = 30;
    int32 Denominator = 1;
    double AsInterval() const { return Numerator != 0 ? static_cast<double>(Denominator) / Numerator : 0.0; }
    bool operator==(const FFrameRate&) const = default;
};

inline FArchive& operator<<(FArchive& Ar, FFrameRate& Rate) {
    return Ar << Rate.Numerator << Rate.Denominator;
}

struct FTwoVectors {
    FVector v1;
    FVector v2;
};
inline FArchive& operator<<(FArchive& Ar, FTwoVectors& TwoVectors) {
    return Ar << TwoVectors.v1 << TwoVectors.v2;
}

struct FPerPlatformInt {
    int32 Default = 0;
    int32 GetValue() const { return Default; }
};
struct FPerPlatformFloat {
    float Default = 0;
    float GetValue() const { return Default; }
};
struct FPerPlatformBool {
    bool Default = false;
    bool GetValue() const { return Default; }
};
struct FPerPlatformFrameRate {
    FFrameRate Default;
};
struct FPerPlatformFString {
    FString Default;
};

struct FPerQualityLevelInt {
    int32 Default = 0;
    TArray<std::pair<int32, int32>> PerQuality;
    int32 GetValue(int32 QualityLevel) const {
        for (const auto& [Level, Value] : PerQuality)
            if (Level == QualityLevel)
                return Value;
        return Default;
    }
};
struct FPerQualityLevelFloat {
    float Default = 0;
    TArray<std::pair<int32, float>> PerQuality;
    float GetValue(int32 QualityLevel) const {
        for (const auto& [Level, Value] : PerQuality)
            if (Level == QualityLevel)
                return Value;
        return Default;
    }
};

class FInstancedStruct {
public:
    FInstancedStruct() = default;
    FInstancedStruct(const FInstancedStruct& Other) { *this = Other; }
    FInstancedStruct& operator=(const FInstancedStruct& Other);
    ~FInstancedStruct() { Reset(); }

    void InitializeAs(const UScriptStruct* InScriptStruct);
    void Reset();
    uint8* GetMutableMemory() { return StructMemory; }
    bool IsValid() const { return ScriptStruct != nullptr && StructMemory != nullptr; }
    bool operator==(const FInstancedStruct& Other) const;
    void Serialize(FArchive& Ar);

private:
    const UScriptStruct* ScriptStruct = nullptr;
    uint8* StructMemory = nullptr;
};

}

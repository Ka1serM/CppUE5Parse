#pragma once

#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/Core/Containers/Map.h"
#include "ue5parse/runtime/Core/Containers/Set.h"
#include "ue5parse/runtime/Core/Containers/UnrealString.h"
#include "ue5parse/runtime/Core/Math/UnrealMath.h"
#include "ue5parse/runtime/Core/Misc/DateTime.h"
#include "ue5parse/runtime/Core/Serialization/Archive.h"

// The core structs that cooked data serializes natively (UE's NoExportTypes.h with their TStructOpsTypeTraits
// WithSerializer): time, frame numbers, per-platform and per-quality values, instanced structs.

namespace ue5parse::runtime {

class UScriptStruct;

inline FArchive& operator<<(FArchive& Ar, FDateTime& DateTime) { return Ar << DateTime.Ticks; }
inline FArchive& operator<<(FArchive& Ar, FTimespan& Timespan) { return Ar << Timespan.Ticks; }

struct FFrameNumber {
    int32 Value = 0;
    bool operator==(const FFrameNumber&) const = default;
};
inline FArchive& operator<<(FArchive& Ar, FFrameNumber& FrameNumber) { return Ar << FrameNumber.Value; }

struct FFrameRate {
    int32 Numerator = 30;
    int32 Denominator = 1;
    double AsDecimal() const { return Denominator != 0 ? static_cast<double>(Numerator) / Denominator : 0.0; }
    double AsInterval() const { return Numerator != 0 ? static_cast<double>(Denominator) / Numerator : 0.0; }
    bool operator==(const FFrameRate&) const = default;
};

inline FArchive& operator<<(FArchive& Ar, FFrameRate& Rate) { return Ar << Rate.Numerator << Rate.Denominator; }

struct FTwoVectors {
    FVector v1;
    FVector v2;
};
inline FArchive& operator<<(FArchive& Ar, FTwoVectors& TwoVectors) { return Ar << TwoVectors.v1 << TwoVectors.v2; }

// TPerPlatformProperty: cooked data keeps only the value of the cooked platform.
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

// FPerQualityLevelProperty: the default and the values of the quality levels that differ.
struct FPerQualityLevelInt {
    int32 Default = 0;
    TArray<std::pair<int32, int32>> PerQuality;
    int32 GetValue(int32 QualityLevel) const {
        for (const auto& [Level, Value] : PerQuality)
            if (Level == QualityLevel) return Value;
        return Default;
    }
};
struct FPerQualityLevelFloat {
    float Default = 0;
    TArray<std::pair<int32, float>> PerQuality;
    float GetValue(int32 QualityLevel) const {
        for (const auto& [Level, Value] : PerQuality)
            if (Level == QualityLevel) return Value;
        return Default;
    }
};

// FInstancedStruct: a struct of any type, owned (StructUtils' InstancedStruct.h).
class FInstancedStruct {
public:
    FInstancedStruct() = default;
    FInstancedStruct(const FInstancedStruct& Other) { *this = Other; }
    FInstancedStruct& operator=(const FInstancedStruct& Other);
    ~FInstancedStruct() { Reset(); }

    void InitializeAs(const UScriptStruct* InScriptStruct);
    void Reset();
    const UScriptStruct* GetScriptStruct() const { return ScriptStruct; }
    uint8* GetMutableMemory() { return StructMemory; }
    const uint8* GetMemory() const { return StructMemory; }
    bool IsValid() const { return ScriptStruct != nullptr && StructMemory != nullptr; }
    bool operator==(const FInstancedStruct& Other) const;
    void Serialize(FArchive& Ar);

private:
    const UScriptStruct* ScriptStruct = nullptr;
    uint8* StructMemory = nullptr;
};

}

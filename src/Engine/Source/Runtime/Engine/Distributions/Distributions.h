#pragma once

#include "Engine/Source/Runtime/Core/Math/RandomStream.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "Distributions.generated.h"

namespace ue5parse::runtime {

enum ERawDistributionOperation : int32 { RDO_Uninitialized, RDO_None, RDO_Random, RDO_Extreme };

UENUM()
enum EDistributionVectorLockFlags : int32 { EDVLF_None, EDVLF_XY, EDVLF_XZ, EDVLF_YZ, EDVLF_XYZ, EDVLF_MAX };

UENUM()
enum EDistributionVectorMirrorFlags : int32 { EDVMF_Same, EDVMF_Different, EDVMF_Mirror, EDVMF_MAX };

UENUM()
enum DistributionParamMode : int32 { DPM_Normal, DPM_Abs, DPM_Direct, DPM_MAX };

USTRUCT()
struct FDistributionLookupTable {
    GENERATED_BODY()
    UPROPERTY()
    float TimeScale = 0;
    UPROPERTY()
    float TimeBias = 0;
    UPROPERTY()
    TArray<float> Values;
    UPROPERTY()
    uint8 Op = RDO_Uninitialized;
    UPROPERTY()
    uint8 EntryCount = 0;
    UPROPERTY()
    uint8 EntryStride = 0;
    UPROPERTY()
    uint8 SubEntryStride = 0;
    UPROPERTY()
    uint8 LockFlag = 0;

    bool IsEmpty() const { return Values.Num() == 0 || EntryCount == 0; }
    float GetValuesPerEntry() const { return float(EntryStride - SubEntryStride); }
    void GetEntry(float Time, const float*& Entry1, const float*& Entry2, float& LerpAlpha) const {
        Time = (Time - TimeBias) * TimeScale;
        if (Time < 0.0f)
            Time = 0.0f;
        LerpAlpha = Time - std::trunc(Time);
        const uint32 Index = uint32(Time);
        const uint32 Last = uint32(EntryCount) - 1;
        Entry1 = &Values[int32(FMath::Min(Index, Last) * EntryStride)];
        Entry2 = &Values[int32(FMath::Min(Index + 1, Last) * EntryStride)];
    }
    void GetRange(float* OutMinValues, float* OutMaxValues) const;
};

USTRUCT()
struct FRawDistribution {
    GENERATED_BODY()
    UPROPERTY()
    FDistributionLookupTable Table;

    void GetValue(float Time, float* Value, int32 NumCoords, int32 Extreme, FRandomStream* InRandomStream) const;
    void GetValue1(float Time, float* Value, int32 Extreme, FRandomStream* InRandomStream) const;
    void GetValue3(float Time, float* Value, int32 Extreme, FRandomStream* InRandomStream) const;
    void GetValue1None(float Time, float* Value) const;
    void GetValue3None(float Time, float* Value) const;
    void GetValue1Extreme(float Time, float* Value, int32 Extreme, FRandomStream* InRandomStream) const;
    void GetValue3Extreme(float Time, float* Value, int32 Extreme, FRandomStream* InRandomStream) const;
    void GetValue1Random(float Time, float* Value, FRandomStream* InRandomStream) const;
    void GetValue3Random(float Time, float* Value, FRandomStream* InRandomStream) const;
    bool IsSimple() const { return Table.Op == RDO_None; }
};

class UDistributionFloat;
class UDistributionVector;

USTRUCT()
struct FRawDistributionFloat : public FRawDistribution {
    GENERATED_BODY()
    UPROPERTY()
    float MinValue = 0;
    UPROPERTY()
    float MaxValue = 0;
    UPROPERTY()
    TObjectPtr<UDistributionFloat> Distribution = nullptr;

    bool IsCreated() const { return HasLookupTable() || Distribution != nullptr; }
    bool HasLookupTable() const { return !Table.IsEmpty(); }
    bool IsUniform() const { return Table.SubEntryStride != 0; }
    float GetValue(float F = 0.0f, UObject* Data = nullptr, FRandomStream* InRandomStream = nullptr) const;
    void GetOutRange(float& MinOut, float& MaxOut) const;
    const FRawDistribution* GetFastRawDistribution() const { return IsSimple() && HasLookupTable() ? this : nullptr; }
};

USTRUCT()
struct FRawDistributionVector : public FRawDistribution {
    GENERATED_BODY()
    UPROPERTY()
    float MinValue = 0;
    UPROPERTY()
    float MaxValue = 0;
    UPROPERTY()
    FVector MinValueVec = FVector::ZeroVector;
    UPROPERTY()
    FVector MaxValueVec = FVector::ZeroVector;
    UPROPERTY()
    TObjectPtr<UDistributionVector> Distribution = nullptr;

    bool IsCreated() const { return HasLookupTable() || Distribution != nullptr; }
    bool HasLookupTable() const { return !Table.IsEmpty(); }
    bool IsUniform() const { return Table.SubEntryStride != 0; }
    FVector GetValue(float F = 0.0f, UObject* Data = nullptr, int32 Extreme = 0, FRandomStream* InRandomStream = nullptr) const;
    void GetOutRange(float& MinOut, float& MaxOut) const;
    void GetRange(FVector& MinOut, FVector& MaxOut) const;
    const FRawDistribution* GetFastRawDistribution() const { return IsSimple() && HasLookupTable() ? this : nullptr; }
};

UCLASS(abstract)
class UDistribution : public UObject {
    GENERATED_BODY()
public:
    static constexpr float DefaultValue = 1.2345E-20f;
};

UCLASS(abstract)
class UDistributionFloat : public UDistribution {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bCanBeBaked = true;
    bool bIsDirty = true;
    UPROPERTY()
    bool bBakedDataSuccesfully = false;

    virtual float GetValue(float F = 0.0f, UObject* Data = nullptr, FRandomStream* InRandomStream = nullptr) const { return 0.0f; }
    virtual void GetOutRange(float& MinOut, float& MaxOut) const {
        MinOut = 0.0f;
        MaxOut = 0.0f;
    }
};

UCLASS()
class UDistributionFloatConstant : public UDistributionFloat {
    GENERATED_BODY()
public:
    UPROPERTY()
    float Constant = 0;

    void PostInitProperties() override;
    void PostLoad() override;
    float GetValue(float F = 0.0f, UObject* Data = nullptr, FRandomStream* InRandomStream = nullptr) const override { return Constant; }
    void GetOutRange(float& MinOut, float& MaxOut) const override {
        MinOut = Constant;
        MaxOut = Constant;
    }
};

UCLASS()
class UDistributionFloatUniform : public UDistributionFloat {
    GENERATED_BODY()
public:
    UPROPERTY()
    float Min = 0;
    UPROPERTY()
    float Max = 0;

    void PostInitProperties() override;
    void PostLoad() override;
    float GetValue(float F = 0.0f, UObject* Data = nullptr, FRandomStream* InRandomStream = nullptr) const override;
    void GetOutRange(float& MinOut, float& MaxOut) const override {
        MinOut = Min;
        MaxOut = Max;
    }
};

UCLASS(abstract)
class UDistributionFloatParameterBase : public UDistributionFloatConstant {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    float MinInput = 0;
    UPROPERTY()
    float MaxInput = 1.0f;
    UPROPERTY()
    float MinOutput = 0;
    UPROPERTY()
    float MaxOutput = 1.0f;
    UPROPERTY()
    TEnumAsByte<DistributionParamMode> ParamMode = DPM_Normal;

    float GetValue(float F = 0.0f, UObject* Data = nullptr, FRandomStream* InRandomStream = nullptr) const override;
    virtual bool GetParamValue(UObject* Data, FName ParamName, float& OutFloat) const { return false; }
};

UCLASS()
class UDistributionFloatParticleParameter : public UDistributionFloatParameterBase {
    GENERATED_BODY()
public:
    bool GetParamValue(UObject* Data, FName ParamName, float& OutFloat) const override;
};

UCLASS(abstract)
class UDistributionVector : public UDistribution {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bCanBeBaked = true;
    UPROPERTY()
    bool bIsDirty = true;
    UPROPERTY()
    bool bBakedDataSuccesfully = false;

    virtual FVector GetValue(float F = 0.0f, UObject* Data = nullptr, int32 Extreme = 0, FRandomStream* InRandomStream = nullptr) const {
        return FVector::ZeroVector;
    }
    virtual void GetOutRange(float& MinOut, float& MaxOut) const {
        MinOut = 0.0f;
        MaxOut = 0.0f;
    }
    virtual void GetRange(FVector& OutMin, FVector& OutMax) const {
        OutMin = FVector::ZeroVector;
        OutMax = FVector::ZeroVector;
    }
};

UCLASS()
class UDistributionVectorConstant : public UDistributionVector {
    GENERATED_BODY()
public:
    UPROPERTY()
    FVector Constant = FVector::ZeroVector;
    UPROPERTY()
    bool bLockAxes = false;
    UPROPERTY()
    TEnumAsByte<EDistributionVectorLockFlags> LockedAxes = EDVLF_None;

    void PostInitProperties() override;
    void PostLoad() override;
    FVector GetValue(float F = 0.0f, UObject* Data = nullptr, int32 Extreme = 0, FRandomStream* InRandomStream = nullptr) const override;
    void GetOutRange(float& MinOut, float& MaxOut) const override;
    void GetRange(FVector& OutMin, FVector& OutMax) const override {
        OutMin = Constant;
        OutMax = Constant;
    }
};

UCLASS()
class UDistributionVectorUniform : public UDistributionVector {
    GENERATED_BODY()
public:
    UPROPERTY()
    FVector Max = FVector::ZeroVector;
    UPROPERTY()
    FVector Min = FVector::ZeroVector;
    UPROPERTY()
    bool bLockAxes = false;
    UPROPERTY()
    TEnumAsByte<EDistributionVectorLockFlags> LockedAxes = EDVLF_None;
    UPROPERTY()
    TEnumAsByte<EDistributionVectorMirrorFlags> MirrorFlags[3] = {EDVMF_Different, EDVMF_Different, EDVMF_Different};
    UPROPERTY()
    bool bUseExtremes = false;

    void PostInitProperties() override;
    void PostLoad() override;
    FVector GetValue(float F = 0.0f, UObject* Data = nullptr, int32 Extreme = 0, FRandomStream* InRandomStream = nullptr) const override;
    void GetOutRange(float& MinOut, float& MaxOut) const override;
    void GetRange(FVector& OutMin, FVector& OutMax) const override;
};

UCLASS(abstract)
class UDistributionVectorParameterBase : public UDistributionVectorConstant {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    FVector MinInput = FVector::ZeroVector;
    UPROPERTY()
    FVector MaxInput = FVector(1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector MinOutput = FVector::ZeroVector;
    UPROPERTY()
    FVector MaxOutput = FVector(1.0f, 1.0f, 1.0f);
    UPROPERTY()
    TEnumAsByte<DistributionParamMode> ParamModes[3] = {DPM_Normal, DPM_Normal, DPM_Normal};

    FVector GetValue(float F = 0.0f, UObject* Data = nullptr, int32 Extreme = 0, FRandomStream* InRandomStream = nullptr) const override;
    virtual bool GetParamValue(UObject* Data, FName ParamName, FVector& OutVector) const { return false; }
};

UCLASS()
class UDistributionVectorParticleParameter : public UDistributionVectorParameterBase {
    GENERATED_BODY()
public:
    bool GetParamValue(UObject* Data, FName ParamName, FVector& OutVector) const override;
};

}

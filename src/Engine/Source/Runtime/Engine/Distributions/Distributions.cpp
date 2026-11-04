#include "Engine/Source/Runtime/Engine/Distributions/Distributions.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

namespace {

float GetRandomValue(FRandomStream* RandomStream) { return RandomStream == nullptr ? FMath::SRand() : RandomStream->GetFraction(); }

bool IsLoadedForParticleModule(const UObject* Distribution) {
    return Distribution->HasAnyFlags(RF_NeedLoad) && Distribution->GetOuter() != nullptr && Distribution->GetOuter()->IsA<UParticleModule>();
}

FVector LockAxes(const FVector& V, EDistributionVectorLockFlags LockedAxes) {
    switch (LockedAxes) {
    case EDVLF_XY: return FVector(V.X, V.X, V.Z);
    case EDVLF_XZ: return FVector(V.X, V.Y, V.X);
    case EDVLF_YZ: return FVector(V.X, V.Y, V.Y);
    case EDVLF_XYZ: return FVector(V.X, V.X, V.X);
    default: return V;
    }
}

}

void FDistributionLookupTable::GetRange(float* OutMinValues, float* OutMaxValues) const {
    if (EntryCount == 0) return;
    const int32 ValuesPerEntry = int32(GetValuesPerEntry());
    const float* Entry = Values.GetData();
    for (int32 ValueIndex = 0; ValueIndex < ValuesPerEntry; ++ValueIndex) {
        OutMinValues[ValueIndex] = Entry[ValueIndex];
        OutMaxValues[ValueIndex] = Entry[ValueIndex + SubEntryStride];
    }
    for (int32 EntryIndex = 1; EntryIndex < EntryCount; ++EntryIndex) {
        Entry += EntryStride;
        for (int32 ValueIndex = 0; ValueIndex < ValuesPerEntry; ++ValueIndex) {
            OutMinValues[ValueIndex] = FMath::Min(OutMinValues[ValueIndex], Entry[ValueIndex]);
            OutMaxValues[ValueIndex] = FMath::Max(OutMaxValues[ValueIndex], Entry[ValueIndex + SubEntryStride]);
        }
    }
}

void FRawDistribution::GetValue1None(float Time, float* Value) const {
    const float* Entry1;
    const float* Entry2;
    float LerpAlpha = 0.0f;
    Table.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    Value[0] = FMath::Lerp(Entry1[0], Entry2[0], LerpAlpha);
}

void FRawDistribution::GetValue3None(float Time, float* Value) const {
    const float* Entry1;
    const float* Entry2;
    float LerpAlpha = 0.0f;
    Table.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    Value[0] = FMath::Lerp(Entry1[0], Entry2[0], LerpAlpha);
    Value[1] = FMath::Lerp(Entry1[1], Entry2[1], LerpAlpha);
    Value[2] = FMath::Lerp(Entry1[2], Entry2[2], LerpAlpha);
}

void FRawDistribution::GetValue1Extreme(float Time, float* Value, int32 Extreme, FRandomStream* InRandomStream) const {
    const float* Entry1;
    const float* Entry2;
    float LerpAlpha = 0.0f;
    const float RandValue = GetRandomValue(InRandomStream);
    Table.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    const int32 InitialElement = (Extreme > 0) || (Extreme == 0 && RandValue > 0.5f);
    Value[0] = FMath::Lerp(Entry1[InitialElement], Entry2[InitialElement], LerpAlpha);
}

void FRawDistribution::GetValue3Extreme(float Time, float* Value, int32 Extreme, FRandomStream* InRandomStream) const {
    const float* Entry1;
    const float* Entry2;
    float LerpAlpha = 0.0f;
    const float RandValue = GetRandomValue(InRandomStream);
    Table.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    const int32 InitialElement = ((Extreme > 0) || (Extreme == 0 && RandValue > 0.5f)) * 3;
    Value[0] = FMath::Lerp(Entry1[InitialElement + 0], Entry2[InitialElement + 0], LerpAlpha);
    Value[1] = FMath::Lerp(Entry1[InitialElement + 1], Entry2[InitialElement + 1], LerpAlpha);
    Value[2] = FMath::Lerp(Entry1[InitialElement + 2], Entry2[InitialElement + 2], LerpAlpha);
}

void FRawDistribution::GetValue1Random(float Time, float* Value, FRandomStream* InRandomStream) const {
    const float* Entry1;
    const float* Entry2;
    float LerpAlpha = 0.0f;
    const float RandValue = GetRandomValue(InRandomStream);
    Table.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    const float Value1 = FMath::Lerp(Entry1[0], Entry2[0], LerpAlpha);
    const float Value2 = FMath::Lerp(Entry1[1], Entry2[1], LerpAlpha);
    Value[0] = Value1 + (Value2 - Value1) * RandValue;
}

void FRawDistribution::GetValue3Random(float Time, float* Value, FRandomStream* InRandomStream) const {
    const float* Entry1;
    const float* Entry2;
    float LerpAlpha = 0.0f;
    float RandValues[3];
    RandValues[0] = GetRandomValue(InRandomStream);
    RandValues[1] = GetRandomValue(InRandomStream);
    RandValues[2] = GetRandomValue(InRandomStream);
    switch (Table.LockFlag) {
    case EDVLF_XY: RandValues[1] = RandValues[0]; break;
    case EDVLF_XZ: RandValues[2] = RandValues[0]; break;
    case EDVLF_YZ: RandValues[2] = RandValues[1]; break;
    case EDVLF_XYZ:
        RandValues[1] = RandValues[0];
        RandValues[2] = RandValues[0];
        break;
    default: break;
    }
    Table.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    for (int32 i = 0; i < 3; i++) {
        const float V0 = FMath::Lerp(Entry1[i], Entry2[i], LerpAlpha);
        const float V1 = FMath::Lerp(Entry1[3 + i], Entry2[3 + i], LerpAlpha);
        Value[i] = V0 + (V1 - V0) * RandValues[i];
    }
}

void FRawDistribution::GetValue1(float Time, float* Value, int32 Extreme, FRandomStream* InRandomStream) const {
    switch (Table.Op) {
    case RDO_None: GetValue1None(Time, Value); break;
    case RDO_Extreme: GetValue1Extreme(Time, Value, Extreme, InRandomStream); break;
    case RDO_Random: GetValue1Random(Time, Value, InRandomStream); break;
    default: *Value = 0.0f; break;
    }
}

void FRawDistribution::GetValue3(float Time, float* Value, int32 Extreme, FRandomStream* InRandomStream) const {
    switch (Table.Op) {
    case RDO_None: GetValue3None(Time, Value); break;
    case RDO_Extreme: GetValue3Extreme(Time, Value, Extreme, InRandomStream); break;
    case RDO_Random: GetValue3Random(Time, Value, InRandomStream); break;
    default: break;
    }
}

void FRawDistribution::GetValue(float Time, float* Value, int32 NumCoords, int32 Extreme, FRandomStream* InRandomStream) const {
    if (NumCoords == 1) {
        GetValue1(Time, Value, Extreme, InRandomStream);
    } else {
        GetValue3(Time, Value, Extreme, InRandomStream);
    }
}

float FRawDistributionFloat::GetValue(float F, UObject* Data, FRandomStream* InRandomStream) const {
    if (!HasLookupTable()) return Distribution != nullptr ? Distribution->GetValue(F, Data, InRandomStream) : 0.0f;
    float Value;
    GetValue1(F, &Value, 0, InRandomStream);
    return Value;
}

void FRawDistributionFloat::GetOutRange(float& MinOut, float& MaxOut) const {
    if (!HasLookupTable() && Distribution != nullptr) {
        Distribution->GetOutRange(MinOut, MaxOut);
    } else {
        MinOut = MinValue;
        MaxOut = MaxValue;
    }
}

FVector FRawDistributionVector::GetValue(float F, UObject* Data, int32 Extreme, FRandomStream* InRandomStream) const {
    if (!HasLookupTable()) return Distribution != nullptr ? Distribution->GetValue(F, Data, Extreme, InRandomStream) : FVector::ZeroVector;
    FVector3f Value;
    GetValue3(F, &Value.X, Extreme, InRandomStream);
    return FVector(Value);
}

void FRawDistributionVector::GetOutRange(float& MinOut, float& MaxOut) const {
    if (!HasLookupTable() && Distribution != nullptr) {
        Distribution->GetOutRange(MinOut, MaxOut);
    } else {
        MinOut = MinValue;
        MaxOut = MaxValue;
    }
}

void FRawDistributionVector::GetRange(FVector& MinOut, FVector& MaxOut) const {
    if (Distribution != nullptr) {
        Distribution->GetRange(MinOut, MaxOut);
    } else {
        MinOut = MinValueVec;
        MaxOut = MaxValueVec;
    }
}

void UDistributionFloatConstant::PostInitProperties() {
    Super::PostInitProperties();
    if (IsLoadedForParticleModule(this)) Constant = DefaultValue;
}

void UDistributionFloatConstant::PostLoad() {
    Super::PostLoad();
    if (Constant == DefaultValue) Constant = 0.0f;
}

void UDistributionFloatUniform::PostInitProperties() {
    Super::PostInitProperties();
    if (!IsLoadedForParticleModule(this)) return;
    Min = DefaultValue;
    Max = DefaultValue;
}

void UDistributionFloatUniform::PostLoad() {
    Super::PostLoad();
    if (Min == DefaultValue) Min = 0.0f;
    if (Max == DefaultValue) Max = 0.0f;
}

float UDistributionFloatUniform::GetValue(float F, UObject* Data, FRandomStream* InRandomStream) const {
    return Max + (Min - Max) * GetRandomValue(InRandomStream);
}

float UDistributionFloatParameterBase::GetValue(float F, UObject* Data, FRandomStream* InRandomStream) const {
    float ParamFloat = 0.0f;
    if (!GetParamValue(Data, ParameterName, ParamFloat)) ParamFloat = Constant;
    if (ParamMode == DPM_Direct) return ParamFloat;
    if (ParamMode == DPM_Abs) ParamFloat = FMath::Abs(ParamFloat);
    const float Gradient = MaxInput <= MinInput ? 0.0f : (MaxOutput - MinOutput) / (MaxInput - MinInput);
    const float ClampedParam = FMath::Clamp(ParamFloat, MinInput, MaxInput);
    return MinOutput + ((ClampedParam - MinInput) * Gradient);
}

bool UDistributionFloatParticleParameter::GetParamValue(UObject* Data, FName ParamName, float& OutFloat) const {
    UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(Data);
    return ParticleComp != nullptr && ParticleComp->GetFloatParameter(ParameterName, OutFloat);
}

void UDistributionVectorConstant::PostInitProperties() {
    Super::PostInitProperties();
    if (IsLoadedForParticleModule(this)) Constant = FVector(DefaultValue);
}

void UDistributionVectorConstant::PostLoad() {
    Super::PostLoad();
    if (Constant == FVector(DefaultValue)) Constant = FVector::ZeroVector;
}

FVector UDistributionVectorConstant::GetValue(float F, UObject* Data, int32 Extreme, FRandomStream* InRandomStream) const {
    return LockAxes(Constant, LockedAxes);
}

void UDistributionVectorConstant::GetOutRange(float& MinOut, float& MaxOut) const {
    const FVector Local = LockAxes(Constant, LockedAxes);
    MinOut = float(Local.GetMin());
    MaxOut = float(Local.GetMax());
}

void UDistributionVectorUniform::PostInitProperties() {
    Super::PostInitProperties();
    if (!IsLoadedForParticleModule(this)) return;
    Min = FVector(DefaultValue);
    Max = FVector(DefaultValue);
}

void UDistributionVectorUniform::PostLoad() {
    Super::PostLoad();
    if (Min == FVector(DefaultValue)) Min = FVector::ZeroVector;
    if (Max == FVector(DefaultValue)) Max = FVector::ZeroVector;
}

FVector UDistributionVectorUniform::GetValue(float F, UObject* Data, int32 Extreme, FRandomStream* InRandomStream) const {
    const FVector LocalMax = Max;
    FVector LocalMin = Min;
    for (int32 i = 0; i < 3; i++) {
        if (MirrorFlags[i] == EDVMF_Same) LocalMin[i] = LocalMax[i];
        if (MirrorFlags[i] == EDVMF_Mirror) LocalMin[i] = -LocalMax[i];
    }
    bool bMin = true;
    if (bUseExtremes) {
        if (Extreme == 0) {
            if (GetRandomValue(InRandomStream) > 0.5f) bMin = false;
        } else if (Extreme > 0) {
            bMin = false;
        }
    }
    const auto Axis = [&](int32 i) -> float {
        if (bUseExtremes) return float(bMin ? LocalMin[i] : LocalMax[i]);
        return float(LocalMax[i] + (LocalMin[i] - LocalMax[i]) * GetRandomValue(InRandomStream));
    };
    float fX, fY, fZ;
    switch (LockedAxes) {
    case EDVLF_XY:
        fX = Axis(0);
        fZ = Axis(2);
        fY = fX;
        break;
    case EDVLF_XZ:
        fX = Axis(0);
        fY = Axis(1);
        fZ = fX;
        break;
    case EDVLF_YZ:
        fX = Axis(0);
        fY = Axis(1);
        fZ = fY;
        break;
    case EDVLF_XYZ:
        fX = Axis(0);
        fY = fX;
        fZ = fX;
        break;
    default:
        fX = Axis(0);
        fY = Axis(1);
        fZ = Axis(2);
        break;
    }
    return FVector(fX, fY, fZ);
}

void UDistributionVectorUniform::GetOutRange(float& MinOut, float& MaxOut) const {
    const FVector LocalMax = Max;
    FVector LocalMin = Min;
    for (int32 i = 0; i < 3; i++) {
        if (MirrorFlags[i] == EDVMF_Same) LocalMin[i] = LocalMax[i];
        if (MirrorFlags[i] == EDVMF_Mirror) LocalMin[i] = -LocalMax[i];
    }
    MinOut = float(LockAxes(LocalMin, LockedAxes).GetMin());
    MaxOut = float(LockAxes(LocalMax, LockedAxes).GetMax());
}

void UDistributionVectorUniform::GetRange(FVector& OutMin, FVector& OutMax) const {
    OutMin = Min;
    OutMax = Max;
}

FVector UDistributionVectorParameterBase::GetValue(float F, UObject* Data, int32 Extreme, FRandomStream* InRandomStream) const {
    FVector ParamVector(0.0f);
    if (!GetParamValue(Data, ParameterName, ParamVector)) ParamVector = Constant;
    for (int32 i = 0; i < 3; i++)
        if (ParamModes[i] == DPM_Abs) ParamVector[i] = FMath::Abs(ParamVector[i]);
    FVector Output;
    for (int32 i = 0; i < 3; i++) {
        const double Gradient = MaxInput[i] <= MinInput[i] ? 0.0 : (MaxOutput[i] - MinOutput[i]) / (MaxInput[i] - MinInput[i]);
        const double ClampedParam = FMath::Clamp(ParamVector[i], MinInput[i], MaxInput[i]);
        Output[i] = ParamModes[i] == DPM_Direct ? ParamVector[i] : MinOutput[i] + (ClampedParam - MinInput[i]) * Gradient;
    }
    return Output;
}

bool UDistributionVectorParticleParameter::GetParamValue(UObject* Data, FName ParamName, FVector& OutVector) const {
    UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(Data);
    return ParticleComp != nullptr && ParticleComp->GetAnyVectorParameter(ParameterName, OutVector);
}

}

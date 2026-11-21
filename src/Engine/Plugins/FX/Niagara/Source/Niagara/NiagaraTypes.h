#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "NiagaraTypes.generated.h"

namespace ue5parse::runtime {

class UEnum;
class UStruct;

USTRUCT()
struct FNiagaraWildcard {
    GENERATED_BODY()
};

USTRUCT()
struct FNiagaraFloat {
    GENERATED_BODY()

    UPROPERTY()
    float Value = 0;
};

USTRUCT()
struct FNiagaraInt32 {
    GENERATED_BODY()

    UPROPERTY()
    int32 Value = 0;
};

USTRUCT()
struct FNiagaraBool {
    GENERATED_BODY()

    UPROPERTY()
    int32 Value = 0;
};

USTRUCT()
struct FNiagaraHalf {
    GENERATED_BODY()

    UPROPERTY()
    uint16 Value = 0;
};

USTRUCT()
struct FNiagaraHalfVector2 {
    GENERATED_BODY()

    UPROPERTY()
    uint16 x = 0;
    UPROPERTY()
    uint16 y = 0;
};

USTRUCT()
struct FNiagaraHalfVector3 {
    GENERATED_BODY()

    UPROPERTY()
    uint16 x = 0;
    UPROPERTY()
    uint16 y = 0;
    UPROPERTY()
    uint16 z = 0;
};

USTRUCT()
struct FNiagaraHalfVector4 {
    GENERATED_BODY()

    UPROPERTY()
    uint16 x = 0;
    UPROPERTY()
    uint16 y = 0;
    UPROPERTY()
    uint16 z = 0;
    UPROPERTY()
    uint16 w = 0;
};

USTRUCT()
struct FNiagaraNumeric {
    GENERATED_BODY()
};

USTRUCT()
struct FNiagaraParameterMap {
    GENERATED_BODY()
};

USTRUCT()
struct FNiagaraDouble {
    GENERATED_BODY()

    UPROPERTY()
    double Value = 0;
};

USTRUCT()
struct FNiagaraEmitterID {
    GENERATED_BODY()

    UPROPERTY()
    int32 ID = INDEX_NONE;
};

USTRUCT()
struct FNiagaraSpawnInfo {
    GENERATED_BODY()

    UPROPERTY()
    int32 Count = 0;
    UPROPERTY()
    float InterpStartDt = 0;
    UPROPERTY()
    float IntervalDt = 1;
    UPROPERTY()
    int32 SpawnGroup = 0;
};

USTRUCT()
struct FNiagaraID {
    GENERATED_BODY()

    UPROPERTY()
    int32 Index = 0;
    UPROPERTY()
    int32 AcquireTag = 0;
};

USTRUCT()
struct FNiagaraRandInfo {
    GENERATED_BODY()

    UPROPERTY()
    int32 Seed1 = 0;
    UPROPERTY()
    int32 Seed2 = 0;
    UPROPERTY()
    int32 Seed3 = 0;
};

USTRUCT()
struct FNiagaraTypeDefinition {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UObject> ClassStructOrEnum = nullptr;
    UPROPERTY()
    uint16 UnderlyingType = 0;
    UPROPERTY()
    uint8 Flags = 0;
    UPROPERTY()
    TObjectPtr<UStruct> Struct = nullptr;
    UPROPERTY()
    TObjectPtr<UEnum> Enum = nullptr;

    bool operator==(const FNiagaraTypeDefinition&) const = default;
};

USTRUCT()
struct FNiagaraTypeDefinitionHandle {
    GENERATED_BODY()

    UPROPERTY()
    int32 RegisteredTypeIndex = INDEX_NONE;

    bool operator==(const FNiagaraTypeDefinitionHandle&) const = default;
};

USTRUCT()
struct FNiagaraVariableBase {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    FNiagaraTypeDefinitionHandle TypeDefHandle;
    //! Before 4.26 the type is a tagged TypeDef, from 4.26 the handle is stored as the type definition it refers to.
    UPROPERTY()
    FNiagaraTypeDefinition TypeDef;

    bool Serialize(FArchive& Ar);
    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
    bool operator==(const FNiagaraVariableBase&) const = default;
    friend uint32 GetTypeHash(const FNiagaraVariableBase& Variable) { return GetTypeHash(Variable.Name); }
};

template <>
struct TStructOpsTypeTraits<FNiagaraVariableBase> : TStructOpsTypeTraitsBase2<FNiagaraVariableBase> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true, WithSerializeFromMismatchedTag = true };
};

USTRUCT()
struct FNiagaraVariable : public FNiagaraVariableBase {
    GENERATED_BODY()

    UPROPERTY()
    TArray<uint8> VarData;

    bool Serialize(FArchive& Ar);
    bool operator==(const FNiagaraVariable&) const = default;
};

template <>
struct TStructOpsTypeTraits<FNiagaraVariable> : TStructOpsTypeTraitsBase2<FNiagaraVariable> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

class UNiagaraEmitter;

USTRUCT()
struct FVersionedNiagaraEmitter {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UNiagaraEmitter> Emitter = nullptr;
    UPROPERTY()
    FGuid Version;
};

UENUM()
enum class ENiagaraNumericOutputTypeSelectionMode : uint8 { None, Largest, Smallest, Scalar, Custom };

UENUM()
enum class ENiagaraExecutionState : uint32 { Active, Inactive, InactiveClear, Complete, Disabled, Num };

USTRUCT()
struct FNiagaraTypeLayoutInfo {
    GENERATED_BODY()

    UPROPERTY()
    TArray<uint32> FloatComponentByteOffsets;
    UPROPERTY()
    TArray<uint32> FloatComponentRegisterOffsets;
    UPROPERTY()
    TArray<uint32> Int32ComponentByteOffsets;
    UPROPERTY()
    TArray<uint32> Int32ComponentRegisterOffsets;
    UPROPERTY()
    TArray<uint32> HalfComponentByteOffsets;
    UPROPERTY()
    TArray<uint32> HalfComponentRegisterOffsets;
    UPROPERTY()
    uint16 NumFloatComponents = 0;
    UPROPERTY()
    uint16 NumInt32Components = 0;
    UPROPERTY()
    uint16 NumHalfComponents = 0;
    UPROPERTY()
    TArray<uint32> ComponentsOffsets;
};

USTRUCT()
struct FNiagaraAssetVersion {
    GENERATED_BODY()

    UPROPERTY()
    int32 MajorVersion = 1;
    UPROPERTY()
    int32 MinorVersion = 0;
    UPROPERTY()
    FGuid VersionGuid;
    UPROPERTY()
    bool bIsVisibleInVersionSelector = true;
};

UENUM()
enum class ENiagaraPythonUpdateScriptReference : uint8 { None, ScriptAsset, DirectTextEntry };

UENUM()
enum class ENiagaraExecutionStateManagement : uint32 { Awaken, SleepAndLetParticlesFinish, SleepAndClearParticles, KillImmediately, KillAfterParticlesFinish, Num };

UENUM()
enum class ENiagaraStructConversionType : uint8 { CopyOnly, DoubleToFloat, Vector2, Vector3, Vector4, Quat };

USTRUCT()
struct FNiagaraStructConversionStep {
    GENERATED_BODY()

    UPROPERTY()
    int32 SourceBytes = 0;
    UPROPERTY()
    int32 SourceOffset = 0;
    UPROPERTY()
    int32 SimulationBytes = 0;
    UPROPERTY()
    int32 SimulationOffset = 0;
    UPROPERTY()
    ENiagaraStructConversionType ConversionType = ENiagaraStructConversionType::CopyOnly;
    UPROPERTY()
    int32 LWCBytes = 0;
    UPROPERTY()
    int32 LWCOffset = 0;

    bool operator==(const FNiagaraStructConversionStep&) const = default;
};

USTRUCT()
struct FNiagaraLwcStructConverter {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraStructConversionStep> ConversionSteps;
    UPROPERTY()
    int32 LWCSize = 0;
    UPROPERTY()
    int32 SWCSize = 0;

    bool operator==(const FNiagaraLwcStructConverter&) const = default;
};

USTRUCT()
struct FNiagaraVariableWithOffset : public FNiagaraVariableBase {
    GENERATED_BODY()

    UPROPERTY()
    int32 Offset = -1;
    UPROPERTY()
    FNiagaraLwcStructConverter StructConverter;
    //! Before 4.25 it derives from FNiagaraVariable, so tagged data also carries VarData.
    UPROPERTY()
    TArray<uint8> VarData;

    bool Serialize(FArchive& Ar);
    bool operator==(const FNiagaraVariableWithOffset&) const = default;
};

template <>
struct TStructOpsTypeTraits<FNiagaraVariableWithOffset> : TStructOpsTypeTraitsBase2<FNiagaraVariableWithOffset> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}

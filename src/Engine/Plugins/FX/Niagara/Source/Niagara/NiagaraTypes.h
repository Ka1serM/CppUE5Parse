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
    bool operator==(const FNiagaraVariableBase&) const = default;
};

template <>
struct TStructOpsTypeTraits<FNiagaraVariableBase> : TStructOpsTypeTraitsBase2<FNiagaraVariableBase> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
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

USTRUCT()
struct FNiagaraVariableWithOffset : public FNiagaraVariableBase {
    GENERATED_BODY()

    UPROPERTY()
    int32 Offset = 0;

    bool Serialize(FArchive& Ar);
    bool operator==(const FNiagaraVariableWithOffset&) const = default;
};

template <>
struct TStructOpsTypeTraits<FNiagaraVariableWithOffset> : TStructOpsTypeTraitsBase2<FNiagaraVariableWithOffset> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}

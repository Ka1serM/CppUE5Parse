#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "NiagaraCommon.generated.h"

namespace ue5parse::runtime {

class UNiagaraDataInterface;
class UObject;

UENUM()
enum class ENiagaraScriptUsage : uint8 { Function, Module, DynamicInput, ParticleSpawnScript, ParticleSpawnScriptInterpolated, ParticleUpdateScript, ParticleEventScript, ParticleSimulationStageScript, ParticleGPUComputeScript, EmitterSpawnScript, EmitterUpdateScript, SystemSpawnScript, SystemUpdateScript };

USTRUCT()
struct FNiagaraScriptDataKey {
    GENERATED_BODY()

    UPROPERTY()
    FGuid EmitterHandleId;
    UPROPERTY()
    ENiagaraScriptUsage ScriptUsage = ENiagaraScriptUsage::Function;
    UPROPERTY()
    FGuid ScriptUsageId;

    bool operator==(const FNiagaraScriptDataKey&) const = default;
    friend uint32 GetTypeHash(const FNiagaraScriptDataKey& Key) { return HashCombine(GetTypeHash(Key.EmitterHandleId), HashCombine(static_cast<uint32>(Key.ScriptUsage), GetTypeHash(Key.ScriptUsageId))); }
};

UENUM()
enum class ENiagaraBindingSource : int { ImplicitFromSource = 0, ExplicitParticles, ExplicitEmitter, ExplicitSystem, ExplicitUser, MaxBindingSource };

USTRUCT()
struct FNiagaraVariableAttributeBinding {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraVariable BoundVariable;
    UPROPERTY()
    FNiagaraVariable DataSetVariable;
    UPROPERTY()
    FNiagaraVariable DefaultValueIfNonExistent;
    UPROPERTY()
    FNiagaraVariableBase ParamMapVariable;
    UPROPERTY()
    FNiagaraVariable RootVariable;
    UPROPERTY()
    TEnumAsByte<ENiagaraBindingSource> BindingSourceMode;
    UPROPERTY()
    uint8 bBindingExistsOnSource : 1 = 0;
    UPROPERTY()
    uint8 bIsCachedParticleValue : 1 = 0;
    UPROPERTY()
    FName DataSetName;
    UPROPERTY()
    FName RootName;
};

USTRUCT()
struct FNiagaraUserParameterBinding {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraVariable Parameter;
};

USTRUCT()
struct FNiagaraMaterialAttributeBinding {
    GENERATED_BODY()

    UPROPERTY()
    FName MaterialParameterName;
    UPROPERTY()
    FNiagaraVariableBase NiagaraVariable;
    UPROPERTY()
    FNiagaraVariableBase ResolvedNiagaraVariable;
    UPROPERTY()
    FNiagaraVariableBase NiagaraChildVariable;
};

UENUM()
enum class ENiagaraRendererSourceDataMode : uint8 { Particles = 0, Emitter };

USTRUCT()
struct FNiagaraScriptDataInterfaceInfo {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UNiagaraDataInterface> DataInterface = nullptr;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    int32 UserPtrIdx = 0;
    UPROPERTY()
    FNiagaraTypeDefinition Type;
    UPROPERTY()
    FName RegisteredParameterMapRead;
    UPROPERTY()
    FName RegisteredParameterMapWrite;
    UPROPERTY()
    FName CompileName;
    UPROPERTY()
    FString SourceEmitterName;
};

USTRUCT()
struct FNiagaraResolvedUserDataInterfaceBinding {
    GENERATED_BODY()

    UPROPERTY()
    int32 UserParameterStoreDataInterfaceIndex = 0;
    UPROPERTY()
    int32 ScriptParameterStoreDataInterfaceIndex = 0;
};

USTRUCT()
struct FNiagaraScriptResolvedDataInterfaceInfo {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName CompileName;
    UPROPERTY()
    FString ResolvedSourceEmitterName;
    UPROPERTY()
    FNiagaraVariableBase ResolvedVariable;
    UPROPERTY()
    FNiagaraVariableBase ParameterStoreVariable;
    UPROPERTY()
    bool bIsInternal = false;
    UPROPERTY()
    TObjectPtr<UNiagaraDataInterface> ResolvedDataInterface = nullptr;
    UPROPERTY()
    int32 UserPtrIdx = 0;
};

USTRUCT()
struct FNiagaraResolvedUObjectInfo {
    GENERATED_BODY()

    UPROPERTY()
    FName ReadVariableName;
    UPROPERTY()
    FNiagaraVariableBase ResolvedVariable;
    UPROPERTY()
    TObjectPtr<UObject> Object = nullptr;
};

UENUM()
enum class ENiagaraRendererMotionVectorSetting : uint8 { AutoDetect, Precise, Approximate, Disable };

UENUM()
enum class ENiagaraDataSetType : uint8 { ParticleData, Shared, Event };

USTRUCT()
struct FNiagaraDataSetID {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    ENiagaraDataSetType Type = ENiagaraDataSetType::ParticleData;
};

UENUM()
enum class ENiagaraSimTarget : uint8 { CPUSim, GPUComputeSim, DynamicLoadBalancedSim };

UENUM()
enum class ENiagaraInterpolatedSpawnMode : uint8 { NoInterpolation = 0, RunUpdateScript, Interpolation };

USTRUCT()
struct FNiagaraFunctionSignature {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    TArray<FNiagaraVariable> Inputs;
    UPROPERTY()
    TArray<FNiagaraVariableBase> Outputs;
    UPROPERTY()
    FName OwnerName;
    UPROPERTY()
    uint32 bRequiresContext : 1 = 0;
    UPROPERTY()
    uint32 bMemberFunction : 1 = 0;
    UPROPERTY()
    TMap<FName, FName> FunctionSpecifiers;
    UPROPERTY()
    uint32 bExperimental : 1 = 0;
    UPROPERTY()
    uint32 bSupportsCPU : 1 = 0;
    UPROPERTY()
    uint32 bSupportsGPU : 1 = 0;
    UPROPERTY()
    uint32 bWriteFunction : 1 = 0;
    UPROPERTY()
    uint32 bRequiresExecPin : 1 = 0;
    UPROPERTY()
    uint32 bSoftDeprecatedFunction : 1 = 0;
    UPROPERTY()
    int32 ModuleUsageBitmask = 0;
    UPROPERTY()
    int32 ContextStageMinIndex = 0;
    UPROPERTY()
    int32 ContextStageMaxIndex = 0;
    UPROPERTY()
    uint32 bIsCompileTagGenerator : 1 = 0;
    UPROPERTY()
    uint32 bHidden : 1 = 0;
    UPROPERTY()
    int32 ContextStageIndex = 0;
    UPROPERTY()
    uint32 bReadFunction : 1 = 0;
    UPROPERTY()
    int16 RequiredInputs = 0;
    UPROPERTY()
    int16 RequiredOutputs = 0;
    UPROPERTY()
    uint16 MiscUsageBitMask = 0;
};

USTRUCT()
struct FNiagaraScriptDataInterfaceCompileInfo {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    int32 UserPtrIdx = 0;
    UPROPERTY()
    FNiagaraTypeDefinition Type;
    UPROPERTY()
    TArray<FNiagaraFunctionSignature> RegisteredFunctions;
    UPROPERTY()
    FName RegisteredParameterMapRead;
    UPROPERTY()
    FName RegisteredParameterMapWrite;
    UPROPERTY()
    bool bIsPlaceholder = false;
    UPROPERTY()
    FString SourceEmitterName;
};

UENUM()
enum class ENiagaraScriptCompileStatus : uint8 { NCS_Unknown, NCS_Dirty, NCS_Error, NCS_UpToDate, NCS_BeingCreated, NCS_UpToDateWithWarnings, NCS_ComputeUpToDateWithWarnings, NCS_MAX };

USTRUCT()
struct FNiagaraDataSetProperties {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraDataSetID ID;
    UPROPERTY()
    TArray<FNiagaraVariableBase> Variables;
};

USTRUCT()
struct FNiagaraStatScope {
    GENERATED_BODY()

    UPROPERTY()
    FName FullName;
    UPROPERTY()
    FName FriendlyName;
};

USTRUCT()
struct FVMFunctionSpecifier {
    GENERATED_BODY()

    UPROPERTY()
    FName Key;
    UPROPERTY()
    FName Value;
};

USTRUCT()
struct FVMExternalFunctionBindingInfo {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName OwnerName;
    UPROPERTY()
    TArray<bool> InputParamLocations;
    UPROPERTY()
    int32 NumOutputs = 0;
    UPROPERTY()
    TMap<FName, FName> Specifiers;
    UPROPERTY()
    TArray<FVMFunctionSpecifier> FunctionSpecifiers;
    UPROPERTY()
    TArray<FNiagaraVariableBase> VariadicInputs;
    UPROPERTY()
    TArray<FNiagaraVariableBase> VariadicOutputs;
};

USTRUCT()
struct FNiagaraScriptDataUsageInfo {
    GENERATED_BODY()

    UPROPERTY()
    bool bReadsAttributeData = false;
};

USTRUCT()
struct FNiagaraScriptUObjectCompileInfo {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraVariableBase Variable;
    UPROPERTY()
    TObjectPtr<UObject> Object = nullptr;
    UPROPERTY()
    FSoftObjectPath ObjectPath;
    UPROPERTY()
    FName RegisteredParameterMapRead;
    UPROPERTY()
    TArray<FName> RegisteredParameterMapWrites;
};

USTRUCT()
struct FNiagaraExternalUObjectInfo {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraVariableBase Variable;
    UPROPERTY()
    FName ExternalName;
};

}

#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCommon.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCompileHash.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraParameterStore.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraParameters.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraScriptBase.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraScriptExecutionParameterStore.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraShaderMap.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraShared.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraUserRedirectionParameterStore.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraScript.generated.h"

namespace ue5parse::runtime {

class UNiagaraParameterCollection;

USTRUCT()
struct FNiagaraCompilerTag {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraVariable Variable;
    UPROPERTY()
    FString StringValue;
};

USTRUCT()
struct FNiagaraVMExecutableByteCode {
    GENERATED_BODY()

    UPROPERTY()
    TArray<uint8> Data;
    UPROPERTY()
    int32 UncompressedSize = INDEX_NONE;

    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FNiagaraVMExecutableByteCode> : TStructOpsTypeTraitsBase2<FNiagaraVMExecutableByteCode> {
    enum { WithSerializeFromMismatchedTag = true };
};

USTRUCT()
struct FNiagaraVMExecutableData {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraVMExecutableByteCode ByteCode;
    UPROPERTY()
    int32 NumUserPtrs = 0;
    UPROPERTY()
    FNiagaraParameters Parameters;
    UPROPERTY()
    FNiagaraParameters InternalParameters;
    UPROPERTY()
    TMap<FName, FNiagaraParameters> DataSetToParameters;
    UPROPERTY()
    TArray<FNiagaraVariableBase> Attributes;
    UPROPERTY()
    FNiagaraScriptDataUsageInfo DataUsage;
    UPROPERTY()
    TArray<FNiagaraScriptDataInterfaceCompileInfo> DataInterfaceInfo;
    UPROPERTY()
    TArray<FVMExternalFunctionBindingInfo> CalledVMExternalFunctions;
    UPROPERTY()
    TArray<FNiagaraDataSetID> ReadDataSets;
    UPROPERTY()
    TArray<FNiagaraDataSetProperties> WriteDataSets;
    UPROPERTY()
    TArray<FNiagaraStatScope> StatScopes;
    UPROPERTY()
    FString LastHlslTranslation;
    UPROPERTY()
    FString LastHlslTranslationGPU;
    UPROPERTY()
    FString LastAssemblyTranslation;
    UPROPERTY()
    uint32 LastOpCount = 0;
    UPROPERTY()
    TArray<FNiagaraDataInterfaceGPUParamInfo> DIParamInfo;
    UPROPERTY()
    TArray<FString> ParameterCollectionPaths;
    UPROPERTY()
    ENiagaraScriptCompileStatus LastCompileStatus = ENiagaraScriptCompileStatus::NCS_Unknown;
    UPROPERTY()
    bool bReadsAttributeData = false;
    UPROPERTY()
    FString ErrorMsg;
    UPROPERTY()
    float CompileTime = 0;
    UPROPERTY()
    int32 NumTempRegisters = 0;
    UPROPERTY()
    TArray<FNiagaraFunctionSignature> AdditionalExternalFunctions;
    UPROPERTY()
    FNiagaraVMExecutableByteCode OptimizedByteCode;
    UPROPERTY()
    TArray<FSimulationStageMetaData> SimulationStageMetaData;
    UPROPERTY()
    TArray<uint8> ScriptLiterals;
    UPROPERTY()
    uint32 bReadsSignificanceIndex : 1 = 0;
    UPROPERTY()
    TArray<FNiagaraCompilerTag> CompileTags;
    UPROPERTY()
    uint32 bNeedsGPUContextInit : 1 = 0;
    UPROPERTY()
    FNiagaraShaderScriptParametersMetadata ShaderScriptParametersMetadata;
    UPROPERTY()
    TArray<uint8> ExperimentalContextData;
    UPROPERTY()
    TArray<FNiagaraScriptUObjectCompileInfo> UObjectInfos;
};

USTRUCT()
struct FNiagaraVMExecutableDataId {
    GENERATED_BODY()

    UPROPERTY()
    FGuid CompilerVersionID;
    UPROPERTY()
    ENiagaraScriptUsage ScriptUsageType = ENiagaraScriptUsage::Function;
    UPROPERTY()
    FGuid ScriptUsageTypeID;
    UPROPERTY()
    TArray<FString> AdditionalDefines;
    UPROPERTY()
    FGuid BaseScriptID;
    UPROPERTY()
    TArray<FGuid> ReferencedDependencyIds;
    UPROPERTY()
    uint32 DetailLevelMask = 0;
    UPROPERTY()
    uint32 bUsesRapidIterationParams : 1 = 0;
    UPROPERTY()
    uint32 bInterpolatedSpawn : 1 = 0;
    UPROPERTY()
    uint32 bRequiresPersistentIDs : 1 = 0;
    UPROPERTY()
    FNiagaraCompileHash BaseScriptCompileHash;
    UPROPERTY()
    FGuid ScriptVersionID;
    UPROPERTY()
    uint32 bDisableDebugSwitches : 1 = 0;
    UPROPERTY()
    ENiagaraInterpolatedSpawnMode InterpolatedSpawnMode = ENiagaraInterpolatedSpawnMode::NoInterpolation;
};

UCLASS()
class UNiagaraScript : public UNiagaraScriptBase {
    GENERATED_BODY()
public:
    explicit UNiagaraScript(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
    void Serialize(FArchive& Ar) override;
    TArray<FNiagaraShaderScript> LoadedScriptResources;

    UPROPERTY()
    ENiagaraScriptUsage Usage = ENiagaraScriptUsage::Function;
    UPROPERTY()
    int32 UsageIndex = 0;
    UPROPERTY()
    FGuid UsageId;
    UPROPERTY()
    int32 ModuleUsageBitmask = 0;
    UPROPERTY()
    FNiagaraParameterStore RapidIterationParameters;
    UPROPERTY()
    ENiagaraNumericOutputTypeSelectionMode NumericOutputTypeSelectionMode = ENiagaraNumericOutputTypeSelectionMode::None;
    UPROPERTY()
    FNiagaraScriptExecutionParameterStore ScriptExecutionParamStoreCPU;
    UPROPERTY()
    FNiagaraScriptExecutionParameterStore ScriptExecutionParamStoreGPU;
    UPROPERTY()
    FNiagaraVMExecutableDataId CachedScriptVMId;
    UPROPERTY()
    FNiagaraVMExecutableDataId LastGeneratedVMId;
    UPROPERTY()
    FNiagaraVMExecutableData CachedScriptVM;
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraParameterCollection>> CachedParameterCollectionReferences;
    UPROPERTY()
    TArray<FNiagaraScriptDataInterfaceInfo> CachedDefaultDataInterfaces;
    UPROPERTY()
    FNiagaraScriptExecutionParameterStore ScriptExecutionParamStore;
    UPROPERTY()
    TArray<FNiagaraBoundParameter> ScriptExecutionBoundParameters;
    UPROPERTY()
    TArray<FNiagaraScriptResolvedDataInterfaceInfo> ResolvedDataInterfaces;
    UPROPERTY()
    TArray<FNiagaraResolvedUserDataInterfaceBinding> ResolvedUserDataInterfaceBindings;
    UPROPERTY()
    TArray<FNiagaraResolvedUObjectInfo> ResolvedUObjectInfos;
};

}

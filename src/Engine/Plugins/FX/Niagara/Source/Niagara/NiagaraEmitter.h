#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCommon.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraDataInterfaceEmitterBinding.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraDataSetCompiledData.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraEffectType.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraEmitterBase.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraPlatformSet.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraScalabilitySettings.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraSimStageExecutionData.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraSystemFastPath.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraUserRedirectionParameterStore.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Containers/Set.h"
#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "NiagaraEmitter.generated.h"

namespace ue5parse::runtime {

class UNiagaraEmitter;
class UNiagaraRendererProperties;
class UNiagaraScript;
class UNiagaraSimulationStageBase;

UENUM()
enum class EParticleAllocationMode : uint8 { AutomaticEstimate = 0, ManualEstimate, FixedCount };

USTRUCT()
struct FNiagaraDetailsLevelScaleOverrides {
    GENERATED_BODY()

    UPROPERTY()
    float Low = 0;
    UPROPERTY()
    float Medium = 0;
    UPROPERTY()
    float High = 0;
    UPROPERTY()
    float Epic = 0;
    UPROPERTY()
    float Cine = 0;
};

USTRUCT()
struct FNiagaraEventGeneratorProperties {
    GENERATED_BODY()

    UPROPERTY()
    int32 MaxEventsPerFrame = 64;
    UPROPERTY()
    FNiagaraDataSetProperties SetProps;
    UPROPERTY()
    FName ID;
    UPROPERTY()
    FNiagaraDataSetCompiledData DataSetCompiledData;

    //! Roco Kingdom World saves the event count and ID without property tags, then the compiled data as tagged properties.
    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_RocoKingdomWorld)
            return false;
        Ar << MaxEventsPerFrame << ID;
        FNiagaraDataSetCompiledData::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(&DataSetCompiledData));
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FNiagaraEventGeneratorProperties> : TStructOpsTypeTraitsBase2<FNiagaraEventGeneratorProperties> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FNiagaraEventReceiverProperties {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName SourceEventGenerator;
    UPROPERTY()
    FName SourceEmitter;
};

USTRUCT()
struct FNiagaraEmitterScriptProperties {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UNiagaraScript> Script = nullptr;
    UPROPERTY()
    TArray<FNiagaraEventReceiverProperties> EventReceivers;
    UPROPERTY()
    TArray<FNiagaraEventGeneratorProperties> EventGenerators;
};

UENUM()
enum class EScriptExecutionMode : uint8 { EveryParticle = 0, SpawnedParticles, SingleParticle };

USTRUCT()
struct FNiagaraEventScriptProperties : public FNiagaraEmitterScriptProperties {
    GENERATED_BODY()

    UPROPERTY()
    EScriptExecutionMode ExecutionMode = EScriptExecutionMode::EveryParticle;
    UPROPERTY()
    uint32 SpawnNumber = 0;
    UPROPERTY()
    uint32 MaxEventsPerFrame = 0;
    UPROPERTY()
    FGuid SourceEmitterID;
    UPROPERTY()
    FName SourceEventName;
    UPROPERTY()
    bool bRandomSpawnNumber = false;
    UPROPERTY()
    uint32 MinSpawnNumber = 0;
    UPROPERTY()
    bool UpdateAttributeInitialValues = false;
};

UENUM()
enum class ENiagaraEmitterCalculateBoundMode : uint8 { Dynamic, Fixed, Programmable };

USTRUCT()
struct FVersionedNiagaraEmitterData {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraAssetVersion Version;
    UPROPERTY()
    FText VersionChangeDescription;
    UPROPERTY()
    ENiagaraPythonUpdateScriptReference UpdateScriptExecution = ENiagaraPythonUpdateScriptReference::None;
    UPROPERTY()
    FString PythonUpdateScript;
    UPROPERTY()
    FFilePath ScriptAsset;
    UPROPERTY()
    bool bDeprecated = false;
    UPROPERTY()
    FText DeprecationMessage;
    UPROPERTY()
    bool bLocalSpace = false;
    UPROPERTY()
    bool bDeterminism = false;
    UPROPERTY()
    int32 RandomSeed = 0;
    UPROPERTY()
    uint32 bInterpolatedSpawning : 1 = 0;
    UPROPERTY()
    ENiagaraSimTarget SimTarget = ENiagaraSimTarget::CPUSim;
    UPROPERTY()
    ENiagaraEmitterCalculateBoundMode CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Dynamic;
    UPROPERTY()
    FBox FixedBounds;
    UPROPERTY()
    uint32 bRequiresPersistentIDs : 1 = 0;
    UPROPERTY()
    uint32 bCombineEventSpawn : 1 = 0;
    UPROPERTY()
    TArray<FNiagaraEventScriptProperties> EventHandlerScriptProps;
    UPROPERTY()
    FNiagaraPlatformSet Platforms;
    UPROPERTY()
    FNiagaraEmitterScalabilityOverrides ScalabilityOverrides;
    UPROPERTY()
    uint32 bLimitDeltaTime : 1 = 0;
    UPROPERTY()
    int32 MaxGPUParticlesSpawnPerFrame = 0;
    UPROPERTY()
    float MaxDeltaTimePerTick = 0.125;
    UPROPERTY()
    EParticleAllocationMode AllocationMode = EParticleAllocationMode::AutomaticEstimate;
    UPROPERTY()
    int32 PreAllocationCount = 0;
    UPROPERTY()
    FNiagaraEmitterScriptProperties UpdateScriptProps;
    UPROPERTY()
    FNiagaraEmitterScriptProperties SpawnScriptProps;
    UPROPERTY()
    FNiagaraParameterStore RendererBindings;
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraRendererProperties>> RendererProperties;
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraSimulationStageBase>> SimulationStages;
    UPROPERTY()
    TObjectPtr<UNiagaraScript> GPUComputeScript = nullptr;
    UPROPERTY()
    TArray<FName> SharedEventGeneratorIds;
    UPROPERTY()
    FNiagaraEmitterScalabilitySettings CurrentScalabilitySettings;
    UPROPERTY()
    TArray<FNiagaraExternalUObjectInfo> RendererBindingsExternalObjects;
    UPROPERTY()
    TMap<FNiagaraVariableBase, FNiagaraVariableBase> ResolvedDIBindings;
    UPROPERTY()
    TArray<FNiagaraSimStageExecutionLoopData> SimStageExecutionLoops;
    UPROPERTY()
    TArray<FNiagaraDataInterfaceEmitterBinding> EmitterDependencies;
    UPROPERTY()
    ENiagaraInterpolatedSpawnMode InterpolatedSpawnMode = ENiagaraInterpolatedSpawnMode::NoInterpolation;
    UPROPERTY()
    ENiagaraEmitterImportance Importance = ENiagaraEmitterImportance::Normal;
};

UCLASS()
class UNiagaraEmitter : public UNiagaraEmitterBase {
    GENERATED_BODY()
public:
    explicit UNiagaraEmitter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    bool bLocalSpace = false;
    UPROPERTY()
    FNiagaraEmitterScriptProperties UpdateScriptProps;
    UPROPERTY()
    FNiagaraEmitterScriptProperties SpawnScriptProps;
    UPROPERTY()
    FNiagaraEmitterScriptProperties EmitterSpawnScriptProps;
    UPROPERTY()
    FNiagaraEmitterScriptProperties EmitterUpdateScriptProps;
    UPROPERTY()
    ENiagaraSimTarget SimTarget = ENiagaraSimTarget::CPUSim;
    UPROPERTY()
    FBox FixedBounds;
    UPROPERTY()
    int32 MinDetailLevel = 0;
    UPROPERTY()
    int32 MaxDetailLevel = 0;
    UPROPERTY()
    uint32 bInterpolatedSpawning : 1 = 0;
    UPROPERTY()
    uint32 bFixedBounds : 1 = 0;
    UPROPERTY()
    uint32 bUseMinDetailLevel : 1 = 0;
    UPROPERTY()
    uint32 bUseMaxDetailLevel : 1 = 0;
    UPROPERTY()
    uint32 bRequiresPersistentIDs : 1 = 0;
    UPROPERTY()
    FString UniqueEmitterName;
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraRendererProperties>> RendererProperties;
    UPROPERTY()
    TArray<FNiagaraEventScriptProperties> EventHandlerScriptProps;
    UPROPERTY()
    TObjectPtr<UNiagaraScript> GPUComputeScript = nullptr;
    UPROPERTY()
    bool bDeterminism = false;
    UPROPERTY()
    int32 RandomSeed = 0;
    UPROPERTY()
    float MaxDeltaTimePerTick = 0;
    UPROPERTY()
    uint32 bLimitDeltaTime : 1 = 0;
    UPROPERTY()
    TArray<FName> SharedEventGeneratorIds;
    UPROPERTY()
    TObjectPtr<UNiagaraEmitter> Parent = nullptr;
    UPROPERTY()
    TObjectPtr<UNiagaraEmitter> ParentAtLastMerge = nullptr;
    UPROPERTY()
    int32 PreAllocationCount = 0;
    UPROPERTY()
    FNiagaraDetailsLevelScaleOverrides GlobalSpawnCountScaleOverrides;
    UPROPERTY()
    uint32 bOverrideGlobalSpawnCountScale : 1 = 0;
    UPROPERTY()
    uint32 MaxUpdateIterations = 0;
    UPROPERTY()
    TSet<uint32> SpawnStages;
    UPROPERTY()
    FNiagaraFastPath_Module_EmitterScalability EmitterScalability;
    UPROPERTY()
    FNiagaraFastPath_Module_EmitterLifeCycle EmitterLifeCycle;
    UPROPERTY()
    TArray<FNiagaraFastPath_Module_SpawnRate> SpawnRate;
    UPROPERTY()
    TArray<FNiagaraFastPath_Module_SpawnPerUnit> SpawnPerUnit;
    UPROPERTY()
    TArray<FNiagaraFastPath_Module_SpawnBurstInstantaneous> SpawnBurstInstantaneous;
    UPROPERTY()
    FNiagaraFastPathAttributeNames SpawnFastPathAttributeNames;
    UPROPERTY()
    FNiagaraFastPathAttributeNames UpdateFastPathAttributeNames;
    UPROPERTY()
    EParticleAllocationMode AllocationMode = EParticleAllocationMode::AutomaticEstimate;
    UPROPERTY()
    FNiagaraPlatformSet Platforms;
    UPROPERTY()
    FNiagaraEmitterScalabilityOverrides ScalabilityOverrides;
    UPROPERTY()
    uint32 DefaultShaderStageIndex = 0;
    UPROPERTY()
    uint32 bSimulationStagesEnabled : 1 = 0;
    UPROPERTY()
    uint32 bDeprecatedShaderStagesEnabled : 1 = 0;
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraSimulationStageBase>> SimulationStages;
    UPROPERTY()
    uint32 bCombineEventSpawn : 1 = 0;
    UPROPERTY()
    int32 MaxGPUParticlesSpawnPerFrame = 0;
    UPROPERTY()
    FNiagaraParameterStore RendererBindings;
    UPROPERTY()
    FGuid ExposedVersion;
    UPROPERTY()
    bool bVersioningEnabled = false;
    UPROPERTY()
    TArray<FVersionedNiagaraEmitterData> VersionData;
};

}

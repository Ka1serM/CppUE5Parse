#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCommon.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraDataSetCompiledData.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraEffectType.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraEmitterHandle.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraPlatformSet.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraScalabilitySettings.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraScriptRuntimeCookedData.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraSystemEmitterState.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraSystemFastPath.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraUserRedirectionParameterStore.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"
#include "Engine/Source/Runtime/Core/Misc/Optional.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystem.h"

#include "NiagaraSystem.generated.h"

namespace ue5parse::runtime {

class UNiagaraDataChannelDefinitions;
class UNiagaraEffectType;
class UNiagaraParameterCollectionInstance;
class UNiagaraScript;
class UObject;

UENUM()
enum class ENiagaraLwcTileUpdateMode : uint8 { ResetSimulation, Rebase, RebaseOrResetSimulation };

USTRUCT()
struct FNiagaraEmitterSpawnAttributes {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FName> SpawnAttributes;
};

USTRUCT()
struct FNiagaraSystemCompileRequest {
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<UObject>> RootObjects;
};

USTRUCT()
struct FNiagaraParameterDataSetBinding {
    GENERATED_BODY()

    UPROPERTY()
    int32 ParameterOffset = 0;
    UPROPERTY()
    int32 DataSetComponentOffset = 0;
};

USTRUCT()
struct FNiagaraParameterDataSetBindingCollection {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraParameterDataSetBinding> FloatOffsets;
    UPROPERTY()
    TArray<FNiagaraParameterDataSetBinding> Int32Offsets;
};

USTRUCT()
struct FNiagaraSystemCompiledData {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraVariable> NumParticleVars;
    UPROPERTY()
    TArray<FNiagaraVariable> TotalSpawnedParticlesVars;
    UPROPERTY()
    FNiagaraParameterStore InstanceParamStore;
    UPROPERTY()
    TArray<FNiagaraVariable> SpawnCountScaleVars;
    UPROPERTY()
    FNiagaraDataSetCompiledData DataSetCompiledData;
    UPROPERTY()
    FNiagaraDataSetCompiledData SpawnInstanceParamsDataSetCompiledData;
    UPROPERTY()
    FNiagaraDataSetCompiledData UpdateInstanceParamsDataSetCompiledData;
    UPROPERTY()
    FNiagaraParameterDataSetBindingCollection SpawnInstanceGlobalBinding;
    UPROPERTY()
    FNiagaraParameterDataSetBindingCollection SpawnInstanceSystemBinding;
    UPROPERTY()
    FNiagaraParameterDataSetBindingCollection SpawnInstanceOwnerBinding;
    UPROPERTY()
    TArray<FNiagaraParameterDataSetBindingCollection> SpawnInstanceEmitterBindings;
    UPROPERTY()
    FNiagaraParameterDataSetBindingCollection UpdateInstanceGlobalBinding;
    UPROPERTY()
    FNiagaraParameterDataSetBindingCollection UpdateInstanceSystemBinding;
    UPROPERTY()
    FNiagaraParameterDataSetBindingCollection UpdateInstanceOwnerBinding;
    UPROPERTY()
    TArray<FNiagaraParameterDataSetBindingCollection> UpdateInstanceEmitterBindings;
};

USTRUCT()
struct FNiagaraSystemScalabilityOverrides {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraSystemScalabilityOverride> Overrides;
};

USTRUCT()
struct FNiagaraEmitterCompiledData {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FName> SpawnAttributes;
    UPROPERTY()
    FNiagaraVariable EmitterSpawnIntervalVar;
    UPROPERTY()
    FNiagaraVariable EmitterInterpSpawnStartDTVar;
    UPROPERTY()
    FNiagaraVariable EmitterSpawnGroupVar;
    UPROPERTY()
    FNiagaraVariable EmitterAgeVar;
    UPROPERTY()
    FNiagaraVariable EmitterRandomSeedVar;
    UPROPERTY()
    FNiagaraVariable EmitterTotalSpawnedParticlesVar;
    UPROPERTY()
    FNiagaraDataSetCompiledData DataSetCompiledData;
    UPROPERTY()
    FNiagaraVariable EmitterInstanceSeedVar;
};

UCLASS()
class UNiagaraSystem : public UFXSystemAsset {
    GENERATED_BODY()
public:
    explicit UNiagaraSystem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    uint8 bDumpDebugSystemInfo : 1 = false;
    UPROPERTY()
    uint8 bDumpDebugEmitterInfo : 1 = false;
    UPROPERTY()
    TArray<FNiagaraEmitterHandle> EmitterHandles;
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraParameterCollectionInstance>> ParameterCollectionOverrides;
    UPROPERTY()
    TArray<FNiagaraSystemCompileRequest> ActiveCompilations;
    UPROPERTY()
    TObjectPtr<UNiagaraScript> SystemSpawnScript = nullptr;
    UPROPERTY()
    TObjectPtr<UNiagaraScript> SystemUpdateScript = nullptr;
    UPROPERTY()
    TArray<FNiagaraEmitterSpawnAttributes> EmitterSpawnAttributes;
    UPROPERTY()
    FNiagaraUserRedirectionParameterStore ExposedParameters;
    UPROPERTY()
    bool bAutoDeactivate = false;
    UPROPERTY()
    float WarmupTime = 0.0f;
    UPROPERTY()
    int32 WarmupTickCount = 0;
    UPROPERTY()
    float WarmupTickDelta = 1.0f / 15.0f;
    UPROPERTY()
    uint32 bSolo : 1 = 0;
    UPROPERTY()
    bool bHasSystemScriptDIsWithPerInstanceData = false;
    UPROPERTY()
    TArray<FName> UserDINamesReadInSystemScripts;
    UPROPERTY()
    uint32 bFixedBounds : 1 = 0;
    UPROPERTY()
    FBox FixedBounds;
    UPROPERTY()
    ENiagaraFastPathMode FastPathMode = ENiagaraFastPathMode::ScriptVMOnly;
    UPROPERTY()
    FNiagaraFastPath_Module_SystemScalability SystemScalability;
    UPROPERTY()
    FNiagaraFastPath_Module_SystemLifeCycle SystemLifeCycle;
    UPROPERTY()
    FNiagaraSystemCompiledData SystemCompiledData;
    UPROPERTY()
    TObjectPtr<UNiagaraEffectType> EffectType = nullptr;
    UPROPERTY()
    uint8 bOverrideScalabilitySettings : 1 = 0;
    UPROPERTY()
    TArray<FNiagaraSystemScalabilityOverride> ScalabilityOverrides;
    UPROPERTY()
    FNiagaraSystemScalabilityOverrides SystemScalabilityOverrides;
    UPROPERTY()
    uint8 bRequireCurrentFrameData : 1 = true;
    UPROPERTY()
    bool bNeedsGPUContextInitForDataInterfaces = false;
    UPROPERTY()
    uint8 bSupportLargeWorldCoordinates : 1 = 0;
    UPROPERTY()
    uint8 bOverrideCastShadow : 1 = 0;
    UPROPERTY()
    uint8 bOverrideReceivesDecals : 1 = 0;
    UPROPERTY()
    uint8 bOverrideRenderCustomDepth : 1 = 0;
    UPROPERTY()
    uint8 bOverrideCustomDepthStencilValue : 1 = 0;
    UPROPERTY()
    uint8 bOverrideCustomDepthStencilWriteMask : 1 = 0;
    UPROPERTY()
    uint8 bCastShadow : 1 = 0;
    UPROPERTY()
    uint8 bReceivesDecals : 1 = 0;
    UPROPERTY()
    uint8 bRenderCustomDepth : 1 = 0;
    UPROPERTY()
    ERendererStencilMask CustomDepthStencilWriteMask{};
    UPROPERTY()
    int32 CustomDepthStencilValue = 0;
    UPROPERTY()
    uint32 bOverrideAllowCullingForLocalPlayers : 1 = 0;
    UPROPERTY()
    uint32 bAllowCullingForLocalPlayersOverride : 1 = 0;
    UPROPERTY()
    bool bDeterminism = false;
    UPROPERTY()
    int32 RandomSeed = 0;
    UPROPERTY()
    bool bFixedTickDelta = false;
    UPROPERTY()
    float FixedTickDeltaTime = 0.01667f;
    UPROPERTY()
    uint8 bOverrideTranslucencySortPriority : 1 = 0;
    UPROPERTY()
    uint8 bOverrideTranslucencySortDistanceOffset : 1 = 0;
    UPROPERTY()
    uint8 bDisableExperimentalVM : 1 = 0;
    UPROPERTY()
    int32 TranslucencySortPriority = 0;
    UPROPERTY()
    float TranslucencySortDistanceOffset = 0.0f;
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraDataChannelDefinitions>> ReferencedDataChannelDefinitions;
    UPROPERTY()
    uint8 bInitialOwnerVelocityFromActor : 1 = true;
    UPROPERTY()
    FNiagaraPlatformSet Platforms;
    UPROPERTY()
    uint32 bAllowSystemStateFastPath : 1 = true;
    UPROPERTY()
    uint32 bSystemStateFastPathEnabled : 1 = true;
    UPROPERTY()
    FNiagaraSystemStateData SystemStateData;
    UPROPERTY()
    TOptional<ENiagaraLwcTileUpdateMode> LargeWorldCoordinateTileUpdateMode;
    UPROPERTY()
    bool bUseInitialStreamingBounds = false;
    UPROPERTY()
    FBox InitialStreamingBounds;
    UPROPERTY()
    FInstancedStruct SystemStateDataStruct;
    UPROPERTY()
    TMap<FNiagaraScriptDataKey, FNiagaraScriptRuntimeCookedData> ScriptRuntimeCookedDataMap;
    UPROPERTY()
    ENiagaraEmitterImportance MaxEmitterImportance = ENiagaraEmitterImportance::Low;
    TArray<FNiagaraEmitterCompiledData> EmitterCompiledData;

    void Serialize(FArchive& Ar) override;
};


}

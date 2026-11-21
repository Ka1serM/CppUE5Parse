#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraDataSetCompiledData.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraEffectType.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraEmitterBase.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraPlatformSet.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraStatelessSpawnInfo.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraSystemEmitterState.h"
#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraStatelessEmitter.generated.h"

namespace ue5parse::runtime {

class UClass;
class UNiagaraParameterCollection;
class UNiagaraRendererProperties;
class UNiagaraStatelessEmitterTemplate;
class UNiagaraStatelessModule;

UCLASS()
class UNiagaraStatelessEmitter : public UNiagaraEmitterBase {
    GENERATED_BODY()
public:
    explicit UNiagaraStatelessEmitter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FString UniqueEmitterName;
    UPROPERTY()
    TObjectPtr<UClass> EmitterTemplateClass = nullptr;
    UPROPERTY()
    uint32 bDeterministic : 1 = false;
    UPROPERTY()
    int32 RandomSeed = 0;
    UPROPERTY()
    FBox FixedBounds;
    UPROPERTY()
    FNiagaraEmitterStateData EmitterState;
    UPROPERTY()
    TArray<FNiagaraStatelessSpawnInfo> SpawnInfos;
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraStatelessModule>> Modules;
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraRendererProperties>> RendererProperties;
    UPROPERTY()
    FNiagaraPlatformSet Platforms;
    UPROPERTY()
    FNiagaraDataSetCompiledData ParticleDataSetCompiledData;
    UPROPERTY()
    TArray<int32> ComponentOffsets;
    UPROPERTY()
    uint32 AllowedFeatureMask = 0;
    UPROPERTY()
    FNiagaraEmitterScalabilityOverrides ScalabilityOverrides;
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraParameterCollection>> CachedParameterCollectionReferences;
    UPROPERTY()
    TObjectPtr<UNiagaraStatelessEmitterTemplate> EmitterTemplate = nullptr;
    UPROPERTY()
    TArray<int32> ShaderOutputVariableOffsets;
};

}

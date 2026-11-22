#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCore.h"
#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraScriptBase.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNiagaraScriptBase : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraScriptBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

};

UENUM()
enum class ENiagaraDirectDispatchElementType : uint8 { NumThreads, NumThreadsNoClipping, NumGroups };

UENUM()
enum class ENiagaraGpuDispatchType : uint8 { OneD, TwoD, ThreeD, Custom };

UENUM()
enum class ENiagaraSimStageExecuteBehavior : uint8 { Always, OnSimulationReset, NotOnSimulationReset };

USTRUCT()
struct FSimulationStageMetaData {
    GENERATED_BODY()

    UPROPERTY()
    FName IterationSource;
    UPROPERTY()
    uint32 bSpawnOnly : 1 = 0;
    UPROPERTY()
    uint32 bWritesParticles : 1 = 0;
    UPROPERTY()
    TArray<FName> OutputDestinations;
    UPROPERTY()
    int32 MinStage = 0;
    UPROPERTY()
    int32 MaxStage = 0;
    UPROPERTY()
    FName SimulationStageName;
    UPROPERTY()
    uint32 bPartialParticleUpdate : 1 = 0;
    UPROPERTY()
    FName EnabledBinding;
    UPROPERTY()
    ENiagaraSimStageExecuteBehavior ExecuteBehavior = ENiagaraSimStageExecuteBehavior::Always;
    UPROPERTY()
    uint32 bParticleIterationStateEnabled : 1 = 0;
    UPROPERTY()
    FName ParticleIterationStateBinding;
    UPROPERTY()
    FIntPoint ParticleIterationStateRange;
    UPROPERTY()
    int32 NumIterations = 1;
    UPROPERTY()
    FName NumIterationsBinding;
    UPROPERTY()
    ENiagaraGpuDispatchType GpuDispatchType = ENiagaraGpuDispatchType::OneD;
    UPROPERTY()
    FIntVector GpuDispatchNumThreads;
    UPROPERTY()
    FName ElementCountXBinding;
    UPROPERTY()
    FName ElementCountYBinding;
    UPROPERTY()
    FName ElementCountZBinding;
    UPROPERTY()
    uint32 bOverrideElementCount : 1 = 0;
    UPROPERTY()
    TArray<FName> InputDataInterfaces;
    UPROPERTY()
    ENiagaraIterationSource IterationSourceType = ENiagaraIterationSource::Particles;
    UPROPERTY()
    FName IterationDataInterface;
    UPROPERTY()
    FName IterationDirectBinding;
    UPROPERTY()
    uint32 bGpuIndirectDispatch : 1 = 0;
    UPROPERTY()
    ENiagaraDirectDispatchElementType GpuDirectDispatchElementType = ENiagaraDirectDispatchElementType::NumThreads;
    UPROPERTY()
    FIntVector ElementCount;
};

}

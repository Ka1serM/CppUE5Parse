#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

#include "Engine/Source/Runtime/Core/Math/Matrix.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInstanceDynamic.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleEvent.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleLocation.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleMaterial.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleOrbit.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleSpawn.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystem.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

#include <cstring>
#include <vector>

namespace ue5parse::runtime {

namespace {

int32 Align16(int32 Value) { return (Value + 15) & ~15; }

}

FParticleEmitterInstance::~FParticleEmitterInstance() {
    RuntimeFree(ParticleData);
    RuntimeFree(ParticleIndices);
    RuntimeFree(InstanceData);
}

void FParticleEmitterInstance::InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent) {
    SpriteTemplate = InTemplate;
    Component = InComponent;
    SetupEmitterDuration();
    InitCustomColor();
}

void FParticleEmitterInstance::Init() {
    UParticleLODLevel* HighLODLevel = SpriteTemplate->LODLevels[0];
    CurrentMaterial = HighLODLevel->RequiredModule->Material;
    const bool bNeedsInit = ParticleSize == 0;
    if (bNeedsInit) {
        bRequiresLoopNotification = SpriteTemplate->bRequiresLoopNotification;
        bAxisLockEnabled = SpriteTemplate->bAxisLockEnabled;
        LockAxisFlags = SpriteTemplate->LockAxisFlags;
        DynamicParameterDataOffset = SpriteTemplate->DynamicParameterDataOffset;
        LightDataOffset = SpriteTemplate->LightDataOffset;
        LightVolumetricScatteringIntensity = SpriteTemplate->LightVolumetricScatteringIntensity;
        CameraPayloadOffset = SpriteTemplate->CameraPayloadOffset;
        ParticleSize = SpriteTemplate->ParticleSize;
        PivotOffset = SpriteTemplate->PivotOffset;
        TypeDataOffset = SpriteTemplate->TypeDataOffset;
        TypeDataInstanceOffset = SpriteTemplate->TypeDataInstanceOffset;
        if (InstanceData == nullptr || SpriteTemplate->ReqInstanceBytes > InstancePayloadSize) {
            InstanceData = (uint8*)RuntimeRealloc(InstanceData, InstancePayloadSize, SpriteTemplate->ReqInstanceBytes);
            InstancePayloadSize = SpriteTemplate->ReqInstanceBytes;
        }
        if (InstancePayloadSize > 0)
            std::memset(InstanceData, 0, InstancePayloadSize);
        for (UParticleModule* ParticleModule : SpriteTemplate->ModulesNeedingInstanceData)
            ParticleModule->PrepPerInstanceBlock(this, GetModuleInstanceData(ParticleModule));
        for (UParticleModule* ParticleModule : SpriteTemplate->ModulesNeedingRandomSeedInstanceData) {
            FParticleRandomSeedInfo* RandomSeedInfo = ParticleModule->GetRandomSeedInfo();
            ParticleModule->PrepRandomSeedInstancePayload(this, GetModuleRandomSeedInstanceData(ParticleModule),
                RandomSeedInfo ? *RandomSeedInfo : FParticleRandomSeedInfo());
        }
        PayloadOffset = ParticleSize;
        ParticleSize += RequiredBytes();
        ParticleSize = Align16(ParticleSize);
        ParticleStride = CalculateParticleStride(ParticleSize);
    }
    SetMeshMaterials(SpriteTemplate->MeshMaterials);
    SpawnFraction = 0;
    SecondsSinceCreation = 0;
    EmitterTime = 0;
    ParticleCounter = 0;
    UpdateTransforms();
    Location = Component->GetComponentLocation();
    OldLocation = Location;
    if (ParticleData == nullptr) {
        MaxActiveParticles = 0;
        ActiveParticles = 0;
    }
    ParticleBoundingBox = FBox();
    UParticleModuleRequired* RequiredModule = HighLODLevel->RequiredModule;
    RequiredModule->RandomImageTime = RequiredModule->RandomImageChanges == 0 ? 1.0f : 0.99f / (RequiredModule->RandomImageChanges + 1);
    if (bNeedsInit && Component->GetWorld()->IsGameWorld() && SpriteTemplate->QualityLevelSpawnRateScale > 0) {
        if (SpriteTemplate->InitialAllocationCount > 0)
            Resize(FMath::Min(SpriteTemplate->InitialAllocationCount, 100));
        else if (HighLODLevel->PeakActiveParticles > 0)
            Resize(FMath::Min(HighLODLevel->PeakActiveParticles, 100));
        else
            Resize(10);
    }
    LoopCount = 0;
    if (bNeedsInit) {
        bKillOnDeactivate = RequiredModule->bKillOnDeactivate;
        bKillOnCompleted = RequiredModule->bKillOnCompleted;
        SortMode = RequiredModule->SortMode;
        if (BurstFired.Num() < SpriteTemplate->LODLevels.Num())
            BurstFired.SetNum(SpriteTemplate->LODLevels.Num());
        for (int32 LODIndex = 0; LODIndex < SpriteTemplate->LODLevels.Num(); LODIndex++) {
            FLODBurstFired& LocalBurstFired = BurstFired[LODIndex];
            const int32 BurstCount = SpriteTemplate->LODLevels[LODIndex]->SpawnModule->BurstList.Num();
            if (LocalBurstFired.Fired.Num() < BurstCount)
                LocalBurstFired.Fired.SetNum(BurstCount);
        }
    }
    ResetBurstList();
    bEmitterIsDone = false;
}

UWorld* FParticleEmitterInstance::GetWorld() const { return Component->GetWorld(); }

void FParticleEmitterInstance::UpdateTransforms() {
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    const FMatrix ComponentToWorld = Component != nullptr ? Component->GetComponentTransform().ToMatrixNoScale() : FMatrix::Identity;
    const FMatrix EmitterToComponent =
        FRotationTranslationMatrix(LODLevel->RequiredModule->EmitterRotation, LODLevel->RequiredModule->EmitterOrigin);
    if (LODLevel->RequiredModule->bUseLocalSpace) {
        EmitterToSimulation = EmitterToComponent;
        SimulationToWorld = ComponentToWorld;
    } else {
        EmitterToSimulation = EmitterToComponent * ComponentToWorld;
        SimulationToWorld = FMatrix::Identity;
    }
}

bool FParticleEmitterInstance::Resize(int32 NewMaxActiveParticles, bool bSetMaxActiveCount) {
    if (NewMaxActiveParticles > MaxActiveParticles) {
        ParticleData = (uint8*)RuntimeRealloc(ParticleData, std::size_t(ParticleStride) * MaxActiveParticles,
            std::size_t(ParticleStride) * NewMaxActiveParticles);
        if (ParticleIndices == nullptr)
            MaxActiveParticles = 0;
        const std::size_t OldIndexBytes = ParticleIndices == nullptr ? 0 : sizeof(uint16) * (MaxActiveParticles + 1);
        ParticleIndices = (uint16*)RuntimeRealloc(ParticleIndices, OldIndexBytes, sizeof(uint16) * (NewMaxActiveParticles + 1));
        for (int32 i = MaxActiveParticles; i < NewMaxActiveParticles; i++)
            ParticleIndices[i] = uint16(i);
        MaxActiveParticles = NewMaxActiveParticles;
    }
    if (bSetMaxActiveCount) {
        UParticleLODLevel* LODLevel = SpriteTemplate->GetLODLevel(0);
        if (MaxActiveParticles > LODLevel->PeakActiveParticles)
            LODLevel->PeakActiveParticles = MaxActiveParticles;
    }
    return true;
}

void FParticleEmitterInstance::Tick(float DeltaTime, bool bSuppressSpawning) {
    const bool bFirstTime = SecondsSinceCreation <= 0.0f;
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    const float EmitterDelay = Tick_EmitterTimeSetup(DeltaTime, LODLevel);
    if (bEnabled) {
        KillParticles();
        ResetParticleParameters(DeltaTime);
        CurrentMaterial = LODLevel->RequiredModule->Material;
        Tick_ModuleUpdate(DeltaTime, LODLevel);
        SpawnFraction = Tick_SpawnParticles(DeltaTime, LODLevel, bSuppressSpawning, bFirstTime);
        Tick_ModulePostUpdate(DeltaTime, LODLevel);
        if (ActiveParticles > 0) {
            UpdateOrbitData(DeltaTime);
            UpdateBoundingBox(DeltaTime);
        }
        Tick_ModuleFinalUpdate(DeltaTime, LODLevel);
        CheckEmitterFinished();
    } else {
        FakeBursts();
    }
    EmitterTime += EmitterDelay;
    LastDeltaTime = DeltaTime;
    PositionOffsetThisTick = FVector::ZeroVector;
}

void FParticleEmitterInstance::CheckEmitterFinished() {
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    if (ActiveParticles != 0)
        return;
    UParticleModuleSpawn* SpawnModule = LODLevel->SpawnModule;
    const FParticleBurst* LastBurst = SpawnModule->BurstList.Num() > 0 ? &SpawnModule->BurstList.Last() : nullptr;
    if (LastBurst != nullptr && LastBurst->Time >= EmitterTime)
        return;
    const UParticleModuleRequired* RequiredModule = LODLevel->RequiredModule;
    if (HasCompleted() ||
        (SpawnModule->GetMaximumSpawnRate() == 0 && RequiredModule->EmitterDuration == 0 && RequiredModule->EmitterLoops == 0))
        bEmitterIsDone = true;
}

float FParticleEmitterInstance::Tick_EmitterTimeSetup(float DeltaTime, UParticleLODLevel* InCurrentLODLevel) {
    if (Component->bJustRegistered) {
        Location = Component->GetComponentLocation();
        OldLocation = Location;
    } else {
        OldLocation = Location;
        Location = Component->GetComponentLocation();
    }
    UpdateTransforms();
    SecondsSinceCreation += DeltaTime;
    UParticleModuleRequired* RequiredModule = InCurrentLODLevel->RequiredModule;
    bool bLooped = false;
    if (!RequiredModule->bUseLegacyEmitterTime) {
        EmitterTime += DeltaTime;
        bLooped = EmitterDuration > 0.0f && EmitterTime >= EmitterDuration;
    } else {
        EmitterTime = SecondsSinceCreation;
        if (EmitterDuration > UE_KINDA_SMALL_NUMBER) {
            EmitterTime = FMath::Fmod(SecondsSinceCreation, EmitterDuration);
            bLooped = (SecondsSinceCreation - EmitterDuration * LoopCount) >= EmitterDuration;
        }
    }
    float EmitterDelay = CurrentDelay;
    if (bLooped) {
        LoopCount++;
        ResetBurstList();
        if (!RequiredModule->bUseLegacyEmitterTime)
            EmitterTime -= EmitterDuration;
        if (RequiredModule->bDurationRecalcEachLoop || (RequiredModule->bDelayFirstLoopOnly && LoopCount == 1))
            SetupEmitterDuration();
        if (bRequiresLoopNotification) {
            for (int32 ModuleIdx = -3; ModuleIdx < InCurrentLODLevel->Modules.Num(); ModuleIdx++) {
                const int32 ModuleFetchIdx = ModuleIdx == -3 ? INDEX_REQUIREDMODULE
                    : ModuleIdx == -2                       ? INDEX_SPAWNMODULE
                    : ModuleIdx == -1                       ? INDEX_TYPEDATAMODULE
                                                            : ModuleIdx;
                UParticleModule* Module = InCurrentLODLevel->GetModuleAtIndex(ModuleFetchIdx);
                if (Module != nullptr && Module->RequiresLoopingNotification())
                    Module->EmitterLoopingNotify(this);
            }
        }
    }
    if (RequiredModule->bDelayFirstLoopOnly && LoopCount > 0)
        EmitterDelay = 0;
    EmitterTime -= EmitterDelay;
    return EmitterDelay;
}

float FParticleEmitterInstance::Tick_SpawnParticles(float DeltaTime, UParticleLODLevel* InCurrentLODLevel, bool bSuppressSpawning, bool bFirstTime) {
    if (!bHaltSpawning && !bHaltSpawningExternal && !bSuppressSpawning && EmitterTime >= 0.0f) {
        const int32 EmitterLoops = InCurrentLODLevel->RequiredModule->EmitterLoops;
        if (EmitterLoops == 0 || LoopCount < EmitterLoops || SecondsSinceCreation < EmitterDuration * EmitterLoops || bFirstTime)
            SpawnFraction = Spawn(DeltaTime);
    } else if (bFakeBurstsWhenSpawningSupressed) {
        FakeBursts();
    }
    return SpawnFraction;
}

void FParticleEmitterInstance::Tick_ModuleUpdate(float DeltaTime, UParticleLODLevel* InCurrentLODLevel) {
    UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
    for (int32 ModuleIndex = 0; ModuleIndex < InCurrentLODLevel->UpdateModules.Num(); ModuleIndex++) {
        UParticleModule* CurrentModule = InCurrentLODLevel->UpdateModules[ModuleIndex];
        if (CurrentModule && CurrentModule->bEnabled && CurrentModule->bUpdateModule)
            CurrentModule->Update(this, GetModuleDataOffset(HighestLODLevel->UpdateModules[ModuleIndex]), DeltaTime);
    }
}

void FParticleEmitterInstance::Tick_ModulePostUpdate(float DeltaTime, UParticleLODLevel* InCurrentLODLevel) {
    if (InCurrentLODLevel->TypeDataModule)
        InCurrentLODLevel->TypeDataModule->Update(this, TypeDataOffset, DeltaTime);
}

void FParticleEmitterInstance::Tick_ModuleFinalUpdate(float DeltaTime, UParticleLODLevel* InCurrentLODLevel) {
    UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
    for (int32 ModuleIndex = 0; ModuleIndex < InCurrentLODLevel->UpdateModules.Num(); ModuleIndex++) {
        UParticleModule* CurrentModule = InCurrentLODLevel->UpdateModules[ModuleIndex];
        if (CurrentModule && CurrentModule->bEnabled && CurrentModule->bFinalUpdateModule)
            CurrentModule->FinalUpdate(this, GetModuleDataOffset(HighestLODLevel->UpdateModules[ModuleIndex]), DeltaTime);
    }
    UParticleModuleTypeDataBase* TypeData = InCurrentLODLevel->TypeDataModule;
    if (TypeData && TypeData->bEnabled && TypeData->bFinalUpdateModule)
        TypeData->FinalUpdate(this, GetModuleDataOffset(HighestLODLevel->TypeDataModule), DeltaTime);
}

void FParticleEmitterInstance::SetCurrentLODIndex(int32 InLODIndex, bool bInFullyProcess) {
    if (SpriteTemplate == nullptr)
        return;
    CurrentLODLevelIndex = InLODIndex < SpriteTemplate->LODLevels.Num() ? InLODIndex : 0;
    CurrentLODLevel = SpriteTemplate->LODLevels[CurrentLODLevelIndex];
    EmitterDuration = EmitterDurations[CurrentLODLevelIndex];
    if (bInFullyProcess) {
        bKillOnCompleted = CurrentLODLevel->RequiredModule->bKillOnCompleted;
        bKillOnDeactivate = CurrentLODLevel->RequiredModule->bKillOnDeactivate;
        UParticleModuleSpawn* SpawnModule = CurrentLODLevel->SpawnModule;
        if (CurrentLODLevelIndex + 1 > BurstFired.Num())
            BurstFired.SetNum(CurrentLODLevelIndex + 1);
        FLODBurstFired& LocalBurstFired = BurstFired[CurrentLODLevelIndex];
        if (LocalBurstFired.Fired.Num() < SpawnModule->BurstList.Num())
            LocalBurstFired.Fired.SetNum(SpawnModule->BurstList.Num());
        for (int32 BurstIndex = 0; BurstIndex < SpawnModule->BurstList.Num(); BurstIndex++)
            if (CurrentLODLevel->RequiredModule->EmitterDelay + SpawnModule->BurstList[BurstIndex].Time < EmitterTime)
                LocalBurstFired.Fired[BurstIndex] = true;
    }
    if (GetWorld()->IsGameWorld() && !CurrentLODLevel->bEnabled)
        KillParticlesForced();
}

void FParticleEmitterInstance::Rewind() {
    SecondsSinceCreation = 0;
    EmitterTime = 0;
    LoopCount = 0;
    ParticleCounter = 0;
    bEnabled = true;
    ResetBurstList();
}

int32 FParticleEmitterInstance::GetOrbitPayloadOffset() {
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    if (LODLevel->OrbitModules.Num() == 0)
        return -1;
    UParticleModuleOrbit* OrbitModule = SpriteTemplate->LODLevels[0]->OrbitModules[LODLevel->OrbitModules.Num() - 1];
    if (OrbitModule == nullptr)
        return -1;
    const uint32* OrbitOffsetIndex = SpriteTemplate->ModuleOffsetMap.Find(OrbitModule);
    return OrbitOffsetIndex != nullptr ? int32(*OrbitOffsetIndex) : -1;
}

FVector FParticleEmitterInstance::GetParticleLocationWithOrbitOffset(FBaseParticle* Particle) {
    const int32 OrbitOffsetValue = GetOrbitPayloadOffset();
    if (OrbitOffsetValue == -1)
        return Particle->Location;
    const FOrbitChainModuleInstancePayload& OrbitPayload = *(const FOrbitChainModuleInstancePayload*)((const uint8*)Particle + OrbitOffsetValue);
    return Particle->Location + FVector(OrbitPayload.Offset);
}

void FParticleEmitterInstance::UpdateBoundingBox(float DeltaTime) {
    if (Component == nullptr)
        return;
    const bool bUpdateBox = !Component->bWarmingUp && Component->Template != nullptr && !Component->Template->bUseFixedRelativeBoundingBox;
    const FVector Scale = Component->GetComponentTransform().GetScale3D();
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    if (bUpdateBox)
        ParticleBoundingBox = FBox();
    UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
    FVector ParticlePivotOffset(-0.5, -0.5, 0.0);
    if (bUpdateBox) {
        for (UParticleModule* Module : HighestLODLevel->Modules) {
            UParticleModulePivotOffset* PivotModule = Cast<UParticleModulePivotOffset>(Module);
            if (PivotModule == nullptr)
                continue;
            ParticlePivotOffset += FVector(PivotModule->PivotOffset.X, PivotModule->PivotOffset.Y, 0.0);
            break;
        }
    }
    const int32 OrbitOffsetValue = GetOrbitPayloadOffset();
    FVector MinVal(HALF_WORLD_MAX);
    FVector MaxVal(-HALF_WORLD_MAX);
    const bool bUseLocalSpace = LODLevel->RequiredModule->bUseLocalSpace;
    const FMatrix ComponentToWorld = bUseLocalSpace ? Component->GetComponentTransform().ToMatrixWithScale() : FMatrix::Identity;
    const bool bSkipDoubleSpawnUpdate = !SpriteTemplate->bUseLegacySpawningBehavior;
    for (int32 i = 0; i < ActiveParticles; i++) {
        DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
        Particle.OldLocation = Particle.Location;
        const bool bJustSpawned = (Particle.Flags & STATE_Particle_JustSpawned) != 0;
        Particle.Flags &= ~STATE_Particle_JustSpawned;
        const bool bSkipUpdate = bJustSpawned && bSkipDoubleSpawnUpdate;
        FVector NewLocation = Particle.Location;
        float NewRotation = Particle.Rotation;
        if ((Particle.Flags & STATE_Particle_Freeze) == 0 && !bSkipUpdate) {
            if ((Particle.Flags & STATE_Particle_FreezeTranslation) == 0)
                NewLocation = Particle.Location + FVector(DeltaTime * Particle.Velocity);
            if ((Particle.Flags & STATE_Particle_FreezeRotation) == 0)
                NewRotation = DeltaTime * Particle.RotationRate + Particle.Rotation;
        }
        double LocalMax = 0.0;
        if (bUpdateBox) {
            if (OrbitOffsetValue == -1) {
                LocalMax = (FVector(Particle.Size) * Scale).GetAbsMax();
            } else {
                const FOrbitChainModuleInstancePayload& OrbitPayload =
                    *(const FOrbitChainModuleInstancePayload*)((const uint8*)&Particle + OrbitOffsetValue);
                LocalMax = OrbitPayload.Offset.GetAbsMax();
            }
            LocalMax += (FVector(Particle.Size) * ParticlePivotOffset).GetAbsMax();
        }
        NewLocation += PositionOffsetThisTick;
        Particle.OldLocation += PositionOffsetThisTick;
        Particle.Location = NewLocation;
        Particle.Rotation = FMath::Fmod(NewRotation, 2.f * float(UE_PI));
        if (bUpdateBox) {
            const FVector PositionForBounds = bUseLocalSpace ? ComponentToWorld.TransformPosition(NewLocation) : NewLocation;
            MinVal.X = FMath::Min(MinVal.X, PositionForBounds.X - LocalMax);
            MaxVal.X = FMath::Max(MaxVal.X, PositionForBounds.X + LocalMax);
            MinVal.Y = FMath::Min(MinVal.Y, PositionForBounds.Y - LocalMax);
            MaxVal.Y = FMath::Max(MaxVal.Y, PositionForBounds.Y + LocalMax);
            MinVal.Z = FMath::Min(MinVal.Z, PositionForBounds.Z - LocalMax);
            MaxVal.Z = FMath::Max(MaxVal.Z, PositionForBounds.Z + LocalMax);
        }
    }
    if (bUpdateBox)
        ParticleBoundingBox = FBox(MinVal, MaxVal);
}

uint32 FParticleEmitterInstance::RequiredBytes() {
    bool bHasSubUV = false;
    for (int32 LODIndex = 0; LODIndex < SpriteTemplate->LODLevels.Num() && !bHasSubUV; LODIndex++) {
        UParticleLODLevel* LODLevel = SpriteTemplate->GetLODLevel(LODIndex);
        if (LODLevel && LODLevel->RequiredModule->InterpolationMethod != PSUVIM_None)
            bHasSubUV = true;
    }
    if (!bHasSubUV)
        return 0;
    SubUVDataOffset = PayloadOffset;
    return sizeof(FFullSubUVPayload);
}

uint32 FParticleEmitterInstance::GetModuleDataOffset(UParticleModule* Module) {
    const uint32* Offset = SpriteTemplate->ModuleOffsetMap.Find(Module);
    return Offset != nullptr ? *Offset : 0;
}

uint8* FParticleEmitterInstance::GetModuleInstanceData(UParticleModule* Module) {
    if (InstanceData == nullptr)
        return nullptr;
    const uint32* Offset = SpriteTemplate->ModuleInstanceOffsetMap.Find(Module);
    return Offset != nullptr ? &InstanceData[*Offset] : nullptr;
}

FParticleRandomSeedInstancePayload* FParticleEmitterInstance::GetModuleRandomSeedInstanceData(UParticleModule* Module) {
    if (InstanceData == nullptr)
        return nullptr;
    const uint32* Offset = SpriteTemplate->ModuleRandomSeedInstanceOffsetMap.Find(Module);
    return Offset != nullptr ? (FParticleRandomSeedInstancePayload*)&InstanceData[*Offset] : nullptr;
}

void FParticleEmitterInstance::ResetBurstList() {
    for (FLODBurstFired& CurrBurstFired : BurstFired)
        for (bool& Fired : CurrBurstFired.Fired)
            Fired = false;
}

float FParticleEmitterInstance::GetCurrentBurstRateOffset(float& DeltaTime, int32& Burst) {
    float SpawnRateInc = 0.0f;
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    UParticleModuleSpawn* SpawnModule = LODLevel->SpawnModule;
    if (SpawnModule->BurstList.Num() == 0)
        return SpawnRateInc;
    FRandomStream& RandomStream = SpawnModule->GetRandomStream(this);
    for (int32 BurstIdx = 0; BurstIdx < SpawnModule->BurstList.Num(); BurstIdx++) {
        const FParticleBurst& BurstEntry = SpawnModule->BurstList[BurstIdx];
        if (LODLevel->Level >= BurstFired.Num())
            continue;
        FLODBurstFired& LocalBurstFired = BurstFired[LODLevel->Level];
        if (BurstIdx >= LocalBurstFired.Fired.Num() || LocalBurstFired.Fired[BurstIdx] || EmitterTime < BurstEntry.Time)
            continue;
        if (DeltaTime < 0.00001f)
            DeltaTime = 0.00001f;
        int32 Count = BurstEntry.Count;
        if (BurstEntry.CountLow > -1)
            Count = RandomStream.RandRange(BurstEntry.CountLow, BurstEntry.Count);
        const float Scale = SpawnModule->BurstScale.GetValue(EmitterTime, Component);
        Count = FMath::CeilToInt(Count * Scale);
        SpawnRateInc += Count / DeltaTime;
        Burst += Count;
        LocalBurstFired.Fired[BurstIdx] = true;
    }
    return SpawnRateInc;
}

void FParticleEmitterInstance::ResetParticleParameters(float DeltaTime) {
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
    TArray<int32> OrbitOffsets;
    for (int32 OrbitIndex = 0; OrbitIndex < LODLevel->OrbitModules.Num(); OrbitIndex++) {
        UParticleModuleOrbit* OrbitModule = HighestLODLevel->OrbitModules[OrbitIndex];
        if (OrbitModule == nullptr)
            continue;
        const uint32* OrbitOffset = SpriteTemplate->ModuleOffsetMap.Find(OrbitModule);
        if (OrbitOffset != nullptr)
            OrbitOffsets.Add(int32(*OrbitOffset));
    }
    const bool bSkipDoubleSpawnUpdate = !SpriteTemplate->bUseLegacySpawningBehavior;
    for (int32 ParticleIndex = 0; ParticleIndex < ActiveParticles; ParticleIndex++) {
        DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
        Particle.Velocity = Particle.BaseVelocity;
        Particle.Size = GetParticleBaseSize(Particle);
        Particle.RotationRate = Particle.BaseRotationRate;
        Particle.Color = Particle.BaseColor;
        const bool bJustSpawned = (Particle.Flags & STATE_Particle_JustSpawned) != 0;
        const bool bSkipUpdate = bJustSpawned && bSkipDoubleSpawnUpdate;
        Particle.RelativeTime += bSkipUpdate ? 0.0f : Particle.OneOverMaxLifetime * DeltaTime;
        if (CameraPayloadOffset > 0) {
            FCameraOffsetParticlePayload& CameraOffsetPayload = *(FCameraOffsetParticlePayload*)((uint8*)&Particle + CameraPayloadOffset);
            CameraOffsetPayload.Offset = CameraOffsetPayload.BaseOffset;
        }
        for (int32 OrbitOffset : OrbitOffsets) {
            FOrbitChainModuleInstancePayload& OrbitPayload = *(FOrbitChainModuleInstancePayload*)((uint8*)&Particle + OrbitOffset);
            OrbitPayload.PreviousOffset = OrbitPayload.Offset;
            OrbitPayload.Offset = OrbitPayload.BaseOffset;
            OrbitPayload.RotationRate = OrbitPayload.BaseRotationRate;
        }
    }
}

void FParticleEmitterInstance::CalculateOrbitOffset(FOrbitChainModuleInstancePayload& Payload, FVector& AccumOffset, FVector& AccumRotation,
    FVector& AccumRotationRate, float DeltaTime, FVector& Result, FMatrix& RotationMat) {
    AccumRotation += AccumRotationRate * DeltaTime;
    Payload.Rotation = FVector3f(AccumRotation);
    if (!AccumRotation.IsNearlyZero()) {
        const FVector RotRot = RotationMat.TransformVector(AccumRotation);
        const FVector ScaledRotation = RotRot * 360.0f;
        RotationMat *= FRotationMatrix(FRotator::MakeFromEuler(ScaledRotation));
        Result = RotationMat.TransformPosition(AccumOffset);
    } else {
        Result = AccumOffset;
    }
    AccumOffset = FVector::ZeroVector;
    AccumRotation = FVector::ZeroVector;
    AccumRotationRate = FVector::ZeroVector;
}

void FParticleEmitterInstance::UpdateOrbitData(float DeltaTime) {
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    const int32 ModuleCount = LODLevel->OrbitModules.Num();
    if (ModuleCount == 0)
        return;
    UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
    std::vector<FVector> Offsets(ModuleCount + 1, FVector::ZeroVector);
    std::vector<int32> ModuleOffsets(ModuleCount + 1, 0);
    for (int32 ModOffIndex = 0; ModOffIndex < ModuleCount; ModOffIndex++)
        ModuleOffsets[ModOffIndex] = int32(GetModuleDataOffset(HighestLODLevel->OrbitModules[ModOffIndex]));
    for (int32 i = ActiveParticles - 1; i >= 0; i--) {
        uint8* ParticleBase = ParticleData + ParticleIndices[i] * ParticleStride;
        const FBaseParticle& Particle = *(const FBaseParticle*)ParticleBase;
        if ((Particle.Flags & STATE_Particle_Freeze) != 0)
            continue;
        int32 OffsetIndex = 0;
        FVector AccumulatedOffset = FVector::ZeroVector;
        FVector AccumulatedRotation = FVector::ZeroVector;
        FVector AccumulatedRotationRate = FVector::ZeroVector;
        FOrbitChainModuleInstancePayload* LocalOrbitPayload = nullptr;
        FOrbitChainModuleInstancePayload* PrevOrbitPayload = nullptr;
        uint8 PrevOrbitChainMode = 0;
        FMatrix AccumRotMatrix = FMatrix::Identity;
        for (int32 OrbitIndex = 0; OrbitIndex < ModuleCount; OrbitIndex++) {
            const int32 CurrentOffset = ModuleOffsets[OrbitIndex];
            UParticleModuleOrbit* OrbitModule = LODLevel->OrbitModules[OrbitIndex];
            if (CurrentOffset == 0)
                continue;
            FOrbitChainModuleInstancePayload& OrbitPayload = *(FOrbitChainModuleInstancePayload*)(ParticleBase + CurrentOffset);
            bool bCalculateOffset = false;
            if (OrbitIndex == ModuleCount - 1) {
                LocalOrbitPayload = &OrbitPayload;
                bCalculateOffset = true;
            }
            if (OrbitModule->ChainMode == EOChainMode_Add) {
                if (OrbitModule->bEnabled) {
                    AccumulatedOffset += FVector(OrbitPayload.Offset);
                    AccumulatedRotation += FVector(OrbitPayload.Rotation);
                    AccumulatedRotationRate += FVector(OrbitPayload.RotationRate);
                }
            } else if (OrbitModule->ChainMode == EOChainMode_Scale) {
                if (OrbitModule->bEnabled) {
                    AccumulatedOffset *= FVector(OrbitPayload.Offset);
                    AccumulatedRotation *= FVector(OrbitPayload.Rotation);
                    AccumulatedRotationRate *= FVector(OrbitPayload.RotationRate);
                }
            } else if (OrbitModule->ChainMode == EOChainMode_Link) {
                if (OrbitIndex > 0 && PrevOrbitChainMode == EOChainMode_Link) {
                    FVector ResultOffset;
                    CalculateOrbitOffset(*PrevOrbitPayload, AccumulatedOffset, AccumulatedRotation, AccumulatedRotationRate, DeltaTime,
                        ResultOffset, AccumRotMatrix);
                    if (!OrbitModule->bEnabled) {
                        AccumulatedOffset = FVector::ZeroVector;
                        AccumulatedRotation = FVector::ZeroVector;
                        AccumulatedRotationRate = FVector::ZeroVector;
                    }
                    Offsets[OffsetIndex++] = ResultOffset;
                }
                if (OrbitModule->bEnabled) {
                    AccumulatedOffset = FVector(OrbitPayload.Offset);
                    AccumulatedRotation = FVector(OrbitPayload.Rotation);
                    AccumulatedRotationRate = FVector(OrbitPayload.RotationRate);
                }
            }
            if (bCalculateOffset) {
                FVector ResultOffset;
                CalculateOrbitOffset(OrbitPayload, AccumulatedOffset, AccumulatedRotation, AccumulatedRotationRate, DeltaTime, ResultOffset,
                    AccumRotMatrix);
                Offsets[OffsetIndex++] = ResultOffset;
            }
            if (OrbitModule->bEnabled) {
                PrevOrbitPayload = &OrbitPayload;
                PrevOrbitChainMode = OrbitModule->ChainMode;
            }
        }
        if (LocalOrbitPayload != nullptr) {
            LocalOrbitPayload->Offset = FVector3f::ZeroVector;
            for (int32 AccumIndex = 0; AccumIndex < OffsetIndex; AccumIndex++)
                LocalOrbitPayload->Offset += FVector3f(Offsets[AccumIndex]);
            std::fill(Offsets.begin(), Offsets.end(), FVector::ZeroVector);
        }
    }
}

float FParticleEmitterInstance::Spawn(float DeltaTime) {
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    float SpawnRate = 0.0f;
    int32 BurstCount = 0;
    const float OldLeftover = SpawnFraction;
    UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
    bool bProcessSpawnRate = true;
    bool bProcessBurstList = true;
    if (SpriteTemplate->QualityLevelSpawnRateScale > 0.0f) {
        for (int32 SpawnModIndex = 0; SpawnModIndex < LODLevel->SpawningModules.Num(); SpawnModIndex++) {
            UParticleModuleSpawnBase* SpawnModule = LODLevel->SpawningModules[SpawnModIndex];
            if (!SpawnModule || !SpawnModule->bEnabled)
                continue;
            const uint32 Offset = GetModuleDataOffset(HighestLODLevel->SpawningModules[SpawnModIndex]);
            int32 Number = 0;
            float Rate = 0.0f;
            if (!SpawnModule->GetSpawnAmount(this, Offset, OldLeftover, DeltaTime, Number, Rate))
                bProcessSpawnRate = false;
            Number = FMath::Max<int32>(0, Number);
            Rate = FMath::Max<float>(0.0f, Rate);
            SpawnRate += Rate;
            int32 BurstNumber = 0;
            if (!SpawnModule->GetBurstCount(this, Offset, OldLeftover, DeltaTime, BurstNumber))
                bProcessBurstList = false;
            BurstCount += BurstNumber;
        }
        if (bProcessSpawnRate) {
            const float RateScale = LODLevel->SpawnModule->RateScale.GetValue(EmitterTime, Component) * LODLevel->SpawnModule->GetGlobalRateScale();
            SpawnRate += LODLevel->SpawnModule->Rate.GetValue(EmitterTime, Component) * RateScale;
            SpawnRate = FMath::Max<float>(0.0f, SpawnRate);
        }
        if (bProcessBurstList) {
            int32 Burst = 0;
            GetCurrentBurstRateOffset(DeltaTime, Burst);
            BurstCount += Burst;
        }
        const float QualityMult = SpriteTemplate->GetQualityLevelSpawnRateMult();
        SpawnRate = FMath::Max<float>(0.0f, SpawnRate * QualityMult);
        BurstCount = FMath::CeilToInt(BurstCount * QualityMult);
    } else {
        SpawnRate = 0.0f;
        BurstCount = 0;
    }
    if (SpawnRate <= 0.f && BurstCount <= 0)
        return SpawnFraction;
    const float SafetyLeftover = OldLeftover;
    float NewLeftover = OldLeftover + DeltaTime * SpawnRate;
    int32 Number = FMath::FloorToInt(NewLeftover);
    const float Increment = SpawnRate > 0.0f ? 1.f / SpawnRate : 0.0f;
    const float StartTime = DeltaTime + OldLeftover * Increment - Increment;
    NewLeftover = NewLeftover - Number;
    int32 NewCount = ActiveParticles + Number + BurstCount;
    if (NewCount > MaxCPUParticlesPerEmitter) {
        int32 MaxNewParticles = MaxCPUParticlesPerEmitter - ActiveParticles;
        BurstCount = FMath::Min(MaxNewParticles, BurstCount);
        MaxNewParticles -= BurstCount;
        Number = FMath::Min(MaxNewParticles, Number);
        NewCount = ActiveParticles + Number + BurstCount;
    }
    const float BurstIncrement = SpriteTemplate->bUseLegacySpawningBehavior && BurstCount > 0 ? 1.f / BurstCount : 0.0f;
    const float BurstStartTime = SpriteTemplate->bUseLegacySpawningBehavior ? DeltaTime * BurstIncrement : 0.0f;
    bool bProcessSpawn = true;
    if (NewCount >= MaxActiveParticles) {
        const int32 Grown = NewCount + FMath::TruncToInt(FMath::Sqrt(FMath::Sqrt(float(NewCount))) + 1);
        bProcessSpawn = Resize(Grown, DeltaTime < PeakActiveParticleUpdateDelta);
    }
    if (!bProcessSpawn)
        return SafetyLeftover;
    FParticleEventInstancePayload* EventPayload = nullptr;
    if (LODLevel->EventGenerator) {
        EventPayload = (FParticleEventInstancePayload*)GetModuleInstanceData(LODLevel->EventGenerator);
        if (EventPayload && !EventPayload->bSpawnEventsPresent && !EventPayload->bBurstEventsPresent)
            EventPayload = nullptr;
    }
    const FVector InitialLocation = EmitterToSimulation.GetOrigin();
    SpawnParticles(Number, StartTime, Increment, InitialLocation, FVector::ZeroVector, EventPayload);
    SpawnParticles(BurstCount, BurstStartTime, BurstIncrement, InitialLocation, FVector::ZeroVector, EventPayload);
    return NewLeftover;
}

void FParticleEmitterInstance::FixupParticleIndices() {
    std::vector<bool> UsedIndices(MaxActiveParticles, false);
    for (int32 i = 0; i < ActiveParticles; ++i) {
        const uint16 UsedIndex = ParticleIndices[i];
        if (UsedIndex < MaxActiveParticles && !UsedIndices[UsedIndex]) {
            UsedIndices[UsedIndex] = true;
            continue;
        }
        if (i != ActiveParticles - 1)
            ParticleIndices[i] = ParticleIndices[ActiveParticles - 1];
        --ActiveParticles;
        --i;
    }
    int32 NextFree = 0;
    for (int32 i = ActiveParticles; i < MaxActiveParticles; ++i) {
        while (NextFree < MaxActiveParticles && UsedIndices[NextFree])
            ++NextFree;
        UsedIndices[NextFree] = true;
        ParticleIndices[i] = uint16(NextFree);
    }
}

void FParticleEmitterInstance::SpawnParticles(int32 Count, float StartTime, float Increment, const FVector& InitialLocation,
    const FVector& InitialVelocity, FParticleEventInstancePayload* EventPayload) {
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    Count = FMath::Min<int32>(Count, MaxActiveParticles - ActiveParticles);
    if (EventPayload && EventPayload->bBurstEventsPresent && Count > 0)
        LODLevel->EventGenerator->HandleParticleBurst(this, EventPayload, Count);
    const bool bLegacySpawnBehavior = SpriteTemplate->bUseLegacySpawningBehavior;
    UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
    float SpawnTime = StartTime;
    float Interp = 1.0f;
    const float InterpIncrement = Count > 0 && Increment > 0.0f ? 1.0f / float(Count) : 0.0f;
    for (int32 i = 0; i < Count; i++) {
        uint16 NextFreeIndex = ParticleIndices[ActiveParticles];
        if (NextFreeIndex >= MaxActiveParticles) {
            FixupParticleIndices();
            NextFreeIndex = ParticleIndices[ActiveParticles];
        }
        DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * NextFreeIndex);
        const uint32 CurrentParticleIndex = ActiveParticles++;
        if (bLegacySpawnBehavior) {
            StartTime -= Increment;
            Interp -= InterpIncrement;
        }
        PreSpawn(Particle, InitialLocation, InitialVelocity);
        for (int32 ModuleIndex = 0; ModuleIndex < LODLevel->SpawnModules.Num(); ModuleIndex++) {
            UParticleModule* SpawnModule = LODLevel->SpawnModules[ModuleIndex];
            if (SpawnModule->bEnabled)
                SpawnModule->Spawn(this, GetModuleDataOffset(HighestLODLevel->SpawnModules[ModuleIndex]), SpawnTime, Particle);
        }
        PostSpawn(Particle, Interp, SpawnTime);
        if (Particle->RelativeTime > 1.0f) {
            KillParticle(CurrentParticleIndex);
            continue;
        }
        if (EventPayload && EventPayload->bSpawnEventsPresent)
            LODLevel->EventGenerator->HandleParticleSpawned(this, EventPayload, Particle);
        if (!bLegacySpawnBehavior) {
            SpawnTime -= Increment;
            Interp -= InterpIncrement;
        }
    }
}

UParticleLODLevel* FParticleEmitterInstance::GetCurrentLODLevelChecked() { return SpriteTemplate->GetCurrentLODLevel(this); }

void FParticleEmitterInstance::ForceSpawn(float DeltaTime, int32 InSpawnCount, int32 InBurstCount, FVector& InLocation, FVector& InVelocity) {
    const int32 SpawnCount = InSpawnCount;
    const int32 BurstCount = InBurstCount;
    if (SpawnCount <= 0 && BurstCount <= 0)
        return;
    const int32 Number = SpawnCount;
    const float Increment = SpawnCount > 0 ? DeltaTime / SpawnCount : 0;
    const float StartTime = DeltaTime;
    bool bProcessSpawn = true;
    const int32 NewCount = ActiveParticles + Number + BurstCount;
    if (NewCount >= MaxActiveParticles) {
        const int32 Grown = NewCount + FMath::TruncToInt(FMath::Sqrt(FMath::Sqrt(float(NewCount))) + 1);
        bProcessSpawn = Resize(Grown, DeltaTime < PeakActiveParticleUpdateDelta);
    }
    if (!bProcessSpawn)
        return;
    SpawnParticles(Number, StartTime, Increment, InLocation, InVelocity, nullptr);
    SpawnParticles(BurstCount, StartTime, 0.0f, InLocation, InVelocity, nullptr);
    //! Not in UE: an emitter that an event gives particles is no longer done, else HasCompleted deactivates it this frame.
    if (ActiveParticles > 0)
        bEmitterIsDone = false;
}

void FParticleEmitterInstance::PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation, const FVector& InitialVelocity) {
    std::memset((void*)Particle, 0, ParticleSize);
    Particle->Location = InitialLocation;
    Particle->BaseVelocity = FVector3f(InitialVelocity);
    Particle->Velocity = FVector3f(InitialVelocity);
    Particle->Location -= PositionOffsetThisTick;
}

bool FParticleEmitterInstance::HasCompleted() {
    if (SpriteTemplate == nullptr)
        return true;
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    const int32 EmitterLoops = LODLevel->RequiredModule->EmitterLoops;
    if (EmitterLoops == 0 || SecondsSinceCreation < EmitterDuration * EmitterLoops)
        return false;
    return ActiveParticles <= 0;
}

void FParticleEmitterInstance::PostSpawn(FBaseParticle* Particle, float InterpolationPercentage, float SpawnTime) {
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    if (!LODLevel->RequiredModule->bUseLocalSpace && FVector::DistSquared(OldLocation, Location) > 1.f)
        Particle->Location += (OldLocation - Location) * InterpolationPercentage;
    Particle->OldLocation = Particle->Location;
    Particle->Location += FVector(Particle->Velocity) * SpawnTime;
    Particle->Flags |= (ParticleCounter++) & STATE_CounterMask;
    Particle->Flags |= STATE_Particle_JustSpawned;
}

namespace {

FParticleEventInstancePayload* GetDeathEventPayload(FParticleEmitterInstance* Instance, UParticleLODLevel* LODLevel) {
    if (!LODLevel->EventGenerator)
        return nullptr;
    FParticleEventInstancePayload* EventPayload = (FParticleEventInstancePayload*)Instance->GetModuleInstanceData(LODLevel->EventGenerator);
    return EventPayload && EventPayload->bDeathEventsPresent ? EventPayload : nullptr;
}

}

void FParticleEmitterInstance::KillParticles() {
    if (ActiveParticles <= 0)
        return;
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    FParticleEventInstancePayload* EventPayload = GetDeathEventPayload(this, LODLevel);
    const bool bValorant = GetRuntimeProvider().getGame() == EGame::GAME_Valorant;
    bool bFoundCorruptIndices = false;
    for (int32 i = ActiveParticles - 1; i >= 0; i--) {
        const int32 CurrentIndex = ParticleIndices[i];
        if (CurrentIndex >= MaxActiveParticles) {
            bFoundCorruptIndices = true;
            continue;
        }
        FBaseParticle& Particle = *(FBaseParticle*)(ParticleData + CurrentIndex * ParticleStride);
        if (bValorant && PreciseLifetimeOffset != 0 ? *(const float*)((const uint8*)&Particle + PreciseLifetimeOffset) > SecondsSinceCreation : Particle.RelativeTime <= 1.0f)
            continue;
        if (EventPayload)
            LODLevel->EventGenerator->HandleParticleKilled(this, EventPayload, &Particle);
        ParticleIndices[i] = ParticleIndices[ActiveParticles - 1];
        ParticleIndices[ActiveParticles - 1] = uint16(CurrentIndex);
        ActiveParticles--;
    }
    if (bFoundCorruptIndices)
        FixupParticleIndices();
}

void FParticleEmitterInstance::KillParticle(int32 Index) {
    if (Index >= ActiveParticles)
        return;
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    FParticleEventInstancePayload* EventPayload = GetDeathEventPayload(this, LODLevel);
    const uint16 KillIndex = ParticleIndices[Index];
    if (EventPayload)
        LODLevel->EventGenerator->HandleParticleKilled(this, EventPayload, (FBaseParticle*)(ParticleData + KillIndex * ParticleStride));
    for (int32 i = Index; i < ActiveParticles - 1; i++)
        ParticleIndices[i] = ParticleIndices[i + 1];
    ParticleIndices[ActiveParticles - 1] = KillIndex;
    ActiveParticles--;
}

void FParticleEmitterInstance::FakeBursts() {
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    UParticleModuleSpawn* SpawnModule = LODLevel->SpawnModule;
    if (LODLevel->Level >= BurstFired.Num())
        return;
    FLODBurstFired& LocalBurstFired = BurstFired[LODLevel->Level];
    for (int32 BurstIdx = 0; BurstIdx < SpawnModule->BurstList.Num(); BurstIdx++)
        if (BurstIdx < LocalBurstFired.Fired.Num() && EmitterTime >= SpawnModule->BurstList[BurstIdx].Time)
            LocalBurstFired.Fired[BurstIdx] = true;
}

void FParticleEmitterInstance::KillParticlesForced(bool bFireEvents) {
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    FParticleEventInstancePayload* EventPayload = bFireEvents ? GetDeathEventPayload(this, LODLevel) : nullptr;
    for (int32 KillIdx = ActiveParticles - 1; KillIdx >= 0; KillIdx--) {
        const uint16 CurrentIndex = ParticleIndices[KillIdx];
        if (EventPayload)
            LODLevel->EventGenerator->HandleParticleKilled(this, EventPayload, (FBaseParticle*)(ParticleData + CurrentIndex * ParticleStride));
        ParticleIndices[KillIdx] = ParticleIndices[ActiveParticles - 1];
        ParticleIndices[ActiveParticles - 1] = CurrentIndex;
        ActiveParticles--;
    }
    ParticleCounter = 0;
}

FBaseParticle* FParticleEmitterInstance::GetParticle(int32 Index) {
    if (Index >= ActiveParticles || Index < 0)
        return nullptr;
    return (FBaseParticle*)(ParticleData + ParticleStride * ParticleIndices[Index]);
}

void FParticleEmitterInstance::SetupEmitterDuration() {
    if (SpriteTemplate == nullptr)
        return;
    const int32 LODCount = SpriteTemplate->LODLevels.Num();
    if (EmitterDurations.Num() != LODCount)
        EmitterDurations.SetNum(LODCount);
    for (int32 LODIndex = 0; LODIndex < LODCount; LODIndex++) {
        UParticleLODLevel* TempLOD = SpriteTemplate->LODLevels[LODIndex];
        UParticleModuleRequired* RequiredModule = TempLOD->RequiredModule;
        FRandomStream& RandomStream = RequiredModule->GetRandomStream(this);
        CurrentDelay = RequiredModule->EmitterDelay + Component->EmitterDelay;
        if (RequiredModule->bEmitterDelayUseRange) {
            const float Rand = RandomStream.FRand();
            CurrentDelay =
                RequiredModule->EmitterDelayLow + (RequiredModule->EmitterDelay - RequiredModule->EmitterDelayLow) * Rand + Component->EmitterDelay;
        }
        float& Duration = EmitterDurations[TempLOD->Level];
        if (RequiredModule->bEmitterDurationUseRange) {
            const float Rand = RandomStream.FRand();
            Duration = RequiredModule->EmitterDurationLow + (RequiredModule->EmitterDuration - RequiredModule->EmitterDurationLow) * Rand + CurrentDelay;
        } else {
            Duration = RequiredModule->EmitterDuration + CurrentDelay;
        }
        if (LoopCount == 1 && RequiredModule->bDelayFirstLoopOnly && (RequiredModule->EmitterLoops == 0 || RequiredModule->EmitterLoops > 1))
            Duration -= CurrentDelay;
    }
    EmitterDuration = EmitterDurations[CurrentLODLevelIndex];
}

bool FParticleEmitterInstance::IsDynamicDataRequired(UParticleLODLevel* InCurrentLODLevel) {
    if (ActiveParticles <= 0 || (SpriteTemplate && SpriteTemplate->EmitterRenderMode == ERM_None))
        return false;
    if (InCurrentLODLevel == nullptr || !InCurrentLODLevel->bEnabled ||
        (InCurrentLODLevel->RequiredModule->bUseMaxDrawCount && InCurrentLODLevel->RequiredModule->MaxDrawCount == 0))
        return false;
    return Component != nullptr;
}

void FParticleEmitterInstance::ProcessParticleEvents(float DeltaTime, bool bSuppressSpawning) {
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    for (UParticleModuleEventReceiverBase* EventRcvr : LODLevel->EventReceiverModules) {
        if (EventRcvr->WillProcessParticleEvent(EPET_Spawn))
            for (FParticleEventSpawnData& Event : Component->SpawnEvents)
                EventRcvr->ProcessParticleEvent(this, Event, DeltaTime);
        if (EventRcvr->WillProcessParticleEvent(EPET_Death))
            for (FParticleEventDeathData& Event : Component->DeathEvents)
                EventRcvr->ProcessParticleEvent(this, Event, DeltaTime);
        if (EventRcvr->WillProcessParticleEvent(EPET_Collision))
            for (FParticleEventCollideData& Event : Component->CollisionEvents)
                EventRcvr->ProcessParticleEvent(this, Event, DeltaTime);
        if (EventRcvr->WillProcessParticleEvent(EPET_Burst))
            for (FParticleEventBurstData& Event : Component->BurstEvents)
                EventRcvr->ProcessParticleEvent(this, Event, DeltaTime);
        if (EventRcvr->WillProcessParticleEvent(EPET_Blueprint))
            for (FParticleEventKismetData& Event : Component->KismetEvents)
                EventRcvr->ProcessParticleEvent(this, Event, DeltaTime);
    }
}

namespace {

int32 EmitterMaterialIndex(const UParticleSystemComponent& Component, int32 EmitterIndex) {
    if (GetRuntimeProvider().getGame() == EGame::GAME_Valorant)
        return EmitterIndex + Component.Template->NamedMaterialSlots.Num();
    return EmitterIndex;
}

}

void FParticleEmitterInstance::Tick_MaterialOverrides(int32 EmitterIndex) {
    UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
    bool bOverridden = false;
    if (LODLevel && LODLevel->RequiredModule && Component && Component->Template) {
        const TArray<FName>& NamedOverrides = LODLevel->RequiredModule->NamedMaterialOverrides;
        const TArray<FNamedEmitterMaterial>& Slots = Component->Template->NamedMaterialSlots;
        const TArray<TObjectPtr<UMaterialInterface>>& EmitterMaterials = Component->EmitterMaterials;
        if (NamedOverrides.Num() > 0) {
            for (int32 CheckIdx = 0; CheckIdx < Slots.Num(); ++CheckIdx) {
                if (NamedOverrides[0] != Slots[CheckIdx].Name)
                    continue;
                CurrentMaterial = Slots[CheckIdx].Material;
                if (EmitterMaterials.IsValidIndex(CheckIdx) && EmitterMaterials[CheckIdx] != nullptr) {
                    CurrentMaterial = EmitterMaterials[CheckIdx];
                    UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(CurrentMaterial);
                    if (bHasCustomColor && Component->bCustomColorOnNamedMaterials && MID)
                        ApplyCustomColor(MID);
                }
                bOverridden = true;
                break;
            }
        }
    }
    if (bOverridden || !Component || !Component->Template)
        return;
    const int32 MaterialIndex = EmitterMaterialIndex(*Component, EmitterIndex);
    if (Component->EmitterMaterials.IsValidIndex(MaterialIndex) && Component->EmitterMaterials[MaterialIndex])
        CurrentMaterial = Component->EmitterMaterials[MaterialIndex];
}

void FParticleEmitterInstance::InitCustomColor() {
    bHasCustomColor = false;
    if (SpriteTemplate == nullptr || Component == nullptr || GetRuntimeProvider().getGame() != EGame::GAME_Valorant)
        return;
    UParticleLODLevel* HighLODLevel = SpriteTemplate->LODLevels[0];
    UParticleModule* CustomColor = nullptr;
    for (UParticleModule* Module : HighLODLevel->Modules) {
        if (Module && Module->IsCustomColor() && Module->bEnabled) {
            CustomColor = Module;
            break;
        }
    }
    if (CustomColor == nullptr)
        return;
    bHasCustomColor = true;
    UParticleModuleRequired* RequiredModule = HighLODLevel->RequiredModule;
    TArray<TObjectPtr<UMaterialInterface>>& EmitterMaterials = Component->EmitterMaterials;
    if (RequiredModule == nullptr || RequiredModule->NamedMaterialOverrides.Num() <= 0) {
        const int32 EmitterIndex = Component->EmitterInstances.Find(this);
        if (EmitterIndex == INDEX_NONE)
            return;
        const int32 MaterialIndex = EmitterIndex + Component->Template->NamedMaterialSlots.Num();
        UMaterialInterface* Material = EmitterMaterials.IsValidIndex(MaterialIndex) && EmitterMaterials[MaterialIndex] ? EmitterMaterials[MaterialIndex].Get()
                                       : RequiredModule                                                               ? RequiredModule->Material.Get()
                                                                                                                      : nullptr;
        if (Material == nullptr)
            return;
        UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material);
        if (MID == nullptr) {
            MID = UMaterialInstanceDynamic::Create(Material, Component);
            Component->SetMaterial(MaterialIndex, MID);
        }
        if (MID)
            CustomColor->ApplyCustomColor(Component, MID);
        return;
    }
    const TArray<FNamedEmitterMaterial>& Slots = Component->Template->NamedMaterialSlots;
    for (const FName& Override : RequiredModule->NamedMaterialOverrides) {
        for (int32 SlotIdx = 0; SlotIdx < Slots.Num(); ++SlotIdx) {
            if (Slots[SlotIdx].Name != Override)
                continue;
            const bool bHasEntry = EmitterMaterials.IsValidIndex(SlotIdx);
            UMaterialInterface* Material = bHasEntry && EmitterMaterials[SlotIdx] ? EmitterMaterials[SlotIdx].Get() : Slots[SlotIdx].Material.Get();
            if (Material == nullptr)
                continue;
            UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material);
            if (MID == nullptr) {
                MID = UMaterialInstanceDynamic::Create(Material, Component);
                if (!bHasEntry)
                    EmitterMaterials.SetNum(SlotIdx + 1);
                EmitterMaterials[SlotIdx] = MID;
            }
            if (MID) {
                CustomColor->ApplyCustomColor(Component, MID);
                Component->bCustomColorOnNamedMaterials = true;
            }
        }
    }
}

void FParticleEmitterInstance::ApplyCustomColor(UMaterialInstanceDynamic* MID) {
    UParticleLODLevel* HighLODLevel = SpriteTemplate->LODLevels[0];
    if (HighLODLevel->Modules.Num() <= 0)
        return;
    UParticleModule* CustomColor = HighLODLevel->Modules[0];
    if (CustomColor && CustomColor->IsCustomColor() && CustomColor->bEnabled)
        CustomColor->ApplyCustomColor(Component, MID);
}

void FParticleMeshEmitterInstance::InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent) {
    FParticleEmitterInstance::InitParameters(InTemplate, InComponent);
    MeshTypeData = CastChecked<UParticleModuleTypeDataMesh>(InTemplate->GetLODLevel(0)->TypeDataModule.Get());
    MeshRotationActive = InTemplate->bMeshRotationActive;
}

bool FParticleMeshEmitterInstance::Resize(int32 NewMaxActiveParticles, bool bSetMaxActiveCount) {
    const int32 OldMaxActiveParticles = MaxActiveParticles;
    if (!FParticleEmitterInstance::Resize(NewMaxActiveParticles, bSetMaxActiveCount))
        return false;
    if (MeshRotationActive) {
        for (int32 i = OldMaxActiveParticles; i < NewMaxActiveParticles; i++) {
            FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)(ParticleData + ParticleStride * ParticleIndices[i] + MeshRotationOffset);
            PayloadData->RotationRateBase = FVector3f::ZeroVector;
        }
    }
    return true;
}

namespace {

void StorePreviousMotionBlurState(const FBaseParticle& Particle, int32 MeshRotationOffset, int32 MeshMotionBlurOffset, int32 CameraPayloadOffset,
    int32 OrbitModuleOffset) {
    const FMeshRotationPayloadData* RotationPayloadData = (const FMeshRotationPayloadData*)((const uint8*)&Particle + MeshRotationOffset);
    FMeshMotionBlurPayloadData* MotionBlurPayloadData = (FMeshMotionBlurPayloadData*)((uint8*)&Particle + MeshMotionBlurOffset);
    MotionBlurPayloadData->BaseParticlePrevRotation = Particle.Rotation;
    MotionBlurPayloadData->BaseParticlePrevVelocity = Particle.Velocity;
    MotionBlurPayloadData->BaseParticlePrevSize = Particle.Size;
    MotionBlurPayloadData->PayloadPrevRotation = RotationPayloadData->Rotation;
    MotionBlurPayloadData->PayloadPrevCameraOffset =
        CameraPayloadOffset ? ((const FCameraOffsetParticlePayload*)((const uint8*)&Particle + CameraPayloadOffset))->Offset : 0.0f;
    MotionBlurPayloadData->PayloadPrevOrbitOffset = OrbitModuleOffset
        ? ((const FOrbitChainModuleInstancePayload*)((const uint8*)&Particle + OrbitModuleOffset))->Offset
        : FVector3f::ZeroVector;
}

}

void FParticleMeshEmitterInstance::Tick(float DeltaTime, bool bSuppressSpawning) {
    if (bEnabled && MeshMotionBlurOffset) {
        for (int32 i = 0; i < ActiveParticles; i++) {
            DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
            StorePreviousMotionBlurState(Particle, MeshRotationOffset, MeshMotionBlurOffset, CameraPayloadOffset, OrbitModuleOffset);
        }
    }
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    if (MeshRotationActive && bEnabled) {
        const UParticleModuleRequired* RequiredModule = LODLevel->RequiredModule;
        for (int32 i = 0; i < ActiveParticles; i++) {
            DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
            FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
            PayloadData->RotationRate = PayloadData->RotationRateBase;
            if (RequiredModule->ScreenAlignment == PSA_Velocity || RequiredModule->ScreenAlignment == PSA_AwayFromCenter) {
                FVector NewDirection = FVector(Particle.Velocity);
                if (RequiredModule->ScreenAlignment == PSA_Velocity) {
                    if (RequiredModule->bOrbitModuleAffectsVelocityAlignment && LODLevel->OrbitModules.Num() > 0) {
                        UParticleModuleOrbit* LastOrbit = SpriteTemplate->LODLevels[0]->OrbitModules[LODLevel->OrbitModules.Num() - 1];
                        const uint32 SpriteOrbitModuleOffset = *SpriteTemplate->ModuleOffsetMap.Find(LastOrbit);
                        if (SpriteOrbitModuleOffset != 0) {
                            const FOrbitChainModuleInstancePayload& OrbitPayload =
                                *(const FOrbitChainModuleInstancePayload*)((const uint8*)&Particle + SpriteOrbitModuleOffset);
                            const FVector NewPos = Particle.Location + FVector(OrbitPayload.Offset);
                            const FVector OldPos = Particle.OldLocation + FVector(OrbitPayload.PreviousOffset);
                            NewDirection = NewPos - OldPos;
                        }
                    }
                } else {
                    NewDirection = Particle.Location;
                }
                NewDirection.Normalize();
                const FQuat Rotation = FQuat::FindBetweenNormals(FVector(1.0, 0.0, 0.0), NewDirection);
                const FVector3f Euler(Rotation.Euler());
                PayloadData->Rotation = PayloadData->InitRotation + Euler;
                PayloadData->Rotation += PayloadData->CurContinuousRotation;
            } else if ((Particle.Flags & STATE_Particle_FreezeRotation) == 0) {
                PayloadData->Rotation = PayloadData->InitRotation + PayloadData->CurContinuousRotation;
            }
        }
    }
    FParticleEmitterInstance::Tick(DeltaTime, bSuppressSpawning);
    if (MeshRotationActive && bEnabled) {
        for (int32 i = 0; i < ActiveParticles; i++) {
            FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)(ParticleData + ParticleStride * ParticleIndices[i] + MeshRotationOffset);
            PayloadData->CurContinuousRotation += DeltaTime * PayloadData->RotationRate;
        }
    }
}

void FParticleMeshEmitterInstance::Tick_MaterialOverrides(int32 EmitterIndex) {
    if (!Component || !Component->Template)
        return;
    TArray<TObjectPtr<UMaterialInterface>>& EmitterMaterials = Component->EmitterMaterials;
    const TArray<FNamedEmitterMaterial>& Slots = Component->Template->NamedMaterialSlots;
    const int32 MaterialIndex = EmitterMaterialIndex(*Component, EmitterIndex);
    UMaterialInterface* EmitterMaterial = EmitterMaterials.IsValidIndex(MaterialIndex) ? EmitterMaterials[MaterialIndex].Get() : nullptr;
    if (EmitterMaterial)
        for (UMaterialInterface*& CurMat : CurrentMaterials)
            CurMat = EmitterMaterial;
    bool bOverridden = false;
    UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
    if (LODLevel && LODLevel->RequiredModule && LODLevel->RequiredModule->NamedMaterialOverrides.Num() > 0) {
        const TArray<FName>& NamedOverrides = LODLevel->RequiredModule->NamedMaterialOverrides;
        CurrentMaterials.SetNum(NamedOverrides.Num());
        for (int32 MaterialIdx = 0; MaterialIdx < NamedOverrides.Num(); ++MaterialIdx) {
            for (int32 CheckIdx = 0; CheckIdx < Slots.Num(); ++CheckIdx) {
                if (NamedOverrides[MaterialIdx] != Slots[CheckIdx].Name)
                    continue;
                CurrentMaterials[MaterialIdx] = Slots[CheckIdx].Material;
                if (EmitterMaterials.IsValidIndex(CheckIdx) && EmitterMaterials[CheckIdx] != nullptr) {
                    CurrentMaterials[MaterialIdx] = EmitterMaterials[CheckIdx];
                    UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(CurrentMaterials[MaterialIdx]);
                    if (bHasCustomColor && Component->bCustomColorOnNamedMaterials && MID)
                        ApplyCustomColor(MID);
                }
                bOverridden = true;
                break;
            }
        }
    }
    if (bHasCustomColor && !bOverridden && EmitterMaterial) {
        CurrentMaterials.SetNum(1);
        CurrentMaterials[0] = EmitterMaterial;
    }
}

void FParticleMeshEmitterInstance::UpdateBoundingBox(float DeltaTime) {
    if (Component == nullptr || ActiveParticles <= 0)
        return;
    const bool bUpdateBox = !Component->bWarmingUp && Component->Template != nullptr && !Component->Template->bUseFixedRelativeBoundingBox;
    const FVector Scale = Component->GetComponentTransform().GetScale3D();
    FBoxSphereBounds MeshBound(FVector(0, 0, 0), FVector(0, 0, 0), 0);
    if (!Component->bWarmingUp && MeshTypeData->Mesh)
        MeshBound = MeshTypeData->Mesh->GetBounds();
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    const bool bUseLocalSpace = LODLevel->RequiredModule->bUseLocalSpace;
    const FMatrix ComponentToWorld = bUseLocalSpace ? Component->GetComponentTransform().ToMatrixWithScale() : FMatrix::Identity;
    if (bUpdateBox)
        ParticleBoundingBox = FBox();
    FVector MinVal(HALF_WORLD_MAX);
    FVector MaxVal(-HALF_WORLD_MAX);
    const bool bSkipDoubleSpawnUpdate = !SpriteTemplate->bUseLegacySpawningBehavior;
    const FVector MeshExtent = MeshBound.GetBox().GetExtent();
    for (int32 i = 0; i < ActiveParticles; i++) {
        DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
        Particle.OldLocation = Particle.Location;
        const bool bJustSpawned = (Particle.Flags & STATE_Particle_JustSpawned) != 0;
        Particle.Flags &= ~STATE_Particle_JustSpawned;
        const bool bSkipUpdate = bJustSpawned && bSkipDoubleSpawnUpdate;
        FVector NewLocation = Particle.Location;
        float NewRotation = Particle.Rotation;
        if ((Particle.Flags & STATE_Particle_Freeze) == 0 && !bSkipUpdate) {
            if ((Particle.Flags & STATE_Particle_FreezeTranslation) == 0)
                NewLocation = Particle.Location + FVector(Particle.Velocity) * DeltaTime;
            if ((Particle.Flags & STATE_Particle_FreezeRotation) == 0)
                NewRotation = Particle.Rotation + DeltaTime * Particle.RotationRate;
        }
        const FVector LocalExtent = MeshExtent * FVector(Particle.Size) * Scale;
        NewLocation += PositionOffsetThisTick;
        Particle.OldLocation += PositionOffsetThisTick;
        Particle.Rotation = FMath::Fmod(NewRotation, 2.f * float(UE_PI));
        Particle.Location = NewLocation;
        if (bUpdateBox) {
            const FVector PositionForBounds = bUseLocalSpace ? ComponentToWorld.TransformPosition(NewLocation) : NewLocation;
            MinVal.X = FMath::Min(MinVal.X, PositionForBounds.X - LocalExtent.X);
            MaxVal.X = FMath::Max(MaxVal.X, PositionForBounds.X + LocalExtent.X);
            MinVal.Y = FMath::Min(MinVal.Y, PositionForBounds.Y - LocalExtent.Y);
            MaxVal.Y = FMath::Max(MaxVal.Y, PositionForBounds.Y + LocalExtent.Y);
            MinVal.Z = FMath::Min(MinVal.Z, PositionForBounds.Z - LocalExtent.Z);
            MaxVal.Z = FMath::Max(MaxVal.Z, PositionForBounds.Z + LocalExtent.Z);
        }
    }
    if (bUpdateBox)
        ParticleBoundingBox = FBox(MinVal, MaxVal);
}

uint32 FParticleMeshEmitterInstance::RequiredBytes() {
    uint32 uiBytes = FParticleEmitterInstance::RequiredBytes();
    MeshRotationOffset = PayloadOffset + uiBytes;
    uiBytes += sizeof(FMeshRotationPayloadData);
    if (MeshTypeData && MeshTypeData->IsMotionBlurEnabled()) {
        MeshMotionBlurOffset = PayloadOffset + uiBytes;
        uiBytes += sizeof(FMeshMotionBlurPayloadData);
    }
    return uiBytes;
}

void FParticleMeshEmitterInstance::PostSpawn(FBaseParticle* Particle, float InterpolationPercentage, float SpawnTime) {
    FParticleEmitterInstance::PostSpawn(Particle, InterpolationPercentage, SpawnTime);
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)Particle + MeshRotationOffset);
    const EParticleScreenAlignment ScreenAlignment = LODLevel->RequiredModule->ScreenAlignment;
    if (ScreenAlignment == PSA_Velocity || ScreenAlignment == PSA_AwayFromCenter) {
        FVector NewDirection = ScreenAlignment == PSA_AwayFromCenter ? Particle->Location : FVector(Particle->Velocity);
        NewDirection.Normalize();
        const FVector Euler = FQuat::FindBetweenNormals(FVector(1.0, 0.0, 0.0), NewDirection).Euler();
        PayloadData->Rotation.X += Euler.X;
        PayloadData->Rotation.Y += Euler.Y;
        PayloadData->Rotation.Z += Euler.Z;
    }
    PayloadData->InitialOrientation = FVector3f(MeshTypeData->RollPitchYawRange.GetValue(SpawnTime, nullptr, 0, &MeshTypeData->RandomStream));
    if (MeshMotionBlurOffset)
        StorePreviousMotionBlurState(*Particle, MeshRotationOffset, MeshMotionBlurOffset, CameraPayloadOffset, OrbitModuleOffset);
}

bool FParticleMeshEmitterInstance::IsDynamicDataRequired(UParticleLODLevel* InCurrentLODLevel) {
    return MeshTypeData->Mesh != nullptr && MeshTypeData->Mesh->GetRenderData() != nullptr &&
        FParticleEmitterInstance::IsDynamicDataRequired(InCurrentLODLevel);
}

void FParticleMeshEmitterInstance::GetMeshMaterials(TArray<UMaterialInterface*>& OutMaterials, const UParticleLODLevel* LODLevel) const {
    if (!MeshTypeData || !MeshTypeData->Mesh || MeshTypeData->Mesh->GetRenderData() == nullptr)
        return;
    const FStaticMeshLODResources& LODModel = MeshTypeData->Mesh->GetRenderData()->LODResources[0];
    for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++) {
        UMaterialInterface* Material = SectionIndex < CurrentMaterials.Num() ? CurrentMaterials[SectionIndex] : nullptr;
        if (Material == nullptr) {
            for (UParticleModule* Module : LODLevel->Modules) {
                UParticleModuleMeshMaterial* MeshMatModule = Cast<UParticleModuleMeshMaterial>(Module);
                if (MeshMatModule && MeshMatModule->bEnabled && SectionIndex < MeshMatModule->MeshMaterials.Num()) {
                    Material = MeshMatModule->MeshMaterials[SectionIndex];
                    break;
                }
            }
        }
        if (Material == nullptr && MeshTypeData->bOverrideMaterial)
            Material = CurrentMaterial ? CurrentMaterial : LODLevel->RequiredModule->Material.Get();
        if (Material == nullptr)
            Material = MeshTypeData->Mesh->GetMaterial(LODModel.Sections[SectionIndex].MaterialIndex);
        OutMaterials.Add(Material);
    }
}

}

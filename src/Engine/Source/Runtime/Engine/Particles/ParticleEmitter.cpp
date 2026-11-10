#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitter.h"

#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleCamera.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleLight.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleLocation.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleMaterial.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleOrientation.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleParameter.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleSubUV.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystem.h"

namespace ue5parse::runtime {

void UParticleEmitter::PostLoad() {
    Super::PostLoad();
    for (UParticleLODLevel* LODLevel : LODLevels)
        if (LODLevel)
            LODLevel->ConditionalPostLoad();
    ConvertedModules = true;
    for (UParticleLODLevel* LODLevel : LODLevels) {
        if (!LODLevel)
            continue;
        for (int32 ModuleIndex = LODLevel->Modules.Num() - 1; ModuleIndex >= 0; ModuleIndex--)
            if (LODLevel->Modules[ModuleIndex] == nullptr)
                LODLevel->Modules.RemoveAt(ModuleIndex);
    }
    QualityLevelSpawnRateScale = FMath::Clamp<float>(QualityLevelSpawnRateScale, 0.0f, 1.0f);
    UpdateModuleLists();
}

void UParticleEmitter::UpdateModuleLists() {
    for (UParticleLODLevel* LODLevel : LODLevels)
        if (LODLevel)
            LODLevel->UpdateModuleLists();
    Build();
}

UParticleLODLevel* UParticleEmitter::GetCurrentLODLevel(FParticleEmitterInstance* Instance) {
    return Instance->CurrentLODLevel;
}

void UParticleEmitter::Build() {
    if (LODLevels.Num() == 0)
        return;
    UParticleLODLevel* HighLODLevel = LODLevels[0];
    if (HighLODLevel->TypeDataModule != nullptr)
        HighLODLevel->TypeDataModule->CacheModuleInfo(this);
    CacheEmitterModuleInfo();
}

void UParticleEmitter::CacheEmitterModuleInfo() {
    bRequiresLoopNotification = false;
    bAxisLockEnabled = false;
    bMeshRotationActive = false;
    LockAxisFlags = EPAL_NONE;
    ModuleOffsetMap.Empty();
    ModuleInstanceOffsetMap.Empty();
    ModuleRandomSeedInstanceOffsetMap.Empty();
    ModulesNeedingInstanceData.Empty();
    ModulesNeedingRandomSeedInstanceData.Empty();
    MeshMaterials.Empty();
    DynamicParameterDataOffset = 0;
    LightDataOffset = 0;
    LightVolumetricScatteringIntensity = 0;
    CameraPayloadOffset = 0;
    ParticleSize = sizeof(FBaseParticle);
    ReqInstanceBytes = 0;
    SubUVAnimation = nullptr;
    PivotOffset = FVector2D(-0.5f, -0.5f);
    TypeDataOffset = 0;
    TypeDataInstanceOffset = -1;

    UParticleLODLevel* HighLODLevel = GetLODLevel(0);
    UParticleModuleTypeDataBase* HighTypeData = HighLODLevel->TypeDataModule;
    if (HighTypeData) {
        const int32 ReqBytes = int32(HighTypeData->RequiredBytes(nullptr));
        if (ReqBytes) {
            TypeDataOffset = ParticleSize;
            ParticleSize += ReqBytes;
        }
        const int32 TempInstanceBytes = int32(HighTypeData->RequiredBytesPerInstance());
        if (TempInstanceBytes) {
            TypeDataInstanceOffset = ReqInstanceBytes;
            ReqInstanceBytes += TempInstanceBytes;
        }
    }

    UParticleModuleRequired* RequiredModule = HighLODLevel->RequiredModule;
    bMeshRotationActive = RequiredModule->ScreenAlignment == PSA_Velocity || RequiredModule->ScreenAlignment == PSA_AwayFromCenter;

    for (int32 ModuleIdx = 0; ModuleIdx < HighLODLevel->Modules.Num(); ModuleIdx++) {
        UParticleModule* ParticleModule = HighLODLevel->Modules[ModuleIdx];
        bRequiresLoopNotification |= ParticleModule->bEnabled && ParticleModule->RequiresLoopingNotification();

        if (!ParticleModule->IsA<UParticleModuleTypeDataBase>()) {
            const int32 ReqBytes = int32(ParticleModule->RequiredBytes(HighTypeData));
            if (ReqBytes) {
                ModuleOffsetMap.Add(ParticleModule, ParticleSize);
                if (ParticleModule->IsA<UParticleModuleParameterDynamic>() && DynamicParameterDataOffset == 0)
                    DynamicParameterDataOffset = ParticleSize;
                if (UParticleModuleLight* ParticleModuleLight = Cast<UParticleModuleLight>(ParticleModule); ParticleModuleLight && LightDataOffset == 0) {
                    LightVolumetricScatteringIntensity = ParticleModuleLight->VolumetricScatteringIntensity;
                    LightDataOffset = ParticleSize;
                }
                if (ParticleModule->IsA<UParticleModuleCameraOffset>() && CameraPayloadOffset == 0)
                    CameraPayloadOffset = ParticleSize;
                ParticleSize += ReqBytes;
            }

            const int32 TempInstanceBytes = int32(ParticleModule->RequiredBytesPerInstance());
            if (TempInstanceBytes > 0) {
                ModuleInstanceOffsetMap.Add(ParticleModule, ReqInstanceBytes);
                ModulesNeedingInstanceData.Add(ParticleModule);
                for (int32 LODIdx = 1; LODIdx < LODLevels.Num(); LODIdx++)
                    ModuleInstanceOffsetMap.Add(LODLevels[LODIdx]->Modules[ModuleIdx], ReqInstanceBytes);
                ReqInstanceBytes += TempInstanceBytes;
            }

            if (ParticleModule->bSupportsRandomSeed) {
                ModuleRandomSeedInstanceOffsetMap.Add(ParticleModule, ReqInstanceBytes);
                ModulesNeedingRandomSeedInstanceData.Add(ParticleModule);
                for (int32 LODIdx = 1; LODIdx < LODLevels.Num(); LODIdx++)
                    ModuleRandomSeedInstanceOffsetMap.Add(LODLevels[LODIdx]->Modules[ModuleIdx], ReqInstanceBytes);
                ReqInstanceBytes += sizeof(FParticleRandomSeedInstancePayload);
            }
        }

        if (UParticleModuleOrientationAxisLock* Module_AxisLock = Cast<UParticleModuleOrientationAxisLock>(ParticleModule)) {
            bAxisLockEnabled = Module_AxisLock->bEnabled;
            LockAxisFlags = Module_AxisLock->LockAxisFlags;
        } else if (UParticleModulePivotOffset* PivotModule = Cast<UParticleModulePivotOffset>(ParticleModule)) {
            PivotOffset += PivotModule->PivotOffset;
        } else if (UParticleModuleMeshMaterial* MeshMaterialModule = Cast<UParticleModuleMeshMaterial>(ParticleModule)) {
            if (MeshMaterialModule->bEnabled) {
                MeshMaterials.Empty();
                for (UMaterialInterface* Material : MeshMaterialModule->MeshMaterials)
                    MeshMaterials.Add(Material);
            }
        } else if (UParticleModuleSubUV* SubUVModule = Cast<UParticleModuleSubUV>(ParticleModule)) {
            USubUVAnimation* ModuleSubUVAnimation = SubUVModule->Animation;
            SubUVAnimation =
                ModuleSubUVAnimation && ModuleSubUVAnimation->SubUVTexture && ModuleSubUVAnimation->IsBoundingGeometryValid() ? ModuleSubUVAnimation : nullptr;
        }

        if (!bMeshRotationActive && ParticleModule->TouchesMeshRotation())
            bMeshRotationActive = true;
    }
}

bool UParticleEmitter::HasAnyEnabledLODs() const {
    for (const UParticleLODLevel* LodLevel : LODLevels)
        if (LodLevel && LodLevel->bEnabled)
            return true;
    return false;
}

void UParticleSpriteEmitter::PostLoad() {
    Super::PostLoad();
    for (UParticleLODLevel* LODLevel : LODLevels) {
        if (!LODLevel)
            continue;
        UParticleModuleRequired* RequiredModule = LODLevel->RequiredModule;
        if (RequiredModule && RequiredModule->Material)
            RequiredModule->Material->ConditionalPostLoad();
    }
}

FParticleEmitterInstance* UParticleSpriteEmitter::CreateInstance(UParticleSystemComponent* InComponent) {
    if (bCookedOut || LODLevels.Num() == 0)
        return nullptr;
    FParticleEmitterInstance* Instance = nullptr;
    UParticleLODLevel* LODLevel = GetLODLevel(0);
    if (LODLevel->TypeDataModule) {
        Instance = LODLevel->TypeDataModule->CreateInstance(this, InComponent);
    } else {
        Instance = new FParticleSpriteEmitterInstance();
        Instance->InitParameters(this, InComponent);
    }
    if (Instance) {
        Instance->CurrentLODLevelIndex = 0;
        Instance->CurrentLODLevel = LODLevels[Instance->CurrentLODLevelIndex];
        Instance->Init();
    }
    return Instance;
}

}

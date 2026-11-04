#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleParameter.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

enum EDynamicParameterUpdateFlags {
    EDPU_UPDATE_NONE = 0x00,
    EDPU_UPDATE_0 = 0x01,
    EDPU_UPDATE_1 = 0x02,
    EDPU_UPDATE_2 = 0x04,
    EDPU_UPDATE_3 = 0x08,
    EDPU_UPDATE_01 = EDPU_UPDATE_0 | EDPU_UPDATE_1,
    EDPU_UPDATE_012 = EDPU_UPDATE_0 | EDPU_UPDATE_1 | EDPU_UPDATE_2,
    EDPU_UPDATE_ALL = EDPU_UPDATE_0 | EDPU_UPDATE_1 | EDPU_UPDATE_2 | EDPU_UPDATE_3
};

static int32 ParticleDynamicParameter_GetIndexFromFlag(int32 InFlags) {
    switch (InFlags) {
    case EDPU_UPDATE_0:
        return 0;
    case EDPU_UPDATE_1:
        return 1;
    case EDPU_UPDATE_2:
        return 2;
    case EDPU_UPDATE_3:
        return 3;
    }
    return INDEX_NONE;
}

UParticleModuleParameterDynamic::UParticleModuleParameterDynamic(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleParameterDynamic::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleParameterDynamic::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    {
        PARTICLE_ELEMENT(FEmitterDynamicParameterPayload, DynamicPayload);
        if (DynamicParams[0].ValueMethod != EDPV_AutoSet)
            DynamicPayload.DynamicParameterValue[0] = GetParameterValue(DynamicParams[0], Particle, Owner, InRandomStream);
        if (DynamicParams[1].ValueMethod != EDPV_AutoSet)
            DynamicPayload.DynamicParameterValue[1] = GetParameterValue(DynamicParams[1], Particle, Owner, InRandomStream);
        if (DynamicParams[2].ValueMethod != EDPV_AutoSet)
            DynamicPayload.DynamicParameterValue[2] = GetParameterValue(DynamicParams[2], Particle, Owner, InRandomStream);
        if (DynamicParams[3].ValueMethod != EDPV_AutoSet)
            DynamicPayload.DynamicParameterValue[3] = GetParameterValue(DynamicParams[3], Particle, Owner, InRandomStream);
    }
}

void UParticleModuleParameterDynamic::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    if (UpdateFlags == EDPU_UPDATE_NONE)
        return;
    if ((Owner == nullptr) || (Owner->ActiveParticles <= 0) || (Owner->ParticleData == nullptr) || (Owner->ParticleIndices == nullptr))
        return;
    FRandomStream& RandomStream = GetRandomStream(Owner);
    int32 ParameterIndex = ParticleDynamicParameter_GetIndexFromFlag(UpdateFlags);
    switch (UpdateFlags) {
    case EDPU_UPDATE_0:
    case EDPU_UPDATE_1:
    case EDPU_UPDATE_2:
    case EDPU_UPDATE_3: {
        FEmitterDynamicParameter& DynParam = DynamicParams[ParameterIndex];
        if (bUsesVelocity == false) {
            BEGIN_UPDATE_LOOP;
            {
                FEmitterDynamicParameterPayload& DynamicPayload = *((FEmitterDynamicParameterPayload*)(ParticleBase + CurrentOffset));
                DynamicPayload.DynamicParameterValue[ParameterIndex] = GetParameterValue_UserSet(DynParam, Particle, Owner, &RandomStream);
            }
            END_UPDATE_LOOP;
        } else {
            BEGIN_UPDATE_LOOP;
            {
                FEmitterDynamicParameterPayload& DynamicPayload = *((FEmitterDynamicParameterPayload*)(ParticleBase + CurrentOffset));
                DynamicPayload.DynamicParameterValue[ParameterIndex] = GetParameterValue(DynParam, Particle, Owner, &RandomStream);
            }
            END_UPDATE_LOOP;
        }
    } break;
    case EDPU_UPDATE_01: {
        FEmitterDynamicParameter& DynParam0 = DynamicParams[0];
        FEmitterDynamicParameter& DynParam1 = DynamicParams[1];
        if (bUsesVelocity == false) {
            BEGIN_UPDATE_LOOP;
            {
                FEmitterDynamicParameterPayload& DynamicPayload = *((FEmitterDynamicParameterPayload*)(ParticleBase + CurrentOffset));
                DynamicPayload.DynamicParameterValue[0] = GetParameterValue_UserSet(DynParam0, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[1] = GetParameterValue_UserSet(DynParam1, Particle, Owner, &RandomStream);
            }
            END_UPDATE_LOOP;
        } else {
            BEGIN_UPDATE_LOOP;
            {
                FEmitterDynamicParameterPayload& DynamicPayload = *((FEmitterDynamicParameterPayload*)(ParticleBase + CurrentOffset));
                DynamicPayload.DynamicParameterValue[0] = GetParameterValue(DynParam0, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[1] = GetParameterValue(DynParam1, Particle, Owner, &RandomStream);
            }
            END_UPDATE_LOOP;
        }
    } break;
    case EDPU_UPDATE_012: {
        FEmitterDynamicParameter& DynParam0 = DynamicParams[0];
        FEmitterDynamicParameter& DynParam1 = DynamicParams[1];
        FEmitterDynamicParameter& DynParam2 = DynamicParams[2];
        if (bUsesVelocity == false) {
            BEGIN_UPDATE_LOOP;
            {
                FEmitterDynamicParameterPayload& DynamicPayload = *((FEmitterDynamicParameterPayload*)(ParticleBase + CurrentOffset));
                DynamicPayload.DynamicParameterValue[0] = GetParameterValue_UserSet(DynParam0, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[1] = GetParameterValue_UserSet(DynParam1, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[2] = GetParameterValue_UserSet(DynParam2, Particle, Owner, &RandomStream);
            }
            END_UPDATE_LOOP;
        } else {
            BEGIN_UPDATE_LOOP;
            {
                FEmitterDynamicParameterPayload& DynamicPayload = *((FEmitterDynamicParameterPayload*)(ParticleBase + CurrentOffset));
                DynamicPayload.DynamicParameterValue[0] = GetParameterValue(DynParam0, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[1] = GetParameterValue(DynParam1, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[2] = GetParameterValue(DynParam2, Particle, Owner, &RandomStream);
            }
            END_UPDATE_LOOP;
        }
    } break;
    case EDPU_UPDATE_ALL: {
        FEmitterDynamicParameter& DynParam0 = DynamicParams[0];
        FEmitterDynamicParameter& DynParam1 = DynamicParams[1];
        FEmitterDynamicParameter& DynParam2 = DynamicParams[2];
        FEmitterDynamicParameter& DynParam3 = DynamicParams[3];
        if (bUsesVelocity == false) {
            BEGIN_UPDATE_LOOP;
            {
                FEmitterDynamicParameterPayload& DynamicPayload = *((FEmitterDynamicParameterPayload*)(ParticleBase + CurrentOffset));
                DynamicPayload.DynamicParameterValue[0] = GetParameterValue_UserSet(DynParam0, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[1] = GetParameterValue_UserSet(DynParam1, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[2] = GetParameterValue_UserSet(DynParam2, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[3] = GetParameterValue_UserSet(DynParam3, Particle, Owner, &RandomStream);
            }
            END_UPDATE_LOOP;
        } else {
            BEGIN_UPDATE_LOOP;
            {
                FEmitterDynamicParameterPayload& DynamicPayload = *((FEmitterDynamicParameterPayload*)(ParticleBase + CurrentOffset));
                DynamicPayload.DynamicParameterValue[0] = GetParameterValue(DynParam0, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[1] = GetParameterValue(DynParam1, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[2] = GetParameterValue(DynParam2, Particle, Owner, &RandomStream);
                DynamicPayload.DynamicParameterValue[3] = GetParameterValue(DynParam3, Particle, Owner, &RandomStream);
            }
            END_UPDATE_LOOP;
        }
    } break;
    default: {
        FEmitterDynamicParameter& DynParam0 = DynamicParams[0];
        FEmitterDynamicParameter& DynParam1 = DynamicParams[1];
        FEmitterDynamicParameter& DynParam2 = DynamicParams[2];
        FEmitterDynamicParameter& DynParam3 = DynamicParams[3];
        if (bUsesVelocity == false) {
            BEGIN_UPDATE_LOOP;
            {
                FEmitterDynamicParameterPayload& DynamicPayload = *((FEmitterDynamicParameterPayload*)(ParticleBase + CurrentOffset));
                DynamicPayload.DynamicParameterValue[0] = (UpdateFlags & EDPU_UPDATE_0)
                                                              ? GetParameterValue_UserSet(DynParam0, Particle, Owner, &RandomStream)
                                                              : DynamicPayload.DynamicParameterValue[0];
                DynamicPayload.DynamicParameterValue[1] = (UpdateFlags & EDPU_UPDATE_1)
                                                              ? GetParameterValue_UserSet(DynParam1, Particle, Owner, &RandomStream)
                                                              : DynamicPayload.DynamicParameterValue[1];
                DynamicPayload.DynamicParameterValue[2] = (UpdateFlags & EDPU_UPDATE_2)
                                                              ? GetParameterValue_UserSet(DynParam2, Particle, Owner, &RandomStream)
                                                              : DynamicPayload.DynamicParameterValue[2];
                DynamicPayload.DynamicParameterValue[3] = (UpdateFlags & EDPU_UPDATE_3)
                                                              ? GetParameterValue_UserSet(DynParam3, Particle, Owner, &RandomStream)
                                                              : DynamicPayload.DynamicParameterValue[3];
            }
            END_UPDATE_LOOP;
        } else {
            BEGIN_UPDATE_LOOP;
            {
                FEmitterDynamicParameterPayload& DynamicPayload = *((FEmitterDynamicParameterPayload*)(ParticleBase + CurrentOffset));
                DynamicPayload.DynamicParameterValue[0] = (UpdateFlags & EDPU_UPDATE_0) ? GetParameterValue(DynParam0, Particle, Owner, &RandomStream)
                                                                                        : DynamicPayload.DynamicParameterValue[0];
                DynamicPayload.DynamicParameterValue[1] = (UpdateFlags & EDPU_UPDATE_1) ? GetParameterValue(DynParam1, Particle, Owner, &RandomStream)
                                                                                        : DynamicPayload.DynamicParameterValue[1];
                DynamicPayload.DynamicParameterValue[2] = (UpdateFlags & EDPU_UPDATE_2) ? GetParameterValue(DynParam2, Particle, Owner, &RandomStream)
                                                                                        : DynamicPayload.DynamicParameterValue[2];
                DynamicPayload.DynamicParameterValue[3] = (UpdateFlags & EDPU_UPDATE_3) ? GetParameterValue(DynParam3, Particle, Owner, &RandomStream)
                                                                                        : DynamicPayload.DynamicParameterValue[3];
            }
            END_UPDATE_LOOP;
        }
    } break;
    }
}

float UParticleModuleParameterDynamic::GetParameterValue(FEmitterDynamicParameter& InDynParams, FBaseParticle& Particle, FParticleEmitterInstance* Owner, FRandomStream* InRandomStream) {
    float ScaleValue = 1.0f;
    float DistributionValue = 1.0f;
    switch (InDynParams.ValueMethod) {
    case EDPV_VelocityX:
        ScaleValue = Particle.Velocity.X;
        break;
    case EDPV_VelocityY:
        ScaleValue = Particle.Velocity.Y;
        break;
    case EDPV_VelocityZ:
        ScaleValue = Particle.Velocity.Z;
        break;
    case EDPV_VelocityMag:
        ScaleValue = Particle.Velocity.Size();
        break;
    default:
        break;
    }
    if (InDynParams.bScaleVelocityByParamValue || InDynParams.ValueMethod == EDPV_UserSet) {
        const float TimeValue = InDynParams.bUseEmitterTime ? Owner->EmitterTime : Particle.RelativeTime;
        DistributionValue = InDynParams.ParamValue.GetValue(TimeValue, Owner->Component, InRandomStream);
    }
    return DistributionValue * ScaleValue;
}
float UParticleModuleParameterDynamic::GetParameterValue_UserSet(FEmitterDynamicParameter& InDynParams, FBaseParticle& Particle, FParticleEmitterInstance* Owner, FRandomStream* InRandomStream) {
    return InDynParams.ParamValue.GetValue(InDynParams.bUseEmitterTime ? Owner->EmitterTime : Particle.RelativeTime, Owner->Component, InRandomStream);
}

uint32 UParticleModuleParameterDynamic::RequiredBytes(UParticleModuleTypeDataBase* TypeData) {
    return sizeof(FEmitterDynamicParameterPayload);
}

UParticleModuleParameterDynamic_Seeded::UParticleModuleParameterDynamic_Seeded(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleParameterDynamic_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = (FParticleRandomSeedInstancePayload*)(Owner->GetModuleInstanceData(this));
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

}

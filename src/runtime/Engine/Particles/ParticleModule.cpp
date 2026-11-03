#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ue5parse/runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "ue5parse/runtime/Engine/Particles/ParticleSystemComponent.h"

// UE's UParticleModule (ParticleModules.cpp, UE 5.3).

namespace ue5parse::runtime {

uint32 UParticleModule::PrepRandomSeedInstancePayload(FParticleEmitterInstance* Owner, FParticleRandomSeedInstancePayload* InRandSeedPayload,
    const FParticleRandomSeedInfo& InRandSeedInfo) {
    if (Owner == nullptr || Owner->Component == nullptr || InRandSeedPayload == nullptr)
        return 0xffffffff;
    new (InRandSeedPayload) FParticleRandomSeedInstancePayload();
    if (InRandSeedInfo.bGetSeedFromInstance) {
        float SeedValue;
        if (Owner->Component->GetFloatParameter(InRandSeedInfo.ParameterName, SeedValue)) {
            if (!InRandSeedInfo.bInstanceSeedIsIndex) {
                InRandSeedPayload->RandomStream.Initialize(FMath::RoundToInt(SeedValue));
                return 0;
            }
            if (InRandSeedInfo.RandomSeeds.Num() == 0)
                return 0xffffffff;
            const int32 Index = FMath::Min<int32>(InRandSeedInfo.RandomSeeds.Num() - 1, FMath::TruncToInt(SeedValue));
            InRandSeedPayload->RandomStream.Initialize(InRandSeedInfo.RandomSeeds[Index]);
            return 0;
        }
    }
    if (InRandSeedInfo.RandomSeeds.Num() == 0)
        return 0xffffffff;
    const int32 Index = InRandSeedInfo.bRandomlySelectSeedArray ? Owner->Component->RandomStream.RandHelper(InRandSeedInfo.RandomSeeds.Num()) : 0;
    InRandSeedPayload->RandomStream.Initialize(InRandSeedInfo.RandomSeeds[Index]);
    return 0;
}

FRandomStream& UParticleModule::GetRandomStream(FParticleEmitterInstance* Owner) {
    FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
    return Payload != nullptr ? Payload->RandomStream : Owner->Component->RandomStream;
}

}

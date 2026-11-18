#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleBeam.generated.h"

namespace ue5parse::runtime {

UENUM()
enum Beam2SourceTargetMethod : int32 { PEB2STM_Default, PEB2STM_UserSet, PEB2STM_Emitter, PEB2STM_Particle, PEB2STM_Actor, PEB2STM_MAX };

UCLASS(abstract)
class UParticleModuleBeamBase : public UParticleModule {
    GENERATED_BODY()
};

UCLASS()
class UParticleModuleBeamSource : public UParticleModuleBeamBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<Beam2SourceTargetMethod> SourceMethod = PEB2STM_Default;
    UPROPERTY()
    bool bSourceAbsolute = false;
    UPROPERTY()
    FRawDistributionVector Source;
};

UCLASS()
class UParticleModuleBeamTarget : public UParticleModuleBeamBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<Beam2SourceTargetMethod> TargetMethod = PEB2STM_Default;
    UPROPERTY()
    FRawDistributionVector Target;
    UPROPERTY()
    bool bTargetAbsolute = false;
};

}

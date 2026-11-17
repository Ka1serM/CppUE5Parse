#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNotifies/AnimNotifyState.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "AnimNotifyState_Trail.generated.h"

namespace ue5parse::runtime {

class UParticleSystem;

UCLASS(const)
class UAnimNotifyState_Trail : public UAnimNotifyState {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UParticleSystem> PSTemplate = nullptr;
    UPROPERTY()
    FName FirstSocketName;
    UPROPERTY()
    FName SecondSocketName;
    UPROPERTY()
    TEnumAsByte<ETrailWidthMode> WidthScaleMode = ETrailWidthMode_FromCentre;
    UPROPERTY()
    FName WidthScaleCurve;
    UPROPERTY()
    bool bRecycleSpawnedSystems = true;
};

}

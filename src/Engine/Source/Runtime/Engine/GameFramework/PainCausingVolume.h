#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/PhysicsVolume.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"

#include "PainCausingVolume.generated.h"

namespace ue5parse::runtime {

class UDamageType;
class AController;

UCLASS()
class APainCausingVolume : public APhysicsVolume {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint8 bPainCausing : 1 = true;
    UPROPERTY()
    float DamagePerSec = 1.0f;
    UPROPERTY()
    TSubclassOf<UDamageType> DamageType;
    UPROPERTY()
    float PainInterval = 1.0f;
    UPROPERTY()
    uint8 bEntryPain : 1 = true;
    UPROPERTY()
    uint8 BACKUP_bPainCausing : 1 = false;
    UPROPERTY()
    TObjectPtr<AController> DamageInstigator = nullptr;

    explicit APainCausingVolume(const FObjectInitializer& ObjectInitializer);
};

}

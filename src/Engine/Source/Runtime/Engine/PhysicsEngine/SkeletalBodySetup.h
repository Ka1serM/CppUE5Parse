#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/BodySetup.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/PhysicalAnimationComponent.h"

#include "SkeletalBodySetup.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPhysicalAnimationProfile {
    GENERATED_BODY()
    UPROPERTY()
    FName ProfileName;
    UPROPERTY()
    FPhysicalAnimationData PhysicalAnimationData;
};

UCLASS()
class USkeletalBodySetup : public UBodySetup {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPhysicalAnimationProfile CurrentPhysicalAnimationProfile;
    UPROPERTY()
    TArray<FPhysicalAnimationProfile> PhysicalAnimationData;
    UPROPERTY()
    bool bSkipScaleFromAnimation = false;
};

}

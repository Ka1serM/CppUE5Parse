#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "AudioVolume.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FInteriorSettings {
    GENERATED_BODY()
    UPROPERTY()
    bool bIsWorldSettings = false;
    UPROPERTY()
    float ExteriorVolume = 1.0f;
    UPROPERTY()
    float ExteriorTime = 0.5f;
    UPROPERTY()
    float ExteriorLPF = 20000.0f;
    UPROPERTY()
    float ExteriorLPFTime = 0.5f;
    UPROPERTY()
    float InteriorVolume = 1.0f;
    UPROPERTY()
    float InteriorTime = 0.5f;
    UPROPERTY()
    float InteriorLPF = 20000.0f;
    UPROPERTY()
    float InteriorLPFTime = 0.5f;
};

UCLASS()
class AAudioVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit AAudioVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}

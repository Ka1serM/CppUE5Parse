#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "SingleAnimationPlayData.generated.h"

namespace ue5parse::runtime {

class UAnimationAsset;
class UAnimSingleNodeInstance;

USTRUCT()
struct FSingleAnimationPlayData {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UAnimationAsset> AnimToPlay = nullptr;
    UPROPERTY()
    bool bSavedLooping = true;
    UPROPERTY()
    bool bSavedPlaying = true;
    UPROPERTY()
    float SavedPosition = 0.0f;
    UPROPERTY()
    float SavedPlayRate = 1.0f;

    void Initialize(UAnimSingleNodeInstance* Instance);
    void PopulateFrom(UAnimSingleNodeInstance* Instance);
};

}

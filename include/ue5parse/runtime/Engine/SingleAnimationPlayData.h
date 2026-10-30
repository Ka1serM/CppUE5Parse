#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "SingleAnimationPlayData.generated.h"

namespace ue5parse::runtime {

class UAnimationAsset;
class UAnimSingleNodeInstance;

// The animation a skeletal mesh component plays in single node mode, and how (UE's SingleAnimationPlayData.h). A
// component's cooked defaults hold it; the single node instance starts from it.
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

    // Gives the instance the asset, position, rate and play state.
    void Initialize(UAnimSingleNodeInstance* Instance);
    // Takes the instance's current asset, position, rate and play state.
    void PopulateFrom(UAnimSingleNodeInstance* Instance);
    // Clamps the position to the asset's length.
    void ValidatePosition();
};

}

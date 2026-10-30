#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_AssetPlayerRelevancyBase.generated.h"

namespace ue5parse::runtime {

class UAnimationAsset;

// A node whose time and weight the state machines read to find a state's most relevant player (UE's
// Animation/AnimNode_AssetPlayerRelevancyBase.h).
USTRUCT()
struct FAnimNode_AssetPlayerRelevancyBase : public FAnimNode_Base {
    GENERATED_BODY()
public:
    virtual UAnimationAsset* GetAnimAsset() const { return nullptr; }
    virtual float GetAccumulatedTime() const { return 0; }
    virtual float GetCurrentAssetTime() const { return 0; }
    // The time from the asset's start in the direction of play.
    virtual float GetCurrentAssetTimePlayRateAdjusted() const { return GetCurrentAssetTime(); }
    virtual float GetCurrentAssetLength() const { return 0; }
    virtual float GetCachedBlendWeight() const { return 0; }
    virtual void ClearCachedBlendWeight() {}
    virtual bool GetIgnoreForRelevancyTest() const { return false; }
};

}

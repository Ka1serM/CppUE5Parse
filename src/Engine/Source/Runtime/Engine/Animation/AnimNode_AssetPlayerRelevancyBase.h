#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_AssetPlayerRelevancyBase.generated.h"

namespace ue5parse::runtime {

class UAnimationAsset;

USTRUCT()
struct FAnimNode_AssetPlayerRelevancyBase : public FAnimNode_Base {
    GENERATED_BODY()
public:
    virtual UAnimationAsset* GetAnimAsset() const { return nullptr; }
    virtual float GetAccumulatedTime() const { return 0; }
    virtual float GetCurrentAssetTime() const { return 0; }
    virtual float GetCurrentAssetTimePlayRateAdjusted() const { return GetCurrentAssetTime(); }
    virtual float GetCurrentAssetLength() const { return 0; }
    virtual float GetCachedBlendWeight() const { return 0; }
    virtual void ClearCachedBlendWeight() {}
    virtual bool GetIgnoreForRelevancyTest() const { return false; }
};

}

#pragma once

#include "ue5parse/runtime/Engine/Animation/BoneReference.h"

#include "BlendProfile.generated.h"

namespace ue5parse::runtime {

class USkeleton;

enum class EBlendProfileMode : uint8 { TimeFactor, WeightFactor, BlendMask };

USTRUCT()
struct FBlendProfileBoneEntry {
    GENERATED_BODY()
    UPROPERTY()
    FBoneReference BoneReference;
    UPROPERTY()
    float BlendScale = 0.0f;
};

// UBlendProfile: a blend scale per bone, for blends and blend masks (UE's Animation/BlendProfile.h).
UCLASS()
class UBlendProfile : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USkeleton> OwningSkeleton = nullptr;
    UPROPERTY()
    TArray<FBlendProfileBoneEntry> ProfileEntries;
    UPROPERTY()
    EBlendProfileMode Mode = EBlendProfileMode::WeightFactor;

    // The scale of a skeleton bone: its entry's, else 1 (0 for a blend mask, which only lists the bones it lets
    // through).
    float GetBoneBlendScale(int32 InBoneIdx) const;
    float GetBoneBlendScale(FName InBoneName) const;
    int32 GetNumBlendEntries() const { return ProfileEntries.Num(); }
    EBlendProfileMode GetMode() const { return Mode; }
    float GetDefaultBlendScale() const { return Mode == EBlendProfileMode::BlendMask ? 0.0f : 1.0f; }
};

}

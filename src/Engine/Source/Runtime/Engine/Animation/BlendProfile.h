#pragma once

#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

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

    float GetBoneBlendScale(int32 InBoneIdx) const;
    float GetBoneBlendScale(FName InBoneName) const;
    float GetDefaultBlendScale() const { return Mode == EBlendProfileMode::BlendMask ? 0.0f : 1.0f; }
};

}

#pragma once

#include "ue5parse/runtime/Engine/AlphaBlend.h"
#include "ue5parse/runtime/Engine/Animation/AnimCompositeBase.h"

#include "AnimMontage.generated.h"

namespace ue5parse::runtime {

class UBlendProfile;

// FCompositeSection: a named section of a montage and the section after it.
USTRUCT()
struct FCompositeSection : public FAnimLinkableElement {
    GENERATED_BODY()
    UPROPERTY()
    FName SectionName;
    UPROPERTY()
    FName NextSectionName;
    UPROPERTY()
    TArray<UObject*> MetaData;
};

// FSlotAnimationTrack: the segments a montage plays in one slot.
USTRUCT()
struct FSlotAnimationTrack {
    GENERATED_BODY()
    UPROPERTY()
    FName SlotName;
    UPROPERTY()
    FAnimTrack AnimTrack;
};

namespace EMontageBlendMode {
enum Type : int32 { Standard, Inertialization };
}

// UAnimMontage (UE's Animation/AnimMontage.h): sections of slot tracks that gameplay plays and blends.
UCLASS()
class UAnimMontage : public UAnimCompositeBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<EMontageBlendMode::Type> BlendModeIn = EMontageBlendMode::Standard;
    UPROPERTY()
    TEnumAsByte<EMontageBlendMode::Type> BlendModeOut = EMontageBlendMode::Standard;
    // The constructor sets both blend times to 0.25.
    UPROPERTY()
    FAlphaBlend BlendIn{0.25f};
    UPROPERTY()
    FAlphaBlend BlendOut{0.25f};
    UPROPERTY()
    float BlendOutTriggerTime = -1;
    UPROPERTY()
    FName SyncGroup;
    UPROPERTY()
    int32 SyncSlotIndex = 0;
    UPROPERTY()
    TArray<FCompositeSection> CompositeSections;
    UPROPERTY()
    TArray<FSlotAnimationTrack> SlotAnimTracks;
    UPROPERTY()
    bool bEnableRootMotionTranslation = false;
    UPROPERTY()
    bool bEnableRootMotionRotation = false;
    UPROPERTY()
    bool bEnableAutoBlendOut = true;
    UPROPERTY()
    UBlendProfile* BlendProfileIn = nullptr;
    UPROPERTY()
    UBlendProfile* BlendProfileOut = nullptr;

    int32 GetSectionIndex(FName InSectionName) const;
    FName GetSectionName(int32 SectionIndex) const;
    const FSlotAnimationTrack* FindSlotTrack(FName InSlotName) const;
    float GetDefaultBlendInTime() const { return BlendIn.GetBlendTime(); }
    float GetDefaultBlendOutTime() const { return BlendOut.GetBlendTime(); }
    FName GetGroupName() const;
};

}
